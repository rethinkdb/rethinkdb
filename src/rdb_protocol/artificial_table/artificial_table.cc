// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/artificial_table.hpp"

#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/table_common.hpp"

/* Determines how many coroutines we spawn for a batched replace or insert. */
static const int max_parallel_ops = 10;

/* This is a wrapper around `backend->read_row()` that also performs sanity checking, to
help catch bugs in the backend. */
bool checked_read_row_from_backend(
        artificial_table_backend_t *backend,
        const ql::datum_t &pval,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    if (!backend->read_row(pval, interruptor, row_out, error_out)) {
        return false;
    }
#ifndef NDEBUG
    if (row_out->has()) {
        ql::datum_t pval2 = row_out->get_field(
            datum_string_t(backend->get_primary_key_name()), ql::NOTHROW);
        rassert(pval2.has());
        rassert(pval2 == pval);
    }
#endif
    return true;
}

artificial_table_t::artificial_table_t(artificial_table_backend_t *_backend) :
    backend(_backend), primary_key(backend->get_primary_key_name()) { }

const std::string &artificial_table_t::get_pkey() {
    return primary_key;
}

ql::datum_t artificial_table_t::read_row(ql::env_t *env,
        ql::datum_t pval, UNUSED bool use_outdated) {
    ql::datum_t row;
    std::string error;
    if (!checked_read_row_from_backend(backend, pval, env->interruptor, &row, &error)) {
        throw ql::datum_exc_t(ql::base_exc_t::GENERIC, error);
    }
    if (!row.has()) {
        row = ql::datum_t::null();
    }
    return row;
}

class artificial_table_datum_stream_t :
    public ql::eager_datum_stream_t
{
public:
    artificial_table_datum_stream_t(
            const ql::protob_t<const Backtrace> &bt_source,
            artificial_table_backend_t *_backend,
            std::vector<ql::datum_t> &&_primary_keys) :
        ql::eager_datum_stream_t(bt_source),
        backend(_backend), primary_keys(std::move(_primary_keys)), index(0) { }
private:
    ql::datum_t next(ql::env_t *env, const ql::batchspec_t &bs) {
        if (ops_to_do()) {
            return datum_stream_t::next(env, bs);
        }
        return next_impl(env);
    }

    ql::datum_t next_impl(ql::env_t *env) {
        ql::datum_t row;
        while (index != primary_keys.size() && !row.has()) {
            std::string error;
            if (!checked_read_row_from_backend(backend, primary_keys[index],
                    env->interruptor, &row, &error)) {
                rfail_target(this, ql::base_exc_t::GENERIC, "%s", error.c_str());
            }
            /* `row` might still be uninitialized, if the row was deleted between when
            the list of primary keys was generated and now. If this happens we will go
            through the loop repeatedly looking for the next element that hasn't been
            removed. */
            ++index;
        }
        return row;
    }

    std::vector<ql::datum_t> next_raw_batch(ql::env_t *env, const ql::batchspec_t &bs) {
        std::vector<ql::datum_t> v;
        ql::batcher_t batcher = bs.to_batcher();
        ql::datum_t d;
        while (d = next_impl(env), d.has()) {
            batcher.note_el(d);
            v.push_back(std::move(d));
            if (batcher.should_send_batch()) {
                break;
            }
        }
        return v;
    }

    bool is_exhausted() const {
        return index == primary_keys.size();
    }

    bool is_cfeed() const { return false; }
    bool is_array() { return false; }

    artificial_table_backend_t *backend;
    std::vector<ql::datum_t> primary_keys;
    size_t index;
};

