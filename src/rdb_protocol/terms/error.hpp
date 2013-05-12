#ifndef RDB_PROTOCOL_TERMS_ERROR_HPP_
#define RDB_PROTOCOL_TERMS_ERROR_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class error_term_t : public op_term_t {
public:
    error_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        rfail("%s", arg(0)->as_str().c_str());
        unreachable();
    }
    virtual const char *name() const { return "error"; }
};

class default_term_t : public op_term_t {
public:
    default_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<func_t> f = arg(1)->as_func(IDENTITY_SHORTCUT);
        counted_t<val_t> v;
        try {
            v = arg(0);
        } catch (const base_exc_t &e) {
            return f->call(make_counted<const datum_t>(e.what()));
        }
        r_sanity_check(v.has());
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> d = v->as_datum();
            if (d->get_type() == datum_t::R_NULL) {
                return f->call(d);
            }
        }
        return v;
    }
    virtual const char *name() const { return "error"; }
};

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_ERROR_HPP_
