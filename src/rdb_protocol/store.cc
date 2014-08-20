// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/store.hpp"

#include "btree/slice.hpp"
#include "btree/superblock.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/cow_ptr.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"

void store_t::note_reshard() {
    if (changefeed_server.has()) {
        changefeed_server->stop_all();
    }
}

reql_version_t update_sindex_last_compatible_version(secondary_index_t *sindex,
                                                     buf_lock_t *sindex_block) {
    sindex_disk_info_t sindex_info;
    deserialize_sindex_info(sindex->opaque_definition, &sindex_info);

    reql_version_t res;
    switch (sindex_info.mapping_version_info.original_reql_version) {
    case reql_version_t::v1_13:
        res = reql_version_t::v1_13;
        break;
    case reql_version_t::v1_14:
        res = reql_version_t::v1_14;
        break;
    default:
        unreachable();
    }

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
    if (index_report != NULL) {
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
    write_token_pair_t token_pair;
    store_view_t::new_write_token_pair(&token_pair);

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(repli_timestamp_t::distant_past,
                                 1,
                                 write_durability_t::SOFT,
                                 &token_pair,
                                 &txn,
                                 &superblock,
                                 &dummy_interruptor);

    buf_lock_t sindex_block
        = acquire_sindex_block_for_write(superblock->expose_buf(),
                                        superblock->get_sindex_block_id());

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

                success = add_sindex(it->first, it->second.opaque_definition, &sindex_block);
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

// TODO: get rid of this extra response_t copy on the stack
struct rdb_read_visitor_t : public boost::static_visitor<void> {
    void operator()(const changefeed_subscribe_t &s) {
        guarantee(store->changefeed_server.has());
        store->changefeed_server->add_client(s.addr, s.region);
        response->response = changefeed_subscribe_response_t();
        auto res = boost::get<changefeed_subscribe_response_t>(&response->response);
        guarantee(res != NULL);
        res->server_uuids.insert(store->changefeed_server->get_uuid());
        res->addrs.insert(store->changefeed_server->get_stop_addr());
    }

    void operator()(const changefeed_stamp_t &s) {
        guarantee(store->changefeed_server.has());
        response->response = changefeed_stamp_response_t();
        auto res = boost::get<changefeed_stamp_response_t>(&response->response);
        res->stamps[store->changefeed_server->get_uuid()]
            = store->changefeed_server->get_stamp(s.addr);
    }

    void operator()(const changefeed_point_stamp_t &s) {
        guarantee(store->changefeed_server.has());
        response->response = changefeed_point_stamp_response_t();
        auto res = boost::get<changefeed_point_stamp_response_t>(&response->response);
        res->stamp = std::make_pair(
            store->changefeed_server->get_uuid(),
            store->changefeed_server->get_stamp(s.addr));
        point_read_response_t val;
        rdb_get(s.key, btree, superblock, &val, trace);
        res->initial_val = val.data;
    }

    void operator()(const point_read_t &get) {
        response->response = point_read_response_t();
        point_read_response_t *res =
            boost::get<point_read_response_t>(&response->response);
        rdb_get(get.key, btree, superblock, res, trace);
    }

    void operator()(const intersecting_geo_read_t &geo_read) {
        ql::env_t ql_env(ctx, interruptor, geo_read.optargs, trace);

        response->response = intersecting_geo_read_response_t();
        intersecting_geo_read_response_t *res =
            boost::get<intersecting_geo_read_response_t>(&response->response);

        sindex_disk_info_t sindex_info;
        uuid_u sindex_uuid;
        scoped_ptr_t<real_superblock_t> sindex_sb;
        try {
            sindex_sb =
                acquire_sindex_for_read(geo_read.table_name, geo_read.sindex_id,
                &sindex_info, &sindex_uuid);
        } catch (const ql::exc_t &e) {
            res->results_or_error = e;
            return;
        }

        if (sindex_info.geo != sindex_geo_bool_t::GEO) {
            res->results_or_error = ql::exc_t(
                ql::base_exc_t::GENERIC,
                strprintf(
                    "Index `%s` is not a geospatial index.  get_intersecting can only "
                    "be used with a geospatial index.",
                    geo_read.sindex_id.c_str()),
                NULL);
            return;
        }

        rdb_get_intersecting_slice(
            store->get_sindex_slice(sindex_uuid),
            geo_read.query_geometry,
            sindex_sb.get(), &ql_env,
            geo_read.region.inner, sindex_info, res);
    }

    void operator()(const nearest_geo_read_t &geo_read) {
        ql::env_t ql_env(ctx, interruptor, geo_read.optargs, trace);

        response->response = nearest_geo_read_response_t();
        nearest_geo_read_response_t *res =
            boost::get<nearest_geo_read_response_t>(&response->response);

        sindex_disk_info_t sindex_info;
        uuid_u sindex_uuid;
        scoped_ptr_t<real_superblock_t> sindex_sb;
        try {
            sindex_sb =
                acquire_sindex_for_read(geo_read.table_name, geo_read.sindex_id,
                &sindex_info, &sindex_uuid);
        } catch (const ql::exc_t &e) {
            res->results_or_error = e;
            return;
        }

        if (sindex_info.geo != sindex_geo_bool_t::GEO) {
            res->results_or_error = ql::exc_t(
                ql::base_exc_t::GENERIC,
                strprintf(
                    "Index `%s` is not a geospatial index.  get_nearest can only be "
                    "used with a geospatial index.",
                    geo_read.sindex_id.c_str()),
                NULL);
            return;
        }

        rdb_get_nearest_slice(
            store->get_sindex_slice(sindex_uuid),
            geo_read.center, geo_read.max_dist, geo_read.max_results, geo_read.geo_system,
            sindex_sb.get(), &ql_env,
            geo_read.region.inner, sindex_info, res);
    }

    void operator()(const rget_read_t &rget) {
        if (rget.transforms.size() != 0 || rget.terminal) {
            // This asserts that the optargs have been initialized.  (There is always
            // a 'db' optarg.)  We have the same assertion in
            // rdb_r_unshard_visitor_t.
            rassert(rget.optargs.size() != 0);
        }

        ql::env_t ql_env(ctx, interruptor, rget.optargs, trace);

        response->response = rget_read_response_t();
        rget_read_response_t *res =
            boost::get<rget_read_response_t>(&response->response);

        if (!rget.sindex) {
            // Normal rget
            rdb_rget_slice(btree, rget.region.inner, superblock,
                           &ql_env, rget.batchspec, rget.transforms, rget.terminal,
                           rget.sorting, res);
        } else {
            sindex_disk_info_t sindex_info;
            uuid_u sindex_uuid;
            scoped_ptr_t<real_superblock_t> sindex_sb;
            try {
                sindex_sb =
                    acquire_sindex_for_read(rget.table_name, rget.sindex->id,
                    &sindex_info, &sindex_uuid);
            } catch (const ql::exc_t &e) {
                res->result = e;
                return;
            }

            if (sindex_info.geo == sindex_geo_bool_t::GEO) {
                res->result = ql::exc_t(
                    ql::base_exc_t::GENERIC,
                    strprintf(
                        "Index `%s` is a geospatial index.  Only get_nearest and "
                        "get_intersecting can use a geospatial index.",
                        rget.sindex->id.c_str()),
                    NULL);
                return;
            }

            rdb_rget_secondary_slice(
                store->get_sindex_slice(sindex_uuid),
                rget.sindex->original_range, rget.sindex->region,
                sindex_sb.get(), &ql_env, rget.batchspec, rget.transforms,
                rget.terminal, rget.region.inner, rget.sorting,
                sindex_info, res);
        }
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

    void operator()(UNUSED const sindex_list_t &sinner) {
        response->response = sindex_list_response_t();
        sindex_list_response_t *res = &boost::get<sindex_list_response_t>(response->response);

        buf_lock_t sindex_block
            = store->acquire_sindex_block_for_read(superblock->expose_buf(),
                                                   superblock->get_sindex_block_id());
        superblock->release();

        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);
        sindex_block.reset_buf_lock();

        res->sindexes.reserve(sindexes.size());
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (!it->second.being_deleted) {
                guarantee(!it->first.being_deleted);
                res->sindexes.push_back(it->first.name);
            }
        }
    }

    void operator()(const sindex_status_t &sindex_status) {
        response->response = sindex_status_response_t();
        auto res = &boost::get<sindex_status_response_t>(response->response);

        buf_lock_t sindex_block
            = store->acquire_sindex_block_for_read(superblock->expose_buf(),
                                                   superblock->get_sindex_block_id());
        superblock->release();

        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);
        sindex_block.reset_buf_lock();

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (it->second.being_deleted) {
                guarantee(it->first.being_deleted);
                continue;
            }
            guarantee(!it->first.being_deleted);
            if (sindex_status.sindexes.find(it->first.name)
                    != sindex_status.sindexes.end()
                || sindex_status.sindexes.empty()) {
                rdb_protocol::single_sindex_status_t *s = &res->statuses[it->first.name];
                const std::vector<char> &vec = it->second.opaque_definition;
                s->func = std::string(&*vec.begin(), vec.size());
                progress_completion_fraction_t frac = store->get_progress(it->second.id);
                s->ready = it->second.is_ready();
                if (!s->ready) {
                    if (frac.estimate_of_total_nodes == -1) {
                        s->blocks_processed = 0;
                        s->blocks_total = 0;
                    } else {
                        s->blocks_processed = frac.estimate_of_released_nodes;
                        s->blocks_total = frac.estimate_of_total_nodes;
                    }
                }

                {
                    sindex_disk_info_t sindex_info;
                    deserialize_sindex_info(it->second.opaque_definition, &sindex_info);

                    s->geo = sindex_info.geo;
                    s->multi = sindex_info.multi;
                    s->outdated =
                        (sindex_info.mapping_version_info.latest_compatible_reql_version
                            != reql_version_t::LATEST);
                }
            }
        }
    }

    rdb_read_visitor_t(btree_slice_t *_btree,
                       store_t *_store,
                       superblock_t *_superblock,
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
        trace(_trace)
    {
    }

private:
    scoped_ptr_t<real_superblock_t> acquire_sindex_for_read(
            const std::string &table_name,
            const std::string &sindex_id,
            sindex_disk_info_t *sindex_info_out,
            uuid_u *sindex_uuid_out) {
        rassert(sindex_info_out != NULL);
        rassert(sindex_uuid_out != NULL);

        scoped_ptr_t<real_superblock_t> sindex_sb;
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
            if (!found) {
                throw ql::exc_t(
                    ql::base_exc_t::GENERIC,
                    strprintf("Index `%s` was not found on table `%s`.",
                              sindex_id.c_str(), table_name.c_str()),
                    NULL);
            }
        } catch (const sindex_not_ready_exc_t &e) {
            throw ql::exc_t(
                ql::base_exc_t::GENERIC, e.what(), NULL);
        }

        try {
            deserialize_sindex_info(sindex_mapping_data, sindex_info_out);
        } catch (const archive_exc_t &e) {
            crash("%s", e.what());
        }

        *sindex_uuid_out = sindex_uuid;
        return std::move(sindex_sb);
    }

    read_response_t *const response;
    rdb_context_t *const ctx;
    signal_t *const interruptor;
    btree_slice_t *const btree;
    store_t *const store;
    superblock_t *const superblock;
    profile::trace_t *const trace;

    DISABLE_COPYING(rdb_read_visitor_t);
};

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            superblock_t *superblock,
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
    counted_t<const ql::datum_t> replace(
        const counted_t<const ql::datum_t> &d, size_t) const {
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
    counted_t<const ql::datum_t> replace(const counted_t<const ql::datum_t> &d,
                                         size_t index) const {
        guarantee(index < datums->size());
        counted_t<const ql::datum_t> newd = (*datums)[index];
        if (d->get_type() == ql::datum_t::R_NULL) {
            return newd;
        } else if (conflict_behavior == conflict_behavior_t::REPLACE) {
            return newd;
        } else if (conflict_behavior == conflict_behavior_t::UPDATE) {
            return d->merge(newd);
        } else {
            rfail_target(d, ql::base_exc_t::GENERIC,
                         "Duplicate primary key `%s`:\n%s\n%s",
                         pkey.c_str(), d->print().c_str(),
                         newd->print().c_str());
        }
        unreachable();
    }
    return_changes_t should_return_changes() const { return return_changes; }
private:
    const std::vector<counted_t<const ql::datum_t> > *const datums;
    const conflict_behavior_t conflict_behavior;
    const std::string pkey;
    const return_changes_t return_changes;
};

