#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/wire_func.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {
configured_limits_t
from_optargs(const std::map<std::string, wire_func_t> &arguments)
{
    auto p = arguments.find("arrayLimit");
    if (p != arguments.end()) {
        return configured_limits_t((*p).second.compile_wire_func()->call(NULL)->as_int());
    } else {
        return configured_limits_t();
    }
}

} // namespace ql
