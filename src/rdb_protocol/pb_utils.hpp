#ifndef RDB_PROTOCOL_PB_UTILS_HPP_
#define RDB_PROTOCOL_PB_UTILS_HPP_

#include <string>
#include <vector>

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/sym.hpp"

namespace ql {
namespace pb {


// Dummy variables are used to identify the variables we use when constructing
// home-made reql functions.
//
// You aren't perfectly safe though -- what if you put your variable inside a term that
// expands its body into a reql function!  So, we use a bunch of different dummy
// variable names for use in different types.
enum class dummy_var_t {
    IGNORED,  // For functions that ignore their parameter.  There's no special meaning
              // to this value, we just don't need to create separate names for these
              // instances.
    VAL_UPSERT_REPLACEMENT,
    GROUPBY_MAP_OBJ,
    GROUPBY_MAP_ATTR,
    GROUPBY_REDUCE_A,
    GROUPBY_REDUCE_B,
    GROUPBY_FINAL_OBJ,
    GROUPBY_FINAL_VAL,
    INNERJOIN_N,
    INNERJOIN_M,
    OUTERJOIN_N,
    OUTERJOIN_M,
    OUTERJOIN_LST,
    EQJOIN_ROW,
    EQJOIN_V,
    UPDATE_OLDROW,
    UPDATE_NEWROW,
    DIFFERENCE_ROW,
    SINDEXCREATE_X,
    OBJORSEQ_VARNUM,
    FUNC_GETFIELD,
    FUNC_PLUCK,
    FUNC_EQCOMPARISON,
    FUNC_PAGE,
    DISTINCT_ROW,
    EXTREME_SELECTION_ROW
};

// Don't use this!  The minidriver uses this.  Returns the sym_t corresponding to a
// dummy_var_t.
sym_t dummy_var_to_sym(dummy_var_t dummy_var);

} // namespace pb
} // namespace ql

#endif // RDB_PROTOCOL_PB_UTILS_HPP_
