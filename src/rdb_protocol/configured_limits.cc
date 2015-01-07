#include "rdb_protocol/configured_limits.hpp"
#include <limits>
#include "rdb_protocol/wire_func.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {
configured_limits_t
from_optargs(rdb_context_t *ctx, signal_t *interruptor, global_optargs_t *arguments)
{
    if (arguments->has_optarg("array_limit")) {
        // Fake an environment with no arguments.  We have to fake it
        // because of a chicken/egg problem; this function gets called
        // before there are any extant environments at all.  Only
        // because we use an empty argument list do we prevent an
        // infinite loop.
        env_t env(ctx, interruptor, std::map<std::string, wire_func_t>(),
                  nullptr);
        int64_t limit = arguments->get_optarg(&env, "array_limit")->as_int();
        rcheck_datum(limit > 1, base_exc_t::GENERIC,
                     strprintf("Illegal array size limit `%" PRIi64 "`.", limit));
        return configured_limits_t(limit);
    } else {
        return configured_limits_t();
    }
}

RDB_IMPL_SERIALIZABLE_1(configured_limits_t, array_size_limit_);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(configured_limits_t);

const configured_limits_t configured_limits_t::unlimited(std::numeric_limits<size_t>::max());

} // namespace ql
