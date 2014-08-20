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

const std::string &real_table_t::get_pkey() {
    return pkey;
}

counted_t<const ql::datum_t> real_table_t::read_row(ql::env_t *env,
        counted_t<const ql::datum_t> pval, bool use_outdated) {
    read_t read(point_read_t(store_key_t(pval->print_primary())), env->profile());
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
        const datum_range_t &range,
        sorting_t sorting,
        bool use_outdated) {
    if (sindex == get_pkey()) {
        return make_counted<ql::lazy_datum_stream_t>(
            *this,
            use_outdated,
            ql::primary_readgen_t::make(env, table_name, range, sorting),
            bt);
    } else {
        return make_counted<ql::lazy_datum_stream_t>(
            *this,
            use_outdated,
            ql::sindex_readgen_t::make(
                env, table_name, sindex, range, sorting),
            bt);
    }
}

counted_t<ql::datum_stream_t> real_table_t::read_row_changes(
        ql::env_t *env,
        counted_t<const ql::datum_t> pval,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name) {
    return changefeed_client->new_feed(env, uuid, bt, table_name, pkey,
        ql::changefeed::keyspec_t(ql::changefeed::keyspec_t::point_t(std::move(pval))));
}

counted_t<ql::datum_stream_t> real_table_t::read_all_changes(ql::env_t *env,
        const ql::protob_t<const Backtrace> &bt, const std::string &table_name) {
    return changefeed_client->new_feed(env, uuid, bt, table_name, pkey,
        ql::changefeed::keyspec_t(ql::changefeed::keyspec_t::all_t()));
}

counted_t<ql::datum_stream_t> real_table_t::read_intersecting(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        bool use_outdated,
        const counted_t<const ql::datum_t> &query_geometry) {

    intersecting_geo_read_t geo_read(
        query_geometry, table_name, sindex, env->get_all_optargs());
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

    intersecting_geo_read_response_t *g_res =
        boost::get<intersecting_geo_read_response_t>(&res.response);
    r_sanity_check(g_res);

    ql::exc_t *error = boost::get<ql::exc_t>(&g_res->results_or_error);
    if (error != NULL) {
        throw *error;
    }

    auto *result = boost::get<counted_t<const ql::datum_t> >(&g_res->results_or_error);
    guarantee(result != NULL);
    return make_counted<ql::array_datum_stream_t>(*result, bt);
}

counted_t<ql::datum_stream_t> real_table_t::read_nearest(
        ql::env_t *env,
        const std::string &sindex,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,
        bool use_outdated,
        lat_lon_point_t center,
        double max_dist,
        uint64_t max_results,
        const ellipsoid_spec_t &geo_system,
        dist_unit_t dist_unit,
        const ql::configured_limits_t &limits) {

    nearest_geo_read_t geo_read(
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
        dup = one_result.add("dist", make_counted<ql::datum_t>(converted_dist));
        r_sanity_check(!dup);
        dup = one_result.add("doc", (*result)[i].second);
        r_sanity_check(!dup);
        formatted_result.add(std::move(one_result).to_counted());
    }
    return make_counted<ql::array_datum_stream_t>(
        std::move(formatted_result).to_counted(), bt);
}

counted_t<const ql::datum_t> real_table_t::write_batched_replace(ql::env_t *env,
        const std::vector<counted_t<const ql::datum_t> > &keys,
        const counted_t<const ql::func_t> &func,
        return_changes_t return_changes, durability_requirement_t durability) {
    std::vector<store_key_t> store_keys;
    store_keys.reserve(keys.size());
    for (auto it = keys.begin(); it != keys.end(); it++) {
        store_keys.push_back(store_key_t((*it)->print_primary()));
    }
    batched_replace_t write(std::move(store_keys), pkey, func,
            env->get_all_optargs(), return_changes);
    write_t w(std::move(write), durability, env->profile(), env->limits());
    write_response_t response;
    write_with_profile(env, &w, &response);
    auto dp = boost::get<counted_t<const ql::datum_t> >(&response.response);
    r_sanity_check(dp != NULL);
    return *dp;
}

counted_t<const ql::datum_t> real_table_t::write_batched_insert(ql::env_t *env,
        std::vector<counted_t<const ql::datum_t> > &&inserts,
        conflict_behavior_t conflict_behavior, return_changes_t return_changes,
        durability_requirement_t durability) {
    batched_insert_t write(std::move(inserts), pkey, conflict_behavior, env->limits(),
        return_changes);
    write_t w(std::move(write), durability, env->profile(), env->limits());
    write_response_t response;
    write_with_profile(env, &w, &response);
    auto dp = boost::get<counted_t<const ql::datum_t> >(&response.response);
    r_sanity_check(dp != NULL);
    return *dp;
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

std::vector<std::string> real_table_t::sindex_list(ql::env_t *env) {
    sindex_list_t sindex_list;
    read_t read(sindex_list, env->profile());
    read_response_t res;
    read_with_profile(env, read, &res, false);
    sindex_list_response_t *s_res =
        boost::get<sindex_list_response_t>(&res.response);
    r_sanity_check(s_res);
    return s_res->sindexes;
}

std::map<std::string, counted_t<const ql::datum_t> >
real_table_t::sindex_status(ql::env_t *env, const std::set<std::string> &sindexes) {
    sindex_status_t sindex_status(sindexes);
    read_t read(sindex_status, env->profile());
    read_response_t res;
    read_with_profile(env, read, &res, false);
    auto s_res = boost::get<sindex_status_response_t>(&res.response);
    r_sanity_check(s_res);
    std::map<std::string, counted_t<const ql::datum_t> > statuses;
    for (const std::pair<std::string, rdb_protocol::single_sindex_status_t> &pair :
            s_res->statuses) {
        std::map<std::string, counted_t<const ql::datum_t> > status;
        if (pair.second.blocks_processed != 0) {
            status["blocks_processed"] =
                make_counted<const ql::datum_t>(
                    safe_to_double(pair.second.blocks_processed));
            status["blocks_total"] =
                make_counted<const ql::datum_t>(
                    safe_to_double(pair.second.blocks_total));
        }
        status["ready"] = ql::datum_t::boolean(pair.second.ready);
        std::string s = sindex_blob_prefix + pair.second.func;
        status["function"] = ql::datum_t::binary(
            wire_string_t::create_and_init(s.size(), s.data()));
        status["outdated"] = ql::datum_t::boolean(pair.second.outdated);
        status["multi"] = ql::datum_t::boolean(pair.second.multi ==
                                               sindex_multi_bool_t::MULTI);
        status["geo"] = ql::datum_t::boolean(pair.second.geo ==
                                             sindex_geo_bool_t::GEO);
        statuses.insert(std::make_pair(
            pair.first,
            make_counted<const ql::datum_t>(std::move(status))));
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

