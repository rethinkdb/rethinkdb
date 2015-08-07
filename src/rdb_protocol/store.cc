// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/store.hpp"

#include "btree/backfill_debug.hpp"
#include "btree/reql_specific.hpp"
#include "btree/superblock.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/erase_range.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/shards.hpp"
#include "rdb_protocol/table_common.hpp"

void store_t::note_reshard() {
    changefeed_servers.clear();
}

reql_version_t update_sindex_last_compatible_version(secondary_index_t *sindex,
                                                     buf_lock_t *sindex_block) {
    sindex_disk_info_t sindex_info;
    deserialize_sindex_info(sindex->opaque_definition, &sindex_info);

    reql_version_t res = sindex_info.mapping_version_info.original_reql_version;

    if (sindex_info.mapping_version_info.latest_checked_reql_version
        != reql_version_t::LATEST) {

        sindex_info.mapping_version_info.latest_compatible_reql_version = res;
        sindex_info.mapping_version_info.latest_checked_reql_version =
            reql_version_t::LATEST;

        write_message_t wm;
        serialize_sindex_info(&wm, sindex_info);

        vector_stream_t stream;
        stream.reserve(wm.size());
        int write_res = send_write_message(&stream, &wm);
        guarantee(write_res == 0);

        sindex->opaque_definition = stream.vector();

        ::set_secondary_index(sindex_block, sindex->id, *sindex);
    }

    return res;
}

void store_t::update_outdated_sindex_list(buf_lock_t *sindex_block) {
    if (index_report.has()) {
        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(sindex_block, &sindexes);

        std::set<std::string> index_set;
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (!it->first.being_deleted &&
                update_sindex_last_compatible_version(&it->second,
                                                      sindex_block) !=
                    reql_version_t::LATEST) {
                index_set.insert(it->first.name);
            }
        }
        index_report->set_outdated_indexes(std::move(index_set));
    }
}

void store_t::help_construct_bring_sindexes_up_to_date() {
    // Make sure to continue bringing sindexes up-to-date if it was interrupted earlier

    // This uses a dummy interruptor because this is the only thing using the store at
    //  the moment (since we are still in the constructor), so things should complete
    //  rather quickly.
    cond_t dummy_interruptor;
    write_token_t token;
    new_write_token(&token);

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(1,
                                 write_durability_t::SOFT,
                                 &token,
                                 &txn,
                                 &superblock,
                                 &dummy_interruptor);

    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::write);

    superblock.reset();

    struct sindex_clearer_t {
        static void clear(store_t *store,
                          secondary_index_t sindex,
                          auto_drainer_t::lock_t store_keepalive) {
            try {
                // Note that we can safely use a noop deleter here, since the
                // secondary index cannot be in use at this point and we therefore
                // don't have to detach anything.
                rdb_noop_deletion_context_t noop_deletion_context;
                rdb_value_sizer_t sizer(store->cache->max_block_size());

                /* Clear the sindex. */
                store->clear_sindex(
                    sindex,
                    &sizer,
                    &noop_deletion_context,
                    store_keepalive.get_drain_signal());
            } catch (const interrupted_exc_t &e) {
                /* Ignore */
            }
        }
    };

    // Get the map of indexes and check if any were postconstructing
    // Drop these and recreate them (see github issue #2925)
    std::set<sindex_name_t> sindexes_to_update;
    {
        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (!it->second.being_deleted && !it->second.post_construction_complete) {
                bool success = mark_secondary_index_deleted(&sindex_block, it->first);
                guarantee(success);

                success = add_sindex_internal(
                    it->first, it->second.opaque_definition, &sindex_block);
                guarantee(success);
                sindexes_to_update.insert(it->first);
            }
        }
    }

    // Get the new map of indexes, now that we're deleting the old postconstructing ones
    // Kick off coroutines to finish deleting all indexes that are being deleted
    {
        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (it->second.being_deleted) {
                coro_t::spawn_sometime(std::bind(&sindex_clearer_t::clear,
                                                 this, it->second, drainer.lock()));
            }
        }
    }

    if (!sindexes_to_update.empty()) {
        rdb_protocol::bring_sindexes_up_to_date(sindexes_to_update, this,
                                                &sindex_block);
    }
}

