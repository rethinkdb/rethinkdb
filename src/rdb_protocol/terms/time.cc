#include <iostream>

#include "errors.hpp"
#include <boost/date_time.hpp>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "rdb_protocol/time.hpp"

namespace ql {

using boost::posix_time::time_input_facet;
using boost::posix_time::ptime;
using boost::posix_time::time_duration;

const std::locale formats[] = {
std::locale(std::locale::classic(),new time_input_facet("%Y-%m-%d %H:%M:%S")),
std::locale(std::locale::classic(),new time_input_facet("%Y/%m/%d %H:%M:%S")),
std::locale(std::locale::classic(),new time_input_facet("%d.%m.%Y %H:%M:%S")),
std::locale(std::locale::classic(),new time_input_facet("%Y-%m-%d"))};
const size_t nformats = sizeof(formats)/sizeof(formats[0]);

const ptime epoch(boost::gregorian::date(1970,1,1));

std::time_t seconds_from_epoch(const std::string& s) {
    ptime time;
    for(size_t i = 0; i < nformats; ++i) {
        std::istringstream is(s);
        is.imbue(formats[i]);
        is >> time;
        if(time != ptime()) break;
    }
    return (time - epoch).ticks() / time_duration::rep_type::ticks_per_second;
}

using boost::posix_time::ptime;


class iso8601_term_t : public op_term_t {
public:
    iso8601_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }

private:
    counted_t<val_t> eval_impl() {
        std::string data = arg(0)->as_str();
        double seconds = static_cast<double>(seconds_from_epoch(data));
        return new_val(pseudo::make_time(seconds));
    }

    bool is_deterministic_impl() const {
        return true;
    }
    virtual const char *name() const { return "iso8601"; }
};

counted_t<term_t> make_iso8601_term(env_t *env, protob_t<const Term> term) {
    return make_counted<iso8601_term_t>(env, term);
}
} //namespace ql

