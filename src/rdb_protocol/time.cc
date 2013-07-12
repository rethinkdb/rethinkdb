#include "rdb_protocol/time.hpp"

namespace ql {
namespace pseudo {

const char *const time_string = "TIME";

const char *const epoch_time_key = "epoch_time";
const char *const timezone_key = "timezone";

int time_cmp(const datum_t& x, const datum_t& y) {
    r_sanity_check(x.get_reql_type() == time_string);
    r_sanity_check(y.get_reql_type() == time_string);
    return x.get(epoch_time_key)->cmp(*y.get(epoch_time_key));
}

} //namespace pseudo
} //namespace ql 
