#include <limits>
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/wire_func.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {
configured_limits_t
from_optargs(const std::map<std::string, wire_func_t> &arguments)
{
    auto p = arguments.find("arrayLimit");
    if (p != arguments.end()) {
        // Fake an environment with no arguments.  We have to fake it
        // because of a chicken/egg problem; this function gets called
        // before there are any extant environments at all.  Only
        // because we use an empty argument list do we prevent an
        // infinite loop.
        cond_t cond;
        rdb_context_t context;
        env_t env(&context, &cond, std::map<std::string, wire_func_t>(),
                  profile_bool_t::DONT_PROFILE);
        return configured_limits_t(p->second.compile_wire_func()->call(&env)->as_int());
    } else {
        return configured_limits_t();
    }
}

RDB_IMPL_ME_SERIALIZABLE_1(configured_limits_t, array_size_limit_);

const configured_limits_t configured_limits_t::unlimited(std::numeric_limits<size_t>::max());

} // namespace ql