counted_t<ql::datum_stream_t> artificial_table_t::read_all(
        ql::env_t *env,
        const std::string &get_all_sindex_id,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        const datum_range_t &range,
        sorting_t sorting,
        UNUSED bool use_outdated) {
    if (get_all_sindex_id != primary_key) {
        rfail_datum(ql::base_exc_t::GENERIC, "%s",
            error_message_index_not_found(get_all_sindex_id, table_name).c_str());
    }
    std::string error;

    /* Fetch the keys from the backend */
    std::vector<ql::datum_t> keys;
    if (!backend->read_all_primary_keys(env->interruptor, &keys, &error)) {
        throw ql::datum_exc_t(ql::base_exc_t::GENERIC, error);    
    }

    /* Apply range filter */
    if (!range.is_universe()) {
        std::vector<ql::datum_t> temp;
        for (const ql::datum_t &key : keys) {
            if (range.contains(reql_version_t::LATEST, key)) {
                temp.push_back(key);
            }
        }
        keys = std::move(temp);
    }

    /* Apply sorting */
    switch (sorting) {
        case sorting_t::UNORDERED:
            break;
        case sorting_t::ASCENDING:
            /* It's OK to use `std::sort()` instead of `std::stable_sort()` here because
            primary keys need to be unique. If we were to support secondary indexes on
            artificial tables, we would need to ensure that `read_all_primary_keys()`
            returns the keys in a deterministic order and then we would need to use a
            `std::stable_sort()` here. */
            std::sort(keys.begin(), keys.end(),
                [](const ql::datum_t &a,
                   const ql::datum_t &b) {
                    return a.compare_lt(reql_version_t::LATEST, b);
                });
            break;
        case sorting_t::DESCENDING:
            std::sort(keys.begin(), keys.end(),
                [](const ql::datum_t &a,
                   const ql::datum_t &b) {
                    return a.compare_gt(reql_version_t::LATEST, b);
                });
            break;
        default:
            unreachable();
    }

    return make_counted<artificial_table_datum_stream_t>(
        bt, backend, std::move(keys));
}

counted_t<ql::datum_stream_t> artificial_table_t::read_row_changes(
        UNUSED ql::env_t *env,
        UNUSED ql::datum_t pval,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED const std::string &table_name) {
    /* RSI(reql_admin): Artificial tables will eventually support change feeds. */
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables don't support changefeeds.");
}

counted_t<ql::datum_stream_t> artificial_table_t::read_all_changes(
        UNUSED ql::env_t *env,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED const std::string &table_name) {
    /* RSI(reql_admin): Artificial tables will eventually support change feeds. */
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables don't support changefeeds.");
}

counted_t<ql::datum_stream_t> artificial_table_t::read_intersecting(
        UNUSED ql::env_t *env,
        const std::string &sindex,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        UNUSED bool use_outdated,
        UNUSED const ql::datum_t &query_geometry) {
    guarantee(sindex != primary_key, "read_intersecting() should never be called with "
        "the primary index");
    rfail_datum(ql::base_exc_t::GENERIC, "%s",
        error_message_index_not_found(sindex, table_name).c_str());
}

ql::datum_t artificial_table_t::read_nearest(
        UNUSED ql::env_t *env,
        const std::string &sindex,
        const std::string &table_name,
        UNUSED bool use_outdated,
        UNUSED lon_lat_point_t center,
        UNUSED double max_dist,
        UNUSED uint64_t max_results,
        UNUSED const ellipsoid_spec_t &geo_system,
        UNUSED dist_unit_t dist_unit,
        UNUSED const ql::configured_limits_t &limits) {
    guarantee(sindex != primary_key, "read_nearest() should never be called with "
        "the primary index");
    rfail_datum(ql::base_exc_t::GENERIC, "%s",
        error_message_index_not_found(sindex, table_name).c_str());
}

