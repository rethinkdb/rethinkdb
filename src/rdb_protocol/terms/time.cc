#include <iostream>

#include "errors.hpp"
#include <boost/date_time.hpp>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "rdb_protocol/pseudo_time.hpp"

namespace ql {

class iso8601_term_t : public op_term_t {
public:
    iso8601_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        return new_val(pseudo::iso8601_to_time(v->as_str(), v.get()));
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