struct rdb_write_visitor_t : public boost::static_visitor<void> {
    void operator()(const batched_replace_t &br) {
        ql::env_t ql_env(ctx, interruptor, br.optargs, trace);
        rdb_modification_report_cb_t sindex_cb(
            store, &sindex_block,
            auto_drainer_t::lock_t(&store->drainer));
        func_replacer_t replacer(&ql_env, br.f, br.return_changes);
        response->response =
            rdb_batched_replace(
                btree_info_t(btree, timestamp,
                             &br.pkey),
                superblock, br.keys, ql_env.limits(), &replacer, &sindex_cb,
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
            keys.emplace_back((*it)->get(bi.pkey)->print_primary());
        }
        response->response =
            rdb_batched_replace(
                btree_info_t(btree, timestamp,
                             &bi.pkey),
                superblock, keys, bi.limits, &replacer, &sindex_cb,
                trace);
    }

    void operator()(const point_write_t &w) {
        response->response = point_write_response_t();
        point_write_response_t *res =
            boost::get<point_write_response_t>(&response->response);

        rdb_live_deletion_context_t deletion_context;
        rdb_modification_report_t mod_report(w.key);
        rdb_set(w.key, w.data, w.overwrite, btree, timestamp, superblock->get(),
                &deletion_context, res, &mod_report.info, trace);

        update_sindexes(mod_report);
    }

