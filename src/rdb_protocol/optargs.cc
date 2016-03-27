// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/optargs.hpp"

#include <set>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/val.hpp"
#include "rpc/serialize_macros.hpp"

namespace ql {

global_optargs_t::global_optargs_t() { }

void global_optargs_t::add_optarg(const raw_term_t &optarg, const std::string &name) {
    rcheck_toplevel(optarg_is_valid(name), base_exc_t::LOGIC,
        strprintf("Unrecognized global optional argument `%s`.", name.c_str()));

    auto res = optargs.insert(std::make_pair(std::string(name),
                                             wire_func_t(optarg, std::vector<sym_t>())));

    rcheck_toplevel(res.second, base_exc_t::LOGIC,
        strprintf("Duplicate global optional argument: `%s`.", name.c_str()));
}

bool global_optargs_t::has_optarg(const std::string &key) const {
    return optargs.count(key) > 0;
}

scoped_ptr_t<val_t> global_optargs_t::get_optarg(env_t *env, const std::string &key) {
    auto it = optargs.find(key);
    if (it == optargs.end()) {
        return scoped_ptr_t<val_t>();
    }
    return it->second.compile_wire_func()->call(env);
}

static const std::set<std::string> acceptable_optargs({
    "_EVAL_FLAGS_",
    "_NO_RECURSE_",
    "_SHORTCUT_",
    "array_limit",
    "attempts",
    "auth",
    "base",
    "binary_format",
    "changefeed_queue_size",
    "conflict",
    "data",
    "db",
    "default",
    "default_timezone",
    "dry_run",
    "durability",
    "emergency_repair",
    "emit",
    "fill",
    "final_emit",
    "first_batch_scaledown_factor",
    "float",
    "geo",
    "geo_system",
    "group_format",
    "header",
    "identifier_format",
    "include_initial",
    "include_offsets",
    "include_states",
    "include_types",
    "index",
    "interleave",
    "ordered",
    "left_bound",
    "max_batch_bytes",
    "max_batch_rows",
    "max_batch_seconds",
    "max_dist",
    "max_results",
    "method",
    "min_batch_rows",
    "multi",
    "non_atomic",
    "nonvoting_replica_tags",
    "noreply",
    "num_vertices",
    "overwrite",
    "page",
    "page_limit",
    "params",
    "primary_key",
    "primary_replica_tag",
    "profile",
    "read_mode",
    "redirects",
    "replicas",
    "result_format",
    "return_changes",
    "return_vals",
    "right_bound",
    "shards",
    "squash",
    "time_format",
    "timeout",
    "unit",
    "use_outdated", // Only so we can detect it and error.
    "verify",
    "wait_for",
});

bool global_optargs_t::optarg_is_valid(const std::string &key) {
    return acceptable_optargs.count(key) != 0;
}

RDB_IMPL_SERIALIZABLE_1(global_optargs_t, optargs);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(global_optargs_t);

} // namespace ql
