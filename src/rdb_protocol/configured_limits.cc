#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/wire_func.hpp"

namespace ql {
configured_limits_t
from_optargs(const std::map<std::string, wire_func_t> &)
{
    return configured_limits_t();
}

} // namespace ql