scoped_ptr_t<sindex_superblock_t> acquire_sindex_for_read(
    store_t *store,
    real_superblock_t *superblock,
    const std::string &table_name,
    const std::string &sindex_id,
    sindex_disk_info_t *sindex_info_out,
    uuid_u *sindex_uuid_out) {
    rassert(sindex_info_out != NULL);
    rassert(sindex_uuid_out != NULL);

    scoped_ptr_t<sindex_superblock_t> sindex_sb;
    std::vector<char> sindex_mapping_data;

    uuid_u sindex_uuid;
    try {
        bool found = store->acquire_sindex_superblock_for_read(
            sindex_name_t(sindex_id),
            table_name,
            superblock,
            &sindex_sb,
            &sindex_mapping_data,
            &sindex_uuid);
        // TODO: consider adding some logic on the machine handling the
        // query to attach a real backtrace here.
        rcheck_toplevel(found, ql::base_exc_t::OP_FAILED,
                strprintf("Index `%s` was not found on table `%s`.",
                          sindex_id.c_str(), table_name.c_str()));
    } catch (const sindex_not_ready_exc_t &e) {
        throw ql::exc_t(
            ql::base_exc_t::OP_FAILED, e.what(), ql::backtrace_id_t::empty());
    }

    try {
        deserialize_sindex_info(sindex_mapping_data, sindex_info_out);
    } catch (const archive_exc_t &e) {
        crash("%s", e.what());
    }

    *sindex_uuid_out = sindex_uuid;
    return sindex_sb;
}

void do_read(ql::env_t *env,
             store_t *store,
             btree_slice_t *btree,
             real_superblock_t *superblock,
             const rget_read_t &rget,
             rget_read_response_t *res,
             release_superblock_t release_superblock) {
    if (!rget.sindex) {
        // Normal rget
        rdb_rget_slice(btree, rget.region.inner, superblock,
                       env, rget.batchspec, rget.transforms, rget.terminal,
                       rget.sorting, res, release_superblock);
    } else {
        sindex_disk_info_t sindex_info;
        uuid_u sindex_uuid;
        scoped_ptr_t<sindex_superblock_t> sindex_sb;
        region_t true_region;
        try {
            sindex_sb =
                acquire_sindex_for_read(
                    store,
                    superblock,
                    rget.table_name,
                    rget.sindex->id,
                    &sindex_info,
                    &sindex_uuid);
            ql::skey_version_t skey_version =
                ql::skey_version_from_reql_version(
                    sindex_info.mapping_version_info.latest_compatible_reql_version);
            res->skey_version = skey_version;
            true_region = rget.sindex->region
                ? *rget.sindex->region
                : region_t(rget.sindex->original_range.to_sindex_keyrange(skey_version));
        } catch (const ql::exc_t &e) {
            res->result = e;
            return;
        } catch (const ql::datum_exc_t &e) {
            // TODO: consider adding some logic on the machine handling the
            // query to attach a real backtrace here.
            res->result = ql::exc_t(e, ql::backtrace_id_t::empty());
            return;
        }

        if (sindex_info.geo == sindex_geo_bool_t::GEO) {
            res->result = ql::exc_t(
                ql::base_exc_t::LOGIC,
                strprintf(
                    "Index `%s` is a geospatial index.  Only get_nearest and "
                    "get_intersecting can use a geospatial index.",
                    rget.sindex->id.c_str()),
                ql::backtrace_id_t::empty());
            return;
        }

        rdb_rget_secondary_slice(
            store->get_sindex_slice(sindex_uuid),
            rget.sindex->original_range, std::move(true_region),
            sindex_sb.get(), env, rget.batchspec, rget.transforms,
            rget.terminal, rget.region.inner, rget.sorting,
            sindex_info, res, release_superblock_t::RELEASE);
    }
}

