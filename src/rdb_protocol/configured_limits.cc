// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/configured_limits.hpp"

#include <limits>
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/wire_func.hpp"

namespace ql {

configured_limits_t from_optargs(
    rdb_context_t *ctx, signal_t *interruptor, global_optargs_t *args) {
    size_t changefeed_queue_size = configured_limits_t::default_changefeed_queue_size;
    size_t array_size_limit = configured_limits_t::default_array_size_limit;
    // Fake an environment with no arguments.  We have to fake it
    // because of a chicken/egg problem; this function gets called
    // before there are any extant environments at all.  Only
    // because we use an empty argument list do we prevent an
    // infinite loop.
    // TODO: there's a `env_t(interruptor, reql_version)` constructor,
    // should this be using that?
    if (args != nullptr) {
        bool has_changefeed_queue_size = args->has_optarg("changefeed_queue_size");
        bool has_array_limit = args->has_optarg("array_limit");
        if (has_changefeed_queue_size || has_array_limit) {
            env_t env(ctx,
                      return_empty_normal_batches_t::NO,
                      interruptor,
                      global_optargs_t(),
                      nullptr);
            if (has_changefeed_queue_size) {
                int64_t sz = args->get_optarg(&env, "changefeed_queue_size")->as_int();
                changefeed_queue_size = check_limit("changefeed queue size", sz);
            }
            if (has_array_limit) {
                int64_t limit = args->get_optarg(&env, "array_limit")->as_int();
                array_size_limit = check_limit("array size limit", limit);
            }
        }
    }
    return configured_limits_t(changefeed_queue_size, array_size_limit);
}

size_t check_limit(const char *name, int64_t limit) {
    rcheck_datum(
        limit >= 1, base_exc_t::LOGIC,
        strprintf("Illegal %s `%" PRIi64 "`.  (Must be >= 1.)", name, limit));
    return limit;
}

RDB_IMPL_SERIALIZABLE_2(configured_limits_t, changefeed_queue_size_, array_size_limit_);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(configured_limits_t);

const configured_limits_t configured_limits_t::unlimited(
    std::numeric_limits<size_t>::max(),
    std::numeric_limits<size_t>::max());

} // namespace ql