    void operator()(const point_delete_t &d) {
        response->response = point_delete_response_t();
        point_delete_response_t *res =
            boost::get<point_delete_response_t>(&response->response);

        rdb_live_deletion_context_t deletion_context;
        rdb_modification_report_t mod_report(d.key);
        rdb_delete(d.key, btree, timestamp, superblock->get(), &deletion_context,
                res, &mod_report.info, trace);

        update_sindexes(mod_report);
    }

    void operator()(const sindex_create_t &c) {
        sindex_create_response_t res;

        write_message_t wm;
        sindex_disk_info_t info(c.mapping, sindex_reql_version_info_t::LATEST(),
                                c.multi, c.geo);
        serialize_sindex_info(&wm, info);

        vector_stream_t stream;
        stream.reserve(wm.size());
        int write_res = send_write_message(&stream, &wm);
        guarantee(write_res == 0);

        sindex_name_t name(c.id);
        res.success = store->add_sindex(
            name,
            stream.vector(),
            &sindex_block);

        if (res.success) {
            std::set<sindex_name_t> sindexes;
            sindexes.insert(name);
            rdb_protocol::bring_sindexes_up_to_date(
                sindexes, store, &sindex_block);
        }

        response->response = res;
    }

    void operator()(const sindex_drop_t &d) {
        sindex_drop_response_t res;
        res.success = store->drop_sindex(sindex_name_t(d.id), &sindex_block);
        response->response = res;
    }