// TODO: get rid of this extra response_t copy on the stack
struct rdb_read_visitor_t : public boost::static_visitor<void> {
    void operator()(const changefeed_subscribe_t &s) {
        auto *cserver = store->changefeed_server(s.region);
        if (cserver == nullptr) {
            cserver = store->make_changefeed_server(s.region);
        }
        guarantee(cserver);
        cserver->add_client(s.addr, s.region);
        response->response = changefeed_subscribe_response_t();
        auto res = boost::get<changefeed_subscribe_response_t>(&response->response);
        guarantee(res != NULL);
        res->server_uuids.insert(cserver->get_uuid());
        res->addrs.insert(cserver->get_stop_addr());
    }

    void operator()(const changefeed_limit_subscribe_t &s) {
        ql::env_t env(ctx, ql::return_empty_normal_batches_t::NO,
                      interruptor, s.optargs, trace);
        ql::stream_t stream;
        {
            std::vector<scoped_ptr_t<ql::op_t> > ops;
            for (const auto &transform : s.spec.range.transforms) {
                ops.push_back(make_op(transform));
            }
            rget_read_t rget;
            rget.region = s.region;
            rget.table_name = s.table;
            rget.batchspec = ql::batchspec_t::all(); // Terminal takes care of stopping.
            if (s.spec.range.sindex) {
                rget.terminal = ql::limit_read_t{
                    is_primary_t::NO,
                    s.spec.limit,
                    s.spec.range.sorting,
                    &ops};
                rget.sindex = sindex_rangespec_t(
                    *s.spec.range.sindex,
                    boost::none, // We just want to use whole range.
                    s.spec.range.range);
            } else {
                rget.terminal = ql::limit_read_t{
                    is_primary_t::YES,
                    s.spec.limit,
                    s.spec.range.sorting,
                    &ops};
            }
            rget.sorting = s.spec.range.sorting;
            // The superblock will instead be released in `store_t::read`
            // shortly after this function returns.
            rget_read_response_t resp;
            do_read(&env, store, btree, superblock, rget, &resp,
                    release_superblock_t::KEEP);
            auto *gs = boost::get<ql::grouped_t<ql::stream_t> >(&resp.result);
            if (gs == NULL) {
                auto *exc = boost::get<ql::exc_t>(&resp.result);
                guarantee(exc != NULL);
                response->response = resp;
                return;
            }
            stream = groups_to_batch(gs->get_underlying_map());
        }
        auto lvec = ql::changefeed::mangle_sort_truncate_stream(
            std::move(stream),
            s.spec.range.sindex ? is_primary_t::NO : is_primary_t::YES,
            s.spec.range.sorting,
            s.spec.limit);

        auto *cserver = store->changefeed_server(s.region);
        if (cserver == nullptr) {
            cserver = store->make_changefeed_server(s.region);
        }
        guarantee(cserver);
        cserver->add_limit_client(
            s.addr,
            s.region,
            s.table,
            ctx,
            s.optargs,
            s.uuid,
            s.spec,
            ql::changefeed::limit_order_t(s.spec.range.sorting),
            std::move(lvec));
        auto addr = cserver->get_limit_stop_addr();
        std::vector<decltype(addr)> vec{addr};
        response->response = changefeed_limit_subscribe_response_t(1, std::move(vec));
    }

    changefeed_stamp_response_t do_stamp(const changefeed_stamp_t &s) {
        if (auto *cserver = store->changefeed_server(s.region)) {
            if (boost::optional<uint64_t> stamp = cserver->get_stamp(s.addr)) {
                changefeed_stamp_response_t out;
                out.stamps = std::map<uuid_u, uint64_t>();
                (*out.stamps)[cserver->get_uuid()] = *stamp;
                return out;
            }
        }
        return changefeed_stamp_response_t();
    }

    void operator()(const changefeed_stamp_t &s) {
        response->response = do_stamp(s);
    }

