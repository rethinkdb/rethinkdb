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
    //TODO better validation for timezones.
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

counted_t<const datum_t> make_time(double epoch_time, std::string tz) {
    scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_OBJECT));
    datum_t::add_txn_t txn(res.get());
    bool clobber = res->add(datum_t::reql_type_string,
                            make_counted<const datum_t>(time_string), &txn);
    clobber |= res->add(epoch_time_key, make_counted<const datum_t>(epoch_time), &txn);
    if (tz != "") {
        clobber |= res->add(timezone_key, make_counted<const datum_t>(tz), &txn);
    }
    r_sanity_check(!clobber);
    return counted_t<const datum_t>(res.release());
}

counted_t<const datum_t> time_add(counted_t<const datum_t> x, counted_t<const datum_t> y) {
    counted_t<const datum_t> time, duration;
    if (x->is_pseudotype(time_string)) {
        time = x;
        duration = y;
    } else {
        r_sanity_check(y->is_pseudotype(time_string));
        time = y;
        duration = x;
    }

    scoped_ptr_t<datum_t> res(new datum_t(time->as_object()));
    bool clobbered = res->add(
        epoch_time_key,
        make_counted<const datum_t>(res->get(epoch_time_key)->as_num() +
                                    duration->as_num()),
        NULL, CLOBBER);
    r_sanity_check(clobbered);

    return counted_t<const datum_t>(res.release());
}

counted_t<const datum_t> time_sub(counted_t<const datum_t> time, counted_t<const datum_t> time_or_duration) {
    r_sanity_check(time->is_pseudotype(time_string));

    scoped_ptr_t<datum_t> res;
    if (time_or_duration->is_pseudotype(time_string)) {
        res.init(new datum_t(time->get(epoch_time_key)->as_num() -
                             time_or_duration->get(epoch_time_key)->as_num()));
    } else {
        res.init(new datum_t(time->as_object()));
        bool clobbered = res->add(
            epoch_time_key,
            make_counted<const datum_t>(res->get(epoch_time_key)->as_num() -
                                        time_or_duration->as_num()),
            NULL, CLOBBER);
        r_sanity_check(clobbered);
    }

    return counted_t<const datum_t>(res.release());
}

} //namespace pseudo
} //namespace ql
