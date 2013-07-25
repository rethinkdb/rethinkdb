#include "rdb_protocol/pseudo_time.hpp"

namespace ql {
namespace pseudo {

const char *const time_string = "TIME";
const char *const epoch_time_key = "epoch_time";
const char *const timezone_key = "timezone";

typedef boost::local_time::local_time_input_facet input_timefmt_t;
typedef boost::local_time::local_time_facet output_timefmt_t;
typedef boost::local_time::local_date_time time_t;

// This is the complete set of accepted formats by my reading of the ISO 8601
// spec.  I would be absolutely astonished if it contained no errors or
// omissions.
const std::locale input_formats[] = {
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H:%M:%S%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H:%M:%S%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H%M%S%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H%M%S%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H:%M%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H:%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H:%M%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H%M%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H%M%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H%M%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%dT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%dT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%mT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YT%H%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%VT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%VT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%uT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%uT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%jT%H%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%jT%H%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m-%d%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%m%d%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%m%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%ZP")),

    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-W%V-%u%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%YW%V%u%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y-%j%ZP")),
    std::locale(std::locale::classic(), new input_timefmt_t("%Y%j%ZP")),
};
const size_t num_input_formats = sizeof(input_formats)/sizeof(input_formats[0]);

const boost::posix_time::ptime raw_epoch(boost::gregorian::date(1970, 1, 1));
const boost::local_time::time_zone_ptr utc(
    new boost::local_time::posix_time_zone("UTC"));
const boost::local_time::local_date_time epoch(raw_epoch, utc);


bool hours_valid(char l, char r) {
    return ((l == '0' || l == '1') && ('0' <= r && r <= '9'))
        || ((l == '2') && ('0' <= r && r <= '4'));
}
bool minutes_valid(char l, char r) {
    return ('0' <= l && l <= '5') && ('0' <= r && r <= '9');
}
bool tz_valid(const std::string &tz) {
    if (tz == "") {
        return false;
    } else if (tz == "Z") {
        return true;
    } else if (tz[0] == '+' || tz[0] == '-') {
        if (tz.size() == 3) {
            return hours_valid(tz[1], tz[2]);
        } else if (tz.size() == 5) {
            return hours_valid(tz[1], tz[2]) && minutes_valid(tz[3], tz[4]);
        } else if (tz.size() == 6) {
            return hours_valid(tz[1], tz[2])
                && tz[3] == ':'
                && minutes_valid(tz[4], tz[5]);
        }
    }
    return false;
}

std::string sanitize_tz(const std::string &tz, const rcheckable_t *target) {
    if (tz == "UTC+00" || tz == "") {
        return "";
    } else if (tz == "Z+00") {
        return "Z";
    } else if (tz_valid(tz)) {
        return tz;
    }
    // TODO: FIX
    rfail_target(target, base_exc_t::GENERIC,
                 "Invalid ISO 8601 timezone: `%s`.", tz.c_str());
}

counted_t<const datum_t> iso8601_to_time(
    const std::string &s, const rcheckable_t *target) {
    time_t t(boost::date_time::not_a_date_time);
    for (size_t i = 0; i < num_input_formats; ++i) {
        std::istringstream ss(s);
        ss.imbue(input_formats[i]);
        ss >> t;
        if (t != time_t(boost::date_time::not_a_date_time)) {
            break;
        }
    }
    rcheck_target(target, base_exc_t::GENERIC,
                  t != time_t(boost::date_time::not_a_date_time),
                  strprintf("Failed to parse `%s` as ISO 8601 time.", s.c_str()));

    boost::posix_time::time_duration dur(t - epoch);
    double seconds = dur.total_microseconds() / 1000000.0;
    std::string tz = t.zone_as_posix_string();
    tz = sanitize_tz(tz, target);
    r_sanity_check(tz == "" || tz_valid(tz));
    return make_time(seconds, tz);
}

const std::locale tz_format =
    std::locale(std::locale::classic(), new output_timefmt_t("%Y-%m-%dT%H:%M:%S%F%Q"));
const std::locale no_tz_format =
    std::locale(std::locale::classic(), new output_timefmt_t("%Y-%m-%dT%H:%M:%S%F"));
std::string time_to_iso8601(counted_t<const datum_t> d) {
    double raw_sec = d->get(epoch_time_key)->as_num();
    double sec = double(int64_t(raw_sec));
    double microsec = (raw_sec * 1000000) - (sec * 1000000);
    std::ostringstream ss;
    boost::posix_time::ptime t(boost::gregorian::date(1970, 1, 1));
    t += boost::posix_time::seconds(sec);
    t += boost::posix_time::microseconds(microsec);
    if (counted_t<const datum_t> tz = d->get(timezone_key, NOTHROW)) {
        ss.imbue(tz_format);
        boost::local_time::time_zone_ptr zone(
            new boost::local_time::posix_time_zone(tz->as_str()));
        ss << time_t(t, zone);
    } else {
        ss.imbue(no_tz_format);
        ss << time_t(t, utc);
    }
    return ss.str();
}

double time_to_epoch_time(counted_t<const datum_t> d) {
    return d->get(epoch_time_key)->as_num();
}

counted_t<const datum_t> time_now() {
    boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
    return make_time((t - raw_epoch).total_microseconds() / 1000000.0);
}

int time_cmp(const datum_t &x, const datum_t &y) {
    r_sanity_check(x.is_pt(time_string));
    r_sanity_check(y.is_pt(time_string));
    return x.get(epoch_time_key)->cmp(*y.get(epoch_time_key));
}

void rcheck_time_valid(const datum_t *time) {
    r_sanity_check(time != NULL);
    r_sanity_check(time->is_pt(time_string));
    std::string msg;
    bool has_epoch_time = false;
    for (auto it = time->as_object().begin(); it != time->as_object().end(); ++it) {
        if (it->first == epoch_time_key) {
            if (it->second->get_type() == datum_t::R_NUM) {
                has_epoch_time = true;
                continue;
            } else {
                msg = strprintf("field `%s` must be a number (got `%s` of type %s)",
                                epoch_time_key, it->second->trunc_print().c_str(),
                                it->second->get_type_name().c_str());
                break;
            }
        } else if (it->first == timezone_key) {
            if (it->second->get_type() == datum_t::R_STR) {
                if (tz_valid(it->second->as_str())) {
                    continue;
                } else {
                    msg = strprintf("invalide timezone string `%s`",
                                    it->second->trunc_print().c_str());
                    break;
                }
            } else {
                msg = strprintf("field `%s` must be a string (got `%s` of type %s)",
                                timezone_key, it->second->trunc_print().c_str(),
                                it->second->get_type_name().c_str());
                break;
            }
        } else if (it->first == datum_t::reql_type_string) {
            continue;
        } else {
            msg = strprintf("unrecognized field `%s`", it->first.c_str());
            break;
        }
    }

    if (msg == "" && !has_epoch_time) {
        msg = strprintf("no field `%s`", epoch_time_key);
    }

    if (msg != "") {
        rfail_target(time, base_exc_t::GENERIC,
                     "Invalid time object constructed (%s):\n%s",
                     msg.c_str(), time->trunc_print().c_str());
    }
}

counted_t<const datum_t> time_in_tz(counted_t<const datum_t> t,
                                    counted_t<const datum_t> tz) {
    r_sanity_check(t->is_pt(time_string));
    datum_ptr_t t2(t->as_object());
    if (tz->get_type() == datum_t::R_NULL) {
        UNUSED bool b = t2.delete_field(timezone_key);
    } else {
        UNUSED bool b = t2.add(timezone_key, tz, CLOBBER);
    }
    return t2.to_counted();
}

counted_t<const datum_t> make_time(double epoch_time, std::string tz) {
    datum_ptr_t res(datum_t::R_OBJECT);
    bool clobber = res.add(datum_t::reql_type_string,
                           make_counted<const datum_t>(time_string));
    clobber |= res.add(epoch_time_key, make_counted<const datum_t>(epoch_time));
    if (tz != "") {
        clobber |= res.add(timezone_key, make_counted<const datum_t>(tz));
    }
    r_sanity_check(!clobber);
    return res.to_counted();
}

counted_t<const datum_t> time_add(counted_t<const datum_t> x,
                                  counted_t<const datum_t> y) {
    counted_t<const datum_t> time, duration;
    if (x->is_pt(time_string)) {
        time = x;
        duration = y;
    } else {
        r_sanity_check(y->is_pt(time_string));
        time = y;
        duration = x;
    }

    datum_ptr_t res(time->as_object());
    bool clobbered = res.add(
        epoch_time_key,
        make_counted<const datum_t>(res->get(epoch_time_key)->as_num() +
                                    duration->as_num()),
        CLOBBER);
    r_sanity_check(clobbered);

    return res.to_counted();
}

counted_t<const datum_t> time_sub(counted_t<const datum_t> time, counted_t<const datum_t> time_or_duration) {
    r_sanity_check(time->is_pt(time_string));

    if (time_or_duration->is_pt(time_string)) {
        return make_counted<const datum_t>(
            time->get(epoch_time_key)->as_num()
            - time_or_duration->get(epoch_time_key)->as_num());
    } else {
        datum_ptr_t res(time->as_object());
        bool clobbered = res.add(
            epoch_time_key,
            make_counted<const datum_t>(res->get(epoch_time_key)->as_num() -
                                        time_or_duration->as_num()),
            CLOBBER);
        r_sanity_check(clobbered);
        return res.to_counted();
    }
}

} //namespace pseudo
} //namespace ql
