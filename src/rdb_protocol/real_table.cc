// Copyright 2010-2014 RethinkDB, all rights reserved
#include "rdb_protocol/real_table.hpp"

#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/protocol.hpp"

namespace_interface_access_t::namespace_interface_access_t() :
    nif(NULL), ref_tracker(NULL), thread(INVALID_THREAD)
    { }

namespace_interface_access_t::namespace_interface_access_t(
        const namespace_interface_access_t& access) :
    nif(access.nif), ref_tracker(access.ref_tracker), thread(access.thread)
{
    if (ref_tracker != NULL) {
        rassert(get_thread_id() == thread);
        ref_tracker->add_ref();
    }
}

namespace_interface_access_t::namespace_interface_access_t(namespace_interface_t *_nif,
        ref_tracker_t *_ref_tracker, threadnum_t _thread) :
    nif(_nif), ref_tracker(_ref_tracker), thread(_thread)
{
    if (ref_tracker != NULL) {
        rassert(get_thread_id() == thread);
        ref_tracker->add_ref();
    }
}

namespace_interface_access_t &namespace_interface_access_t::operator=(
        const namespace_interface_access_t &access) {
    if (this != &access) {
        if (ref_tracker != NULL) {
            rassert(get_thread_id() == thread);
            ref_tracker->release();
        }
        nif = access.nif;
        ref_tracker = access.ref_tracker;
        thread = access.thread;
        if (ref_tracker != NULL) {
            rassert(get_thread_id() == thread);
            ref_tracker->add_ref();
        }
    }
    return *this;
}

namespace_interface_access_t::~namespace_interface_access_t() {
    if (ref_tracker != NULL) {
        rassert(get_thread_id() == thread);
        ref_tracker->release();
    }
}

namespace_interface_t *namespace_interface_access_t::get() {
    rassert(thread == get_thread_id());
    return nif;
}

ql::datum_t real_table_t::get_id() const {
    return ql::datum_t(datum_string_t(uuid_to_str(uuid)));
}

const std::string &real_table_t::get_pkey() const {
    return pkey;
}

ql::datum_t real_table_t::read_row(ql::env_t *env,
        ql::datum_t pval, bool use_outdated) {
    read_t read(point_read_t(store_key_t(pval.print_primary())), env->profile());
    read_response_t res;
    read_with_profile(env, read, &res, use_outdated);
    point_read_response_t *p_res = boost::get<point_read_response_t>(&res.response);
    r_sanity_check(p_res);
    return p_res->data;
}

counted_t<ql::datum_stream_t> real_table_t::read_all(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        const ql::datum_range_t &range,
        sorting_t sorting,
        bool use_outdated) {
    if (sindex == get_pkey()) {
        return make_counted<ql::lazy_datum_stream_t>(
            make_scoped<ql::rget_reader_t>(
                counted_t<real_table_t>(this),
                use_outdated,
                ql::primary_readgen_t::make(env, table_name, range, sorting)),
            bt);
    } else {
        return make_counted<ql::lazy_datum_stream_t>(
            make_scoped<ql::rget_reader_t>(
                counted_t<real_table_t>(this),
                use_outdated,
                ql::sindex_readgen_t::make(
                    env, table_name, sindex, range, sorting)),
            bt);
    }
}

counted_t<ql::datum_stream_t> real_table_t::read_changes(
    ql::env_t *env,
    const ql::datum_t &squash,
    ql::changefeed::keyspec_t::spec_t &&spec,
    const ql::protob_t<const Backtrace> &bt,
    const std::string &table_name) {
    return changefeed_client->new_stream(
        env, squash, uuid, bt, table_name, std::move(spec));
}

counted_t<ql::datum_stream_t> real_table_t::read_intersecting(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        bool use_outdated,
        const ql::datum_t &query_geometry) {

    return make_counted<ql::lazy_datum_stream_t>(
        make_scoped<ql::intersecting_reader_t>(
            counted_t<real_table_t>(this),
            use_outdated,
            ql::intersecting_readgen_t::make(
                env, table_name, sindex, query_geometry)),
        bt);
}