    void operator()(const sindex_rename_t &r) {
        sindex_rename_response_t res;
        sindex_name_t old_name(r.old_name);
        sindex_name_t new_name(r.new_name);

        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);

        bool old_name_found = false;
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (it->first == old_name) {
                guarantee(!it->first.being_deleted);
                old_name_found = true;
            } else if (it->first == new_name && !r.overwrite) {
                guarantee(!it->first.being_deleted);
                res.result = sindex_rename_result_t::NEW_NAME_EXISTS;
                response->response = res;
                return;
            }
        }

        if (!old_name_found) {
            res.result = sindex_rename_result_t::OLD_NAME_DOESNT_EXIST;
        } else {
            if (r.old_name != r.new_name) {
                store->rename_sindex(old_name, new_name, &sindex_block);
            }
            res.result = sindex_rename_result_t::SUCCESS;
        }

        response->response = res;
    }

    void operator()(const sync_t &) {
        response->response = sync_response_t();

        // We know this sync_t operation will force all preceding write transactions
        // (on our cache_conn_t) to flush before or at the same time, because the
        // cache guarantees that.  (Right now it will force _all_ preceding write
        // transactions to flush, on any conn, because they all touch the metainfo in
        // the superblock.)
    }


    rdb_write_visitor_t(btree_slice_t *_btree,
                        store_t *_store,
                        txn_t *_txn,
                        scoped_ptr_t<superblock_t> *_superblock,
                        repli_timestamp_t _timestamp,
                        rdb_context_t *_ctx,
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
        trace(_trace) {
        sindex_block =
            store->acquire_sindex_block_for_write((*superblock)->expose_buf(),
                                                  (*superblock)->get_sindex_block_id());
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
    scoped_ptr_t<superblock_t> *const superblock;
    const repli_timestamp_t timestamp;
    profile::trace_t *const trace;
    buf_lock_t sindex_block;
    profile::event_log_t event_log_out;

    DISABLE_COPYING(rdb_write_visitor_t);
};

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             transition_timestamp_t timestamp,
                             scoped_ptr_t<superblock_t> *superblock,
                             signal_t *interruptor) {
    scoped_ptr_t<profile::trace_t> trace = ql::maybe_make_profile_trace(write.profile);

    {
        profile::starter_t start_write("Perform write on shard.", trace);
        rdb_write_visitor_t v(btree.get(),
                              this,
                              (*superblock)->expose_buf().txn(),
                              superblock,
                              timestamp.to_repli_timestamp(),
                              ctx,
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

struct rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t : public boost::static_visitor<repli_timestamp_t> {
    repli_timestamp_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return del.recency;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::delete_range_t &) {
        return repli_timestamp_t::invalid;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::key_value_pairs_t &kv) {
        repli_timestamp_t most_recent = repli_timestamp_t::invalid;
        rassert(!kv.backfill_atoms.empty());
        for (size_t i = 0; i < kv.backfill_atoms.size(); ++i) {
            if (most_recent == repli_timestamp_t::invalid
                || most_recent < kv.backfill_atoms[i].recency) {

                most_recent = kv.backfill_atoms[i].recency;
            }
        }
        return most_recent;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::sindexes_t &) {
        return repli_timestamp_t::invalid;
    }
};

repli_timestamp_t backfill_chunk_t::get_btree_repli_timestamp() const THROWS_NOTHING {
    rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct rdb_backfill_callback_impl_t : public rdb_backfill_callback_t {
public:
    typedef backfill_chunk_t chunk_t;

    explicit rdb_backfill_callback_impl_t(chunk_fun_callback_t *_chunk_fun_cb)
        : chunk_fun_cb(_chunk_fun_cb) { }
    ~rdb_backfill_callback_impl_t() { }

    void on_delete_range(const key_range_t &range,
                         signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_range(region_t(range)), interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency,
                     signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_key(to_store_key(key), recency),
                                 interruptor);
    }

    void on_keyvalues(std::vector<backfill_atom_t> &&atoms,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::set_keys(std::move(atoms)), interruptor);
    }

    void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes,
                     signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::sindexes(sindexes), interruptor);
    }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    chunk_fun_callback_t *chunk_fun_cb;

    DISABLE_COPYING(rdb_backfill_callback_impl_t);
};