ql::datum_t artificial_table_t::write_batched_replace(
        ql::env_t *env,
        const std::vector<ql::datum_t> &keys,
        const counted_t<const ql::func_t> &func,
        return_changes_t return_changes,
        UNUSED durability_requirement_t durability) {
    /* RSI(reql_admin): Should we require that durability is soft? */
    ql::datum_t stats = ql::datum_t::empty_object();
    std::set<std::string> conditions;

    throttled_pmap(keys.size(), [&] (int i) {
        try {
            do_single_update(env, keys[i], false,
                [&] (ql::datum_t old_row) {
                    return func->call(env, old_row, ql::LITERAL_OK)->as_datum();
                },
                return_changes, env->interruptor, &stats, &conditions);
        } catch (interrupted_exc_t) {
            /* don't throw since we're in throttled_pmap() */
        }
    }, max_parallel_ops);
    if (env->interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    ql::datum_object_builder_t obj_builder(stats);
    obj_builder.add_warnings(conditions, env->limits());
    return std::move(obj_builder).to_datum();
}

ql::datum_t artificial_table_t::write_batched_insert(
        ql::env_t *env,
        std::vector<ql::datum_t> &&inserts,
        std::vector<bool> &&pkey_was_autogenerated,
        conflict_behavior_t conflict_behavior,
        return_changes_t return_changes,
        UNUSED durability_requirement_t durability) {
    ql::datum_t stats = ql::datum_t::empty_object();
    std::set<std::string> conditions;
    throttled_pmap(inserts.size(), [&] (int i) {
        try {
            ql::datum_t insert_row = inserts[i];
            ql::datum_t key = insert_row.get_field(
                datum_string_t(primary_key), ql::NOTHROW);
            guarantee(key.has(), "write_batched_insert() shouldn't ever be called with "
                "documents that lack a primary key.");

            do_single_update(env, key, pkey_was_autogenerated[i],
                [&](ql::datum_t old_row) {
                    return resolve_insert_conflict(
                        primary_key, old_row, insert_row, conflict_behavior);
                },
                return_changes, env->interruptor, &stats, &conditions);

        } catch (interrupted_exc_t) {
            /* don't throw since we're in throttled_pmap() */
        }
    }, max_parallel_ops);
    if (env->interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    ql::datum_object_builder_t obj_builder(stats);
    obj_builder.add_warnings(conditions, env->limits());
    return std::move(obj_builder).to_datum();
}

bool artificial_table_t::write_sync_depending_on_durability(
        UNUSED ql::env_t *env,
        UNUSED durability_requirement_t durability) {
    /* Calling `sync()` on an artificial table is a meaningful operation; it would mean
    to flush the metadata to disk. But it would be a lot of trouble to implement in
    practice, so we don't. */
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables don't support `sync()`.");
}

bool artificial_table_t::sindex_create(
        UNUSED ql::env_t *env, UNUSED const std::string &id,
        UNUSED counted_t<const ql::func_t> index_func, UNUSED sindex_multi_bool_t multi,
        UNUSED sindex_geo_bool_t geo) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Can't create a secondary index on an artificial table.");
}

bool artificial_table_t::sindex_drop(UNUSED ql::env_t *env,
        UNUSED const std::string &id) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Can't drop a secondary index on an artificial table.");
}

sindex_rename_result_t artificial_table_t::sindex_rename(
        UNUSED ql::env_t *env,
        UNUSED const std::string &old_name,
        UNUSED const std::string &new_name,
        UNUSED bool overwrite) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Can't rename a secondary index on an artificial table.");
}

std::vector<std::string> artificial_table_t::sindex_list(UNUSED ql::env_t *env) {
    return std::vector<std::string>();
}

std::map<std::string, ql::datum_t> artificial_table_t::sindex_status(
        UNUSED ql::env_t *env, UNUSED const std::set<std::string> &sindexes) {
    return std::map<std::string, ql::datum_t>();
}

void artificial_table_t::do_single_update(
        ql::env_t *env,
        ql::datum_t pval,
        bool pkey_was_autogenerated,
        const std::function<ql::datum_t(ql::datum_t)>
            &function,
        return_changes_t return_changes,
        signal_t *interruptor,
        ql::datum_t *stats_inout,
        std::set<std::string> *conditions_inout) {
    std::string error;
    ql::datum_t old_row;
    if (!checked_read_row_from_backend(backend, pval, interruptor, &old_row, &error)) {
        ql::datum_object_builder_t builder;
        builder.add_error(error.c_str());
        *stats_inout = (*stats_inout).merge(
            std::move(builder).to_datum(), ql::stats_merge, env->limits(),
            conditions_inout);
        return;
    }
    if (!old_row.has()) {
        old_row = ql::datum_t::null();
    }

    ql::datum_t resp;
    try {
        ql::datum_t new_row = function(old_row);
        bool was_changed;
        resp = make_row_replacement_stats(
            datum_string_t(primary_key), store_key_t(pval.print_primary()),
            old_row, new_row, return_changes, &was_changed);
        if (was_changed) {
            if (new_row.get_type() == ql::datum_t::R_NULL) {
                new_row.reset();
            }
            if (!backend->write_row(pval, pkey_was_autogenerated, new_row,
                    interruptor, &error)) {
                rfail_datum(ql::base_exc_t::GENERIC, "%s", error.c_str());
            }
        }
    } catch (const ql::base_exc_t &e) {
        resp = make_row_replacement_error_stats(
            old_row, return_changes, e.what());
    }
    *stats_inout = (*stats_inout).merge(
        resp, ql::stats_merge, env->limits(), conditions_inout);
}

