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

bool time_valid(const datum_t &time) {
    r_sanity_check(time.get_reql_type() == time_string);
    bool has_epoch_time = false;
    for (auto it = time.as_object().begin(); it != time.as_object().end(); ++it) {
        if (it->first == epoch_time_key) {
            has_epoch_time = true;
        } else if (it->first == timezone_key || it->first == datum_t::reql_type_string) {
            continue;
        } else {
            return false;
        }
    }
    return has_epoch_time;
}

} //namespace pseudo
} //namespace ql 