void call_rdb_backfill(int i, btree_slice_t *btree,
                       const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
                       rdb_backfill_callback_t *callback,
                       superblock_t *superblock,
                       buf_lock_t *sindex_block,
                       traversal_progress_combiner_t *progress,
                       signal_t *interruptor) THROWS_NOTHING {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owned(p);
    progress->add_constituent(&p_owned);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        rdb_backfill(btree, regions[i].first.inner, timestamp, callback,
                     superblock, sindex_block, p, interruptor);
    } catch (const interrupted_exc_t &) {
        /* do nothing; `protocol_send_backfill()` will notice that interruptor
        has been pulsed */
    }
}

void store_t::protocol_send_backfill(const region_map_t<state_timestamp_t> &start_point,
                                     chunk_fun_callback_t *chunk_fun_cb,
                                     superblock_t *superblock,
                                     buf_lock_t *sindex_block,
                                     traversal_progress_combiner_t *progress,
                                     signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    with_priority_t p(CORO_PRIORITY_BACKFILL_SENDER);
    rdb_backfill_callback_impl_t callback(chunk_fun_cb);
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
    refcount_superblock_t refcount_wrapper(superblock, regions.size());
    pmap(regions.size(), std::bind(&call_rdb_backfill, ph::_1,
                                   btree.get(), regions, &callback,
                                   &refcount_wrapper, sindex_block, progress,
                                   interruptor));

    /* If interruptor was pulsed, `call_rdb_backfill()` exited silently, so we
    have to check directly. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

void backfill_chunk_single_rdb_set(const backfill_atom_t &bf_atom,
                                   btree_slice_t *btree, superblock_t *superblock,
                                   UNUSED auto_drainer_t::lock_t drainer_acq,
                                   rdb_modification_report_t *mod_report_out,
                                   promise_t<superblock_t *> *superblock_promise_out) {
    mod_report_out->primary_key = bf_atom.key;
    point_write_response_t response;
    rdb_live_deletion_context_t deletion_context;
    rdb_set(bf_atom.key, bf_atom.value, true,
            btree, bf_atom.recency, superblock, &deletion_context, &response,
            &mod_report_out->info, static_cast<profile::trace_t *>(NULL),
            superblock_promise_out);
}

struct rdb_receive_backfill_visitor_t : public boost::static_visitor<void> {
    rdb_receive_backfill_visitor_t(store_t *_store,
                                   btree_slice_t *_btree,
                                   txn_t *_txn,
                                   scoped_ptr_t<superblock_t> &&_superblock,
                                   signal_t *_interruptor) :
        store(_store), btree(_btree), txn(_txn), superblock(std::move(_superblock)),
        interruptor(_interruptor) {
        sindex_block =
            store->acquire_sindex_block_for_write(superblock->expose_buf(),
                                                  superblock->get_sindex_block_id());
    }

    void operator()(const backfill_chunk_t::delete_key_t &delete_key) {
        point_delete_response_t response;
        std::vector<rdb_modification_report_t> mod_reports(1);
        mod_reports[0].primary_key = delete_key.key;
        rdb_live_deletion_context_t deletion_context;
        rdb_delete(delete_key.key, btree, delete_key.recency,
                   superblock.get(), &deletion_context, &response,
                   &mod_reports[0].info, static_cast<profile::trace_t *>(NULL));
        superblock.reset();
        update_sindexes(mod_reports);
    }

    void operator()(const backfill_chunk_t::delete_range_t &delete_range) {
        rdb_protocol::range_key_tester_t tester(&delete_range.range);
        rdb_live_deletion_context_t deletion_context;
        std::vector<rdb_modification_report_t> mod_reports;
        rdb_erase_small_range(&tester, delete_range.range.inner,
                              superblock.get(), &deletion_context, interruptor,
                              &mod_reports);
        superblock.reset();
        if (!mod_reports.empty()) {
            update_sindexes(mod_reports);
        }
    }

    void operator()(const backfill_chunk_t::key_value_pairs_t &kv) {
        std::vector<rdb_modification_report_t> mod_reports(kv.backfill_atoms.size());
        {
            auto_drainer_t drainer;
            for (size_t i = 0; i < kv.backfill_atoms.size(); ++i) {
                promise_t<superblock_t *> superblock_promise;
                // `spawn_now_dangerously` so that we don't have to wait for the
                // superblock if it's immediately available.
                coro_t::spawn_now_dangerously(std::bind(&backfill_chunk_single_rdb_set,
                                                        kv.backfill_atoms[i], btree,
                                                        superblock.release(),
                                                        auto_drainer_t::lock_t(&drainer),
                                                        &mod_reports[i],
                                                        &superblock_promise));
                superblock.init(superblock_promise.wait());
            }
            superblock.reset();
        }
        update_sindexes(mod_reports);
    }

    void operator()(const backfill_chunk_t::sindexes_t &s) {
        // Release the superblock. We don't need it for this.
        superblock.reset();

        std::map<sindex_name_t, secondary_index_t> sindexes;
        for (auto it = s.sindexes.begin(); it != s.sindexes.end(); ++it) {
            secondary_index_t sindex = it->second;
            // backfill_chunk_t::sindexes_t contains hard-coded UUIDs for the
            // secondary indexes. This can cause problems if indexes are deleted
            // and recreated very quickly. The very reason for why we have UUIDs
            // in the sindexes is to avoid two post-constructions to interfere with
            // each other in such cases.
            // More information:
            // https://github.com/rethinkdb/rethinkdb/issues/657
            // https://github.com/rethinkdb/rethinkdb/issues/2087
            //
            // Assign new random UUIDs to the secondary indexes to avoid collisions
            // during post construction:
            sindex.id = generate_uuid();

            auto res = sindexes.insert(std::make_pair(sindex_name_t(it->first),
                                                      sindex));
            guarantee(res.second);
        }

        std::set<sindex_name_t> created_sindexes;
        store->set_sindexes(sindexes, &sindex_block, &created_sindexes);

        if (!created_sindexes.empty()) {
            rdb_protocol::bring_sindexes_up_to_date(created_sindexes, store,
                                                            &sindex_block);
        }
    }

private:
    void update_sindexes(const std::vector<rdb_modification_report_t> &mod_reports) {
        store->update_sindexes(txn,
                               &sindex_block,
                               mod_reports,
                               true /* release sindex block */);
    }

    store_t *store;
    btree_slice_t *btree;
    txn_t *txn;
    scoped_ptr_t<superblock_t> superblock;
    signal_t *interruptor;
    buf_lock_t sindex_block;

    DISABLE_COPYING(rdb_receive_backfill_visitor_t);
};

void store_t::protocol_receive_backfill(scoped_ptr_t<superblock_t> &&_superblock,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    scoped_ptr_t<superblock_t> superblock(std::move(_superblock));
    rdb_receive_backfill_visitor_t v(this, btree.get(),
                                     superblock->expose_buf().txn(),
                                     std::move(superblock),
                                     interruptor);
    boost::apply_visitor(v, chunk.val);
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