ql::datum_t real_table_t::read_nearest(
        ql::env_t *env,
        const std::string &sindex,
        const std::string &table_name,
        bool use_outdated,
        lon_lat_point_t center,
        double max_dist,
        uint64_t max_results,
        const ellipsoid_spec_t &geo_system,
        dist_unit_t dist_unit,
        const ql::configured_limits_t &limits) {

    nearest_geo_read_t geo_read(
        region_t::universe(),
        center, max_dist, max_results, geo_system, table_name, sindex,
        env->get_all_optargs());
    read_t read(geo_read, env->profile());
    read_response_t res;
    try {
        if (use_outdated) {
            namespace_access.get()->read_outdated(read, &res, env->interruptor);
        } else {
            namespace_access.get()->read(
                read, &res, order_token_t::ignore, env->interruptor);
        }
    } catch (const cannot_perform_query_exc_t &ex) {
        rfail_datum(ql::base_exc_t::GENERIC, "Cannot perform read: %s", ex.what());
    }

    nearest_geo_read_response_t *g_res =
        boost::get<nearest_geo_read_response_t>(&res.response);
    r_sanity_check(g_res);

    ql::exc_t *error = boost::get<ql::exc_t>(&g_res->results_or_error);
    if (error != NULL) {
        throw *error;
    }

    auto *result =
        boost::get<nearest_geo_read_response_t::result_t>(&g_res->results_or_error);
    guarantee(result != NULL);

    // Generate the final output, converting distance units on the way.
    ql::datum_array_builder_t formatted_result(limits);
    for (size_t i = 0; i < result->size(); ++i) {
        ql::datum_object_builder_t one_result;
        const double converted_dist =
            convert_dist_unit((*result)[i].first, dist_unit_t::M, dist_unit);
        bool dup;
        dup = one_result.add("dist", ql::datum_t(converted_dist));
        r_sanity_check(!dup);
        dup = one_result.add("doc", (*result)[i].second);
        r_sanity_check(!dup);
        formatted_result.add(std::move(one_result).to_datum());
    }
    return std::move(formatted_result).to_datum();
}

const size_t split_size = 128;
template<class T>
std::vector<std::vector<T> > split(std::vector<T> &&v) {
    std::vector<std::vector<T> > out;
    out.reserve(v.size() / split_size + (v.size() % split_size != 0));
    size_t i = 0;
    while (i < v.size()) {
        size_t step = split_size, keys_left = v.size() - i;
        if (step > keys_left) {
            step = keys_left;
        } else if (step < keys_left && keys_left < 2 * step) {
            // Be less absurd if we have e.g. `split_size + 1` elements.
            step = keys_left / 2;
        }
        guarantee(step != 0);

        std::vector<T> batch;
        batch.reserve(step);
        std::move(v.begin() + i, v.begin() + i + step, std::back_inserter(batch));
        i += step;
        out.push_back(std::move(batch));
    }
    guarantee(i == v.size());
    return std::move(out);
}

ql::datum_t real_table_t::write_batched_replace(
    ql::env_t *env,
    const std::vector<ql::datum_t> &keys,
    const counted_t<const ql::func_t> &func,
    return_changes_t return_changes,
    durability_requirement_t durability) {

    std::vector<store_key_t> store_keys;
    store_keys.reserve(keys.size());
    for (auto it = keys.begin(); it != keys.end(); it++) {
        store_keys.push_back(store_key_t((*it).print_primary()));
    }

    ql::datum_t stats((std::map<datum_string_t, ql::datum_t>()));
    std::set<std::string> conditions;
    std::vector<std::vector<store_key_t> > batches = split(std::move(store_keys));
    for (auto &&batch : batches) {
        batched_replace_t write(
            std::move(batch), pkey, func, env->get_all_optargs(), return_changes);
        write_t w(std::move(write), durability, env->profile(), env->limits());
        write_response_t response;
        write_with_profile(env, &w, &response);
        auto dp = boost::get<ql::datum_t>(&response.response);
        r_sanity_check(dp != NULL);
        stats = stats.merge(*dp, ql::stats_merge, env->limits(), &conditions);
    }
    ql::datum_object_builder_t result(stats);
    result.add_warnings(conditions, env->limits());
    return std::move(result).to_datum();
}

ql::datum_t real_table_t::write_batched_insert(
    ql::env_t *env,
    std::vector<ql::datum_t> &&inserts,
    UNUSED std::vector<bool> &&pkey_is_autogenerated,
    conflict_behavior_t conflict_behavior,
    return_changes_t return_changes,
    durability_requirement_t durability) {

    ql::datum_t stats((std::map<datum_string_t, ql::datum_t>()));
    std::set<std::string> conditions;
    std::vector<std::vector<ql::datum_t> > batches = split(std::move(inserts));
    for (auto &&batch : batches) {
        batched_insert_t write(
            std::move(batch), pkey, conflict_behavior, env->limits(), return_changes);
        write_t w(std::move(write), durability, env->profile(), env->limits());
        write_response_t response;
        write_with_profile(env, &w, &response);
        auto dp = boost::get<ql::datum_t>(&response.response);
        r_sanity_check(dp != NULL);
        stats = stats.merge(*dp, ql::stats_merge, env->limits(), &conditions);
    }
    ql::datum_object_builder_t result(stats);
    result.add_warnings(conditions, env->limits());
    return std::move(result).to_datum();
}

bool real_table_t::write_sync_depending_on_durability(ql::env_t *env,
        durability_requirement_t durability) {
    write_t write(sync_t(), durability, env->profile(), env->limits());
    write_response_t res;
    write_with_profile(env, &write, &res);
    sync_response_t *response = boost::get<sync_response_t>(&res.response);
    r_sanity_check(response);
    return true; // With our current implementation, a sync can never fail.
}

