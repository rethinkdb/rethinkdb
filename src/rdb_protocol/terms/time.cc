#include <iostream>

#include "errors.hpp"
#include <boost/date_time.hpp>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "rdb_protocol/pseudo_time.hpp"

namespace ql {

typedef boost::local_time::local_time_input_facet timefmt_t;
typedef boost::local_time::local_date_time time_t;

// This is the complete set of accepted formats by my reading of the ISO 8601
// spec.  I would be absolutely astonished if it contained no errors or
// omissions.
const std::locale formats[] = {
    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H:%M:%S%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H:%M:%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H:%M:%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H:%M:%S%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H%M%S%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H%M%S%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H%M%s%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H%M%S%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H:%M%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H:%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H:%M%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H%M%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H%M%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H%M%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%dT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%dT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%mT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YT%H%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-W%VT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%VT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%uT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%uT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%jT%H%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%jT%H%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-%m-%d%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%m%d%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%m%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%ZP")),

    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-W%V-%u%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%YW%V%u%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y-%j%ZP")),
    std::locale(std::locale::classic(), new timefmt_t("%Y%j%ZP")),
};
const size_t num_formats = sizeof(formats)/sizeof(formats[0]);

const boost::posix_time::ptime raw_epoch(boost::gregorian::date(1970, 1, 1));
const boost::local_time::time_zone_ptr utc(
    new boost::local_time::posix_time_zone("UTC"));
const boost::local_time::local_date_time epoch(raw_epoch, utc);

class iso8601_term_t : public op_term_t {
public:
    iso8601_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl() {
        time_t t = parse_time(arg(0)->as_str());
        std::string data = arg(0)->as_str();
        boost::posix_time::time_duration dur(t - epoch);
        double seconds = dur.total_microseconds() / 1000000.0;
        std::string tz = t.zone_as_posix_string();
        if (tz == "UTC+00" || tz == "") {
            return new_val(pseudo::make_time(seconds));
        } else {
            rcheck(tz[0] == '-' || tz[0] == '+'
                   || (tz[0] == 'Z' && (tz == "Z" || tz[1] == '-' || tz[1] == '+')),
                   base_exc_t::GENERIC,
                   strprintf("Invalid ISO 8601 timezone: `%s`.", tz.c_str()));
            return new_val(pseudo::make_time(seconds, tz));
        }
    }

    time_t parse_time(const std::string &s) {
        time_t t(boost::date_time::not_a_date_time);
        for (size_t i = 0; i < num_formats; ++i) {
            std::istringstream ss(s);
            ss.imbue(formats[i]);
            ss >> t;
            if (t != time_t(boost::date_time::not_a_date_time)) {
                return t;
            }
        }

        rfail(base_exc_t::GENERIC,
              "Failed to parse `%s` as ISO 8601 time.", s.c_str());
    }

    virtual const char *name() const { return "iso8601"; }
};

class to_iso8601_term_t : public op_term_t {
public:
    to_iso8601_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl() {
        counted_t<const datum_t> t = arg(0)->as_pt(pseudo::time_string);
        return new_val(make_counted<const datum_t>(
                           pseudo::time_to_iso8601(arg(0)->as_pt(pseudo::time_string))));
    }
    virtual const char *name() const { return "to_iso8601"; }
};

counted_t<term_t> make_iso8601_term(env_t *env, protob_t<const Term> term) {
    return make_counted<iso8601_term_t>(env, term);
}
counted_t<term_t> make_to_iso8601_term(env_t *env, protob_t<const Term> term) {
    return make_counted<to_iso8601_term_t>(env, term);
}

} //namespace ql