    void operator()(const changefeed_point_stamp_t &s) {
        response->response = changefeed_point_stamp_response_t();
        auto *res = boost::get<changefeed_point_stamp_response_t>(&response->response);
        if (auto *changefeed_server = store->changefeed_server(s.key)) {
            res->resp = changefeed_point_stamp_response_t::valid_response_t();
            auto *vres = &*res->resp;
            if (boost::optional<uint64_t> stamp = changefeed_server->get_stamp(s.addr)) {
                vres->stamp = std::make_pair(changefeed_server->get_uuid(), *stamp);
            } else {
                // The client was removed, so no future messages are coming.
                vres->stamp = std::make_pair(changefeed_server->get_uuid(),
                                             std::numeric_limits<uint64_t>::max());
            }
            point_read_response_t val;
            rdb_get(s.key, btree, superblock, &val, trace);
            vres->initial_val = val.data;
        } else {
            res->resp = boost::none;
        }
    }

    void operator()(const point_read_t &get) {
        response->response = point_read_response_t();
        point_read_response_t *res =
            boost::get<point_read_response_t>(&response->response);
        rdb_get(get.key, btree, superblock, res, trace);
    }

    void operator()(const intersecting_geo_read_t &geo_read) {
        ql::env_t ql_env(ctx, ql::return_empty_normal_batches_t::NO,
                         interruptor, geo_read.optargs, trace);

        response->response = rget_read_response_t();
        rget_read_response_t *res =
            boost::get<rget_read_response_t>(&response->response);

        sindex_disk_info_t sindex_info;
        uuid_u sindex_uuid;
        scoped_ptr_t<sindex_superblock_t> sindex_sb;
        try {
            sindex_sb =
                acquire_sindex_for_read(
                    store,
                    superblock,
                    geo_read.table_name,
                    geo_read.sindex.id,
                &sindex_info, &sindex_uuid);
        } catch (const ql::exc_t &e) {
            res->result = e;
            return;
        }

        if (sindex_info.geo != sindex_geo_bool_t::GEO) {
            res->result = ql::exc_t(
                ql::base_exc_t::LOGIC,
                strprintf(
                    "Index `%s` is not a geospatial index.  get_intersecting can only "
                    "be used with a geospatial index.",
                    geo_read.sindex.id.c_str()),
                ql::backtrace_id_t::empty());
            return;
        }

        guarantee(geo_read.sindex.region);
        rdb_get_intersecting_slice(
            store->get_sindex_slice(sindex_uuid),
            geo_read.query_geometry,
            *geo_read.sindex.region,
            sindex_sb.get(),
            &ql_env,
            geo_read.batchspec,
            geo_read.transforms,
            geo_read.terminal,
            geo_read.region.inner,
            sindex_info,
            res);
    }

    void operator()(const nearest_geo_read_t &geo_read) {
        ql::env_t ql_env(ctx, ql::return_empty_normal_batches_t::NO,
                         interruptor, geo_read.optargs, trace);

        response->response = nearest_geo_read_response_t();
        nearest_geo_read_response_t *res =
            boost::get<nearest_geo_read_response_t>(&response->response);

        sindex_disk_info_t sindex_info;
        uuid_u sindex_uuid;
        scoped_ptr_t<sindex_superblock_t> sindex_sb;
        try {
            sindex_sb =
                acquire_sindex_for_read(
                    store,
                    superblock,
                    geo_read.table_name,
                    geo_read.sindex_id,
                &sindex_info, &sindex_uuid);
        } catch (const ql::exc_t &e) {
            res->results_or_error = e;
            return;
        }

        if (sindex_info.geo != sindex_geo_bool_t::GEO) {
            res->results_or_error = ql::exc_t(
                ql::base_exc_t::LOGIC,
                strprintf(
                    "Index `%s` is not a geospatial index.  get_nearest can only be "
                    "used with a geospatial index.",
                    geo_read.sindex_id.c_str()),
                ql::backtrace_id_t::empty());
            return;
        }

        rdb_get_nearest_slice(
            store->get_sindex_slice(sindex_uuid),
            geo_read.center,
            geo_read.max_dist,
            geo_read.max_results,
            geo_read.geo_system,
            sindex_sb.get(),
            &ql_env,
            geo_read.region.inner,
            sindex_info,
            res);
    }

