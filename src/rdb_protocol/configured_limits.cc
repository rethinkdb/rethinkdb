#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/wire_func.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {
configured_limits_t
from_optargs(const std::map<std::string, wire_func_t> &arguments)
{
    auto p = arguments.find("arrayLimit");
    if (p != arguments.end()) {
        // Fake an environment with no arguments.
        cond_t cond;
        rdb_context_t context;
        // Can't use the original arguments, because it will cause an infinite loop.
        env_t env(&context, &cond, std::map<std::string, wire_func_t>(),
                  profile_bool_t::DONT_PROFILE);
        return configured_limits_t((*p).second.compile_wire_func()->call(&env)->as_int());
    } else {
        return configured_limits_t();
    }
}

} // namespace ql
