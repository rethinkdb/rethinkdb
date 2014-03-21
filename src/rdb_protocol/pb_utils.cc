#include "rdb_protocol/pb_utils.hpp"

#include "rdb_protocol/env.hpp"

namespace ql {
namespace pb {

sym_t dummy_var_to_sym(dummy_var_t dummy_var) {
    // dummy_var values are non-negative, we map them to small negative values.
    return sym_t(-1 - static_cast<int64_t>(dummy_var));
}

} // namespace pb
} // namespace ql