    void operator()(const rget_read_t &rget) {
        response->response = rget_read_response_t();
        auto *res = boost::get<rget_read_response_t>(&response->response);

        if (rget.stamp) {
            res->stamp_response = changefeed_stamp_response_t();
            changefeed_stamp_response_t r = do_stamp(*rget.stamp);
            if (r.stamps) {
                res->stamp_response = r;
            } else {
                res->result = ql::exc_t(
                    ql::base_exc_t::OP_FAILED,
                    "Feed aborted before initial values were read.",
                    ql::backtrace_id_t::empty());
                return;
            }
        }

        if (rget.transforms.size() != 0 || rget.terminal) {
            // This asserts that the optargs have been initialized.  (There is always
            // a 'db' optarg.)  We have the same assertion in
            // rdb_r_unshard_visitor_t.
            rassert(rget.optargs.size() != 0);
        }
        ql::env_t ql_env(ctx, ql::return_empty_normal_batches_t::NO,
                         interruptor, rget.optargs, trace);
        do_read(&ql_env, store, btree, superblock, rget, res,
                release_superblock_t::RELEASE);
    }

    void operator()(const distribution_read_t &dg) {
        response->response = distribution_read_response_t();
        distribution_read_response_t *res = boost::get<distribution_read_response_t>(&response->response);
        rdb_distribution_get(dg.max_depth, dg.region.inner.left,
                             superblock, res);
        for (std::map<store_key_t, int64_t>::iterator it = res->key_counts.begin(); it != res->key_counts.end(); ) {
            if (!dg.region.inner.contains_key(store_key_t(it->first))) {
                std::map<store_key_t, int64_t>::iterator tmp = it;
                ++it;
                res->key_counts.erase(tmp);
            } else {
                ++it;
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dg.result_limit > 0 && res->key_counts.size() > dg.result_limit) {
            scale_down_distribution(dg.result_limit, &res->key_counts);
        }

        res->region = dg.region;
    }

    void operator()(const dummy_read_t &) {
        response->response = dummy_read_response_t();
    }

    rdb_read_visitor_t(btree_slice_t *_btree,
                       store_t *_store,
                       real_superblock_t *_superblock,
                       rdb_context_t *_ctx,
                       read_response_t *_response,
                       profile::trace_t *_trace,
                       signal_t *_interruptor) :
        response(_response),
        ctx(_ctx),
        interruptor(_interruptor),
        btree(_btree),
        store(_store),
        superblock(_superblock),
        trace(_trace) { }

private:

    read_response_t *const response;
    rdb_context_t *const ctx;
    signal_t *const interruptor;
    btree_slice_t *const btree;
    store_t *const store;
    real_superblock_t *const superblock;
    profile::trace_t *const trace;

    DISABLE_COPYING(rdb_read_visitor_t);
};

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            real_superblock_t *superblock,
                            signal_t *interruptor) {
    scoped_ptr_t<profile::trace_t> trace = ql::maybe_make_profile_trace(read.profile);

    {
        profile::starter_t start_read("Perform read on shard.", trace);
        rdb_read_visitor_t v(btree.get(), this,
                             superblock,
                             ctx, response, trace.get_or_null(), interruptor);
        boost::apply_visitor(v, read.read);
    }

    response->n_shards = 1;
    if (trace.has()) {
        response->event_log = std::move(*trace).extract_event_log();
    }
    // This is a tad hacky, this just adds a stop event to signal the end of the
    // parallel task.

    // TODO: Is this is the right thing to do if profiling's not enabled?
    response->event_log.push_back(profile::stop_t());
}


class func_replacer_t : public btree_batched_replacer_t {
public:
    func_replacer_t(ql::env_t *_env, const ql::wire_func_t &wf, return_changes_t _return_changes)
        : env(_env), f(wf.compile_wire_func()), return_changes(_return_changes) { }
    ql::datum_t replace(
        const ql::datum_t &d, size_t) const {
        return f->call(env, d, ql::LITERAL_OK)->as_datum();
    }
    return_changes_t should_return_changes() const { return return_changes; }
private:
    ql::env_t *const env;
    const counted_t<const ql::func_t> f;
    const return_changes_t return_changes;
};

