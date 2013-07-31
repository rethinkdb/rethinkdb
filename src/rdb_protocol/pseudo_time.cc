#include <time.h>

#include "rdb_protocol/pseudo_time.hpp"

namespace ql {
namespace pseudo {

const char *const time_string = "TIME";
const char *const epoch_time_key = "epoch_time";
const char *const timezone_key = "timezone";

typedef boost::local_time::local_time_input_facet input_timefmt_t;
typedef boost::local_time::local_time_facet output_timefmt_t;
typedef boost::local_time::local_date_time time_t;
typedef boost::posix_time::ptime ptime_t;
typedef boost::posix_time::time_duration dur_t;
typedef boost::gregorian::date date_t;

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

const ptime_t raw_epoch(date_t(1970, 1, 1));
const boost::local_time::time_zone_ptr utc(
    new boost::local_time::posix_time_zone("UTC"));
const boost::local_time::local_date_time epoch(raw_epoch, utc);

// Boost's documentation on what errors may be thrown where is somewhat lacking,
// so this is probably a slight superset of what we actually need to handle.  We
// may also need to catch the following exceptions in the future, but I omitted
// them because we never seem to encounter them and they might catch legitimate
// non-boost exceptions as well.
// * std::out_of_range
// * std::invalid_argument
// * std::runtime_error
#define HANDLE_BOOST_ERRORS(TARGET)                                     \
      catch (const boost::gregorian::bad_year &e) {                     \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::gregorian::bad_month &e) {                    \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::gregorian::bad_day_of_month &e) {             \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::gregorian::bad_weekday &e) {                  \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::bad_offset &e) {                  \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::bad_adjustment &e) {              \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::time_label_invalid &e) {          \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::dst_not_valid &e) {               \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::ambiguous_result &e) {            \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::data_not_accessible &e) {         \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const boost::local_time::bad_field_count &e) {             \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    } catch (const std::ios_base::failure &e) {                         \
        rfail_target(TARGET, base_exc_t::GENERIC,                       \
                     "Error in time logic: %s.", e.what());             \
    }                                                                   \

// Produces a datum_exc_t instead
const datum_t dummy_datum;
#define HANDLE_BOOST_ERRORS_NO_TARGET HANDLE_BOOST_ERRORS(&dummy_datum)

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
            // TODO: Figure out why Boost doesn't like non-`:` UTC offsets in
            // %ZP, or else get %Q to parse properly.
            // } else if (tz.size() == 5) {
            //     return hours_valid(tz[1], tz[2]) && minutes_valid(tz[3], tz[4]);
        } else if (tz.size() == 6) {
            return (hours_valid(tz[1], tz[2])
                    && tz[3] == ':'
                    && minutes_valid(tz[4], tz[5]))
                || (hours_valid(tz[1], tz[2])
                    && tz[3] == ':'
                    && tz[4] == '-'
                    && minutes_valid('0', tz[5]));
        } else if (tz.size() == 7) {
            return hours_valid(tz[1], tz[2])
                && tz[3] == ':'
                && tz[4] == '-'
                && minutes_valid(tz[5], tz[6]);
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

counted_t<const datum_t> boost_to_time(time_t t, const rcheckable_t *target) {
    dur_t dur(t - epoch);
    double seconds = dur.total_microseconds() / 1000000.0;
    std::string tz = t.zone_as_posix_string();
    tz = sanitize_tz(tz, target);
    r_sanity_check(tz == "" || tz_valid(tz));
    return make_time(seconds, tz);
}

counted_t<const datum_t> iso8601_to_time(
    const std::string &s, const rcheckable_t *target) {
    try {
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
        return boost_to_time(t, target);
    } HANDLE_BOOST_ERRORS(target);
}

const int64_t sec_incr = INT_MAX;
void add_seconds_to_ptime(ptime_t *t, double raw_sec) {
    int64_t sec = raw_sec;
    int64_t microsec = (raw_sec * 1000000.0) - (sec * 1000000);

    // boost::posix_time::seconds doesn't like large numbers, and like any
    // mature library, it reacts by silently overflowing somehwere and producing
    // an incorrect date if you give it a number that it doesn't like.
    int sign = sec < 0 ? -1 : 1;
    sec *= sign;
    while (sec > 0) {
        int64_t diff = std::min(sec, sec_incr);
        sec -= diff;
        *t += boost::posix_time::seconds(diff * sign);
    }
    r_sanity_check(sec == 0);

    *t += boost::posix_time::microseconds(microsec);
}


time_t time_to_boost(counted_t<const datum_t> d) {
    double raw_sec = d->get(epoch_time_key)->as_num();
    ptime_t t(date_t(1970, 1, 1));
    add_seconds_to_ptime(&t, raw_sec);

    if (counted_t<const datum_t> tz = d->get(timezone_key, NOTHROW)) {
        boost::local_time::time_zone_ptr zone(
            new boost::local_time::posix_time_zone(tz->as_str()));
        return time_t(t, zone);
    } else {
        return time_t(t, utc);
    }
}


const std::locale tz_format =
    std::locale(std::locale::classic(), new output_timefmt_t("%Y-%m-%dT%H:%M:%S%F%Q"));
const std::locale no_tz_format =
    std::locale(std::locale::classic(), new output_timefmt_t("%Y-%m-%dT%H:%M:%S%F"));
std::string time_to_iso8601(counted_t<const datum_t> d) {
    try {
        std::ostringstream ss;
        if (counted_t<const datum_t> tz = d->get(timezone_key, NOTHROW)) {
            ss.imbue(tz_format);
        } else {
            ss.imbue(no_tz_format);
        }
        ss << time_to_boost(d);
        return ss.str();
    } HANDLE_BOOST_ERRORS_NO_TARGET;
}

double time_to_epoch_time(counted_t<const datum_t> d) {
    return d->get(epoch_time_key)->as_num();
}

counted_t<const datum_t> time_now() {
    try {
        ptime_t t = boost::posix_time::microsec_clock::universal_time();
        return make_time((t - raw_epoch).total_microseconds() / 1000000.0);
    } HANDLE_BOOST_ERRORS_NO_TARGET;
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
                    msg = strprintf("invalid timezone string `%s`",
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

counted_t<const datum_t> time_tz(counted_t<const datum_t> time) {
    r_sanity_check(time->is_pt(time_string));
    if (counted_t<const datum_t> tz = time->get(timezone_key, NOTHROW)) {
        return tz;
    } else {
        return make_counted<const datum_t>(datum_t::R_NULL);
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

counted_t<const datum_t> make_time(
    int year, int month, int day, int hours, int minutes, double seconds,
    std::string tz, const rcheckable_t *target) {
    try {
        ptime_t ptime(date_t(year, month, day), dur_t(hours, minutes, 0));
        add_seconds_to_ptime(&ptime, seconds);
        tz = sanitize_tz(tz, target);
        if (tz != "") {
            boost::local_time::time_zone_ptr zone(
                new boost::local_time::posix_time_zone(tz));
            return boost_to_time(time_t(ptime, zone), target);
        } else {
            return boost_to_time(time_t(ptime, utc), target);
        }
    } HANDLE_BOOST_ERRORS(target);
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

counted_t<const datum_t> time_sub(counted_t<const datum_t> time,
                                  counted_t<const datum_t> time_or_duration) {
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

double time_portion(counted_t<const datum_t> time, time_component_t c) {
    try {
        ptime_t ptime = time_to_boost(time).local_time();
        switch (c) {
        case YEAR: return ptime.date().year();
        case MONTH: return ptime.date().month();
        case DAY: return ptime.date().day();
        case DAY_OF_WEEK: {
            // We use the ISO 8601 convention which counts from 1 and starts with Monday.
            int d = ptime.date().day_of_week();
            return d == 0 ? 7 : d;
        } unreachable();
        case DAY_OF_YEAR: return ptime.date().day_of_year();
        case HOURS: return ptime.time_of_day().hours();
        case MINUTES: return ptime.time_of_day().minutes();
        case SECONDS: {
            dur_t dur = ptime.time_of_day();
            double microsec = dur.total_microseconds();
            double sec = dur.total_seconds();
            return dur.seconds() + ((microsec - sec * 1000000) / 1000000.0);
        } unreachable();
        default: unreachable();
        }
        unreachable();
    } HANDLE_BOOST_ERRORS_NO_TARGET;
}

time_t boost_date(time_t boost_time) {
    ptime_t ptime = boost_time.local_time();
    date_t d(ptime.date().year_month_day());
    return time_t(ptime_t(d), boost_time.zone());
}

counted_t<const datum_t> time_date(counted_t<const datum_t> time,
                                   const rcheckable_t *target) {
    try {
        return boost_to_time(boost_date(time_to_boost(time)), target);
    } HANDLE_BOOST_ERRORS(target);
}

counted_t<const datum_t> time_of_day(counted_t<const datum_t> time) {
    try {
        time_t boost_time = time_to_boost(time);
        double sec =
            (boost_time - boost_date(boost_time)).total_microseconds() / 1000000.0;
        return make_counted<const datum_t>(sec);
    } HANDLE_BOOST_ERRORS_NO_TARGET;
}

} //namespace pseudo
} //namespace ql