bool real_table_t::sindex_create(ql::env_t *env, const std::string &id,
        counted_t<const ql::func_t> index_func, sindex_multi_bool_t multi,
        sindex_geo_bool_t geo) {
    ql::map_wire_func_t wire_func(index_func);
    write_t write(sindex_create_t(id, wire_func, multi, geo), env->profile(),
                  env->limits());
    write_response_t res;
    write_with_profile(env, &write, &res);
    sindex_create_response_t *response =
        boost::get<sindex_create_response_t>(&res.response);
    r_sanity_check(response);
    return response->success;
}

bool real_table_t::sindex_drop(ql::env_t *env, const std::string &id) {
    write_t write(sindex_drop_t(id), env->profile(), env->limits());
    write_response_t res;
    write_with_profile(env, &write, &res);
    sindex_drop_response_t *response =
        boost::get<sindex_drop_response_t>(&res.response);
    r_sanity_check(response);
    return response->success;
}

sindex_rename_result_t real_table_t::sindex_rename(ql::env_t *env,
                                                   const std::string &old_name,
                                                   const std::string &new_name,
                                                   bool overwrite) {
    write_t write(sindex_rename_t(old_name, new_name, overwrite),
                  env->profile(),
                  env->limits());
    write_response_t res;
    write_with_profile(env, &write, &res);
    sindex_rename_response_t *response =
        boost::get<sindex_rename_response_t>(&res.response);
    r_sanity_check(response);
    return response->result;
}

std::vector<std::string> real_table_t::sindex_list(ql::env_t *env, bool use_outdated) {
    sindex_list_t sindex_list;
    read_t read(sindex_list, env->profile());
    read_response_t res;
    read_with_profile(env, read, &res, use_outdated);
    sindex_list_response_t *s_res =
        boost::get<sindex_list_response_t>(&res.response);
    r_sanity_check(s_res);
    return s_res->sindexes;
}

std::map<std::string, ql::datum_t>
real_table_t::sindex_status(ql::env_t *env, const std::set<std::string> &sindexes) {
    sindex_status_t sindex_status(sindexes);
    read_t read(sindex_status, env->profile());
    read_response_t res;
    read_with_profile(env, read, &res, false);
    auto s_res = boost::get<sindex_status_response_t>(&res.response);
    r_sanity_check(s_res);
    std::map<std::string, ql::datum_t> statuses;
    for (const std::pair<std::string, rdb_protocol::single_sindex_status_t> &pair :
            s_res->statuses) {
        std::map<datum_string_t, ql::datum_t> status;
        if (pair.second.blocks_processed != 0) {
            status[datum_string_t("blocks_processed")] =
                ql::datum_t(
                    safe_to_double(pair.second.blocks_processed));
            status[datum_string_t("blocks_total")] =
                ql::datum_t(
                    safe_to_double(pair.second.blocks_total));
        }
        status[datum_string_t("ready")] = ql::datum_t::boolean(pair.second.ready);
        std::string s = sindex_blob_prefix + pair.second.func;
        status[datum_string_t("function")] = ql::datum_t::binary(
            datum_string_t(s.size(), s.data()));
        status[datum_string_t("outdated")] = ql::datum_t::boolean(pair.second.outdated);
        status[datum_string_t("multi")] =
            ql::datum_t::boolean(pair.second.multi == sindex_multi_bool_t::MULTI);
        status[datum_string_t("geo")] =
            ql::datum_t::boolean(pair.second.geo == sindex_geo_bool_t::GEO);
        statuses.insert(std::make_pair(
            pair.first,
            ql::datum_t(std::move(status))));
    }

    return statuses;
}

void real_table_t::read_with_profile(ql::env_t *env, const read_t &read,
        read_response_t *response, bool outdated) {
    profile::starter_t starter(
        (outdated ? "Perform outdated read." : "Perform read."),
        env->trace);
    profile::splitter_t splitter(env->trace);
    /* propagate whether or not we're doing profiles */
    r_sanity_check(read.profile == env->profile());
    /* Do the actual read. */
    try {
        if (!outdated) {
            namespace_access.get()->read(read, response, order_token_t::ignore,
                env->interruptor);
        } else {
            namespace_access.get()->read_outdated(read, response, env->interruptor);
        }
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC, "Cannot perform read: %s", e.what());
    }
    /* Append the results of the profile to the current task */
    splitter.give_splits(response->n_shards, response->event_log);
}

void real_table_t::write_with_profile(ql::env_t *env, write_t *write,
        write_response_t *response) {
    profile::starter_t starter("Perform write", env->trace);
    profile::splitter_t splitter(env->trace);
    /* propagate whether or not we're doing profiles */
    write->profile = env->profile();
    /* Do the actual write. */
    try {
        namespace_access.get()->write(*write, response, order_token_t::ignore,
            env->interruptor);
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC, "Cannot perform write: %s", e.what());
    }
    /* Append the results of the profile to the current task */
    splitter.give_splits(response->n_shards, response->event_log);
}