class datum_replacer_t : public btree_batched_replacer_t {
public:
    explicit datum_replacer_t(const batched_insert_t &bi)
        : datums(&bi.inserts), conflict_behavior(bi.conflict_behavior),
          pkey(bi.pkey), return_changes(bi.return_changes) { }
    ql::datum_t replace(const ql::datum_t &d, size_t index) const {
        guarantee(index < datums->size());
        ql::datum_t newd = (*datums)[index];
        return resolve_insert_conflict(pkey, d, newd, conflict_behavior);
    }
    return_changes_t should_return_changes() const { return return_changes; }
private:
    const std::vector<ql::datum_t> *const datums;
    const conflict_behavior_t conflict_behavior;
    const std::string pkey;
    const return_changes_t return_changes;
};

struct rdb_write_visitor_t : public boost::static_visitor<void> {
    void operator()(const batched_replace_t &br) {
        ql::env_t ql_env(ctx, ql::return_empty_normal_batches_t::NO,
                         interruptor, br.optargs, trace);
        rdb_modification_report_cb_t sindex_cb(
            store, &sindex_block,
            auto_drainer_t::lock_t(&store->drainer));
        func_replacer_t replacer(&ql_env, br.f, br.return_changes);

        response->response =
            rdb_batched_replace(
                btree_info_t(btree, timestamp, datum_string_t(br.pkey)),
                superblock,
                br.keys,
                &replacer,
                &sindex_cb,
                ql_env.limits(),
                sampler,
                trace);
    }

    void operator()(const batched_insert_t &bi) {
        rdb_modification_report_cb_t sindex_cb(
            store, &sindex_block,
            auto_drainer_t::lock_t(&store->drainer));
        datum_replacer_t replacer(bi);
        std::vector<store_key_t> keys;
        keys.reserve(bi.inserts.size());
        for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
            keys.emplace_back(it->get_field(datum_string_t(bi.pkey)).print_primary());
        }
        response->response =
            rdb_batched_replace(
                btree_info_t(btree, timestamp, datum_string_t(bi.pkey)),
                superblock,
                keys,
                &replacer,
                &sindex_cb,
                bi.limits,
                sampler,
                trace);
    }

    void operator()(const point_write_t &w) {
        sampler->new_sample();
        response->response = point_write_response_t();
        point_write_response_t *res =
            boost::get<point_write_response_t>(&response->response);

        backfill_debug_key(w.key, strprintf("upsert %" PRIu64, timestamp.longtime));

        rdb_live_deletion_context_t deletion_context;
        rdb_modification_report_t mod_report(w.key);
        rdb_set(w.key, w.data, w.overwrite, btree, timestamp, superblock->get(),
                &deletion_context, res, &mod_report.info, trace);

        update_sindexes(mod_report);
    }

    void operator()(const point_delete_t &d) {
        sampler->new_sample();
        response->response = point_delete_response_t();
        point_delete_response_t *res =
            boost::get<point_delete_response_t>(&response->response);

        backfill_debug_key(d.key, strprintf("delete %" PRIu64, timestamp.longtime));

        rdb_live_deletion_context_t deletion_context;
        rdb_modification_report_t mod_report(d.key);
        rdb_delete(d.key, btree, timestamp, superblock->get(), &deletion_context,
                delete_mode_t::REGULAR_QUERY, res, &mod_report.info, trace);

        update_sindexes(mod_report);
    }

    void operator()(const sync_t &) {
        sampler->new_sample();
        response->response = sync_response_t();

        // We know this sync_t operation will force all preceding write transactions
        // (on our cache_conn_t) to flush before or at the same time, because the
        // cache guarantees that.  (Right now it will force _all_ preceding write
        // transactions to flush, on any conn, because they all touch the metainfo in
        // the superblock.)
    }

    void operator()(const dummy_write_t &) {
        response->response = dummy_write_response_t();
    }

    rdb_write_visitor_t(btree_slice_t *_btree,
                        store_t *_store,
                        txn_t *_txn,
                        scoped_ptr_t<real_superblock_t> *_superblock,
                        repli_timestamp_t _timestamp,
                        rdb_context_t *_ctx,
                        profile::sampler_t *_sampler,
                        profile::trace_t *_trace,
                        write_response_t *_response,
                        signal_t *_interruptor) :
        btree(_btree),
        store(_store),
        txn(_txn),
        response(_response),
        ctx(_ctx),
        interruptor(_interruptor),
        superblock(_superblock),
        timestamp(_timestamp),
        sampler(_sampler),
        trace(_trace),
        sindex_block((*superblock)->expose_buf(),
                     (*superblock)->get_sindex_block_id(),
                     access_t::write) {
    }

