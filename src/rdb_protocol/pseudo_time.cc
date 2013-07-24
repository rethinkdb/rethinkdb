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
    if (tz == "UTC+00" || tz == "") {
        return make_time(seconds);
    } else {
        rcheck_target(target, base_exc_t::GENERIC,
                      tz[0] == '-' || tz[0] == '+'
                      || (tz[0] == 'Z' && (tz == "Z" || tz[1] == '-' || tz[1] == '+')),
                      strprintf("Invalid ISO 8601 timezone: `%s`.", tz.c_str()));
        return make_time(seconds, tz);
    }
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

int time_cmp(const datum_t &x, const datum_t &y) {
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
    if (x->is_pt(time_string)) {
        time = x;
        duration = y;
    } else {
        r_sanity_check(y->is_pt(time_string));
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
    r_sanity_check(time->is_pt(time_string));

    scoped_ptr_t<datum_t> res;
    if (time_or_duration->is_pt(time_string)) {
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