private:
    void update_sindexes(const rdb_modification_report_t &mod_report) {
        std::vector<rdb_modification_report_t> mod_reports;
        // This copying of the mod_report is inefficient, but it seems this
        // function is only used for unit tests at the moment anyway.
        mod_reports.push_back(mod_report);
        store->update_sindexes(txn, &sindex_block, mod_reports,
                               true /* release_sindex_block */);
    }

    btree_slice_t *const btree;
    store_t *const store;
    txn_t *const txn;
    write_response_t *const response;
    rdb_context_t *const ctx;
    signal_t *const interruptor;
    scoped_ptr_t<real_superblock_t> *const superblock;
    const repli_timestamp_t timestamp;
    profile::sampler_t *const sampler;
    profile::trace_t *const trace;
    buf_lock_t sindex_block;
    profile::event_log_t event_log_out;

    DISABLE_COPYING(rdb_write_visitor_t);
};

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             state_timestamp_t timestamp,
                             scoped_ptr_t<real_superblock_t> *superblock,
                             signal_t *interruptor) {
    scoped_ptr_t<profile::trace_t> trace = ql::maybe_make_profile_trace(write.profile);

    {
        profile::sampler_t start_write("Perform write on shard.", trace);
        rdb_write_visitor_t v(btree.get(),
                              this,
                              (*superblock)->expose_buf().txn(),
                              superblock,
                              timestamp.to_repli_timestamp(),
                              ctx,
                              &start_write,
                              trace.get_or_null(),
                              response,
                              interruptor);
        boost::apply_visitor(v, write.write);
    }

    response->n_shards = 1;
    if (trace.has()) {
        response->event_log = std::move(*trace).extract_event_log();
    }
    // This is a tad hacky, this just adds a stop event to signal the end of the
    // parallel task.

    // TODO: Is this the right thing to do if profiling's not enabled?
    response->event_log.push_back(profile::stop_t());
}

void store_t::delayed_clear_sindex(
        secondary_index_t sindex,
        auto_drainer_t::lock_t store_keepalive)
        THROWS_NOTHING
{
    try {
        rdb_value_sizer_t sizer(cache->max_block_size());
        /* If the index had been completely constructed, we must
         * detach its values since snapshots might be accessing it.
         * If on the other hand the index had not finished post
         * construction, it would be incorrect to do so.
         * The reason being that some of the values that the sindex
         * points to might have been deleted in the meantime
         * (the deletion would be on the sindex queue, but might
         * not have found its way into the index tree yet). */
        rdb_live_deletion_context_t live_deletion_context;
        rdb_post_construction_deletion_context_t post_con_deletion_context;
        deletion_context_t *actual_deletion_context =
            sindex.post_construction_complete
            ? static_cast<deletion_context_t *>(&live_deletion_context)
            : static_cast<deletion_context_t *>(&post_con_deletion_context);

        /* Clear the sindex. */
        clear_sindex(sindex,
                     &sizer,
                     actual_deletion_context,
                     store_keepalive.get_drain_signal());
    } catch (const interrupted_exc_t &e) {
        /* Ignore. The sindex deletion will continue when the store
        is next started up. */
    }
}

namespace_id_t const &store_t::get_table_id() const {
    return table_id;
}

store_t::sindex_context_map_t *store_t::get_sindex_context_map() {
    return &sindex_context;
}
