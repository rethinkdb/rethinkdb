#include "rdb_protocol/terms/terms.hpp"


#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class error_term_t : public op_term_t {
public:
    error_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(0, 1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        if (num_args() == 0) {
            rfail(base_exc_t::EMPTY_USER, "Empty ERROR term outside a default block.");
        } else {
            rfail(base_exc_t::GENERIC, "%s", arg(0)->as_str().c_str());
        }
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
        counted_t<const datum_t> func_arg;
        scoped_ptr_t<exc_t> err;
        counted_t<val_t> v;
        try {
            v = arg(0);
            if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
                func_arg = v->as_datum();
                if (func_arg->get_type() != datum_t::R_NULL) {
                    return v;
                }
            } else {
                return v;
            }
        } catch (const exc_t &e) {
            if (e.get_type() == base_exc_t::NON_EXISTENCE) {
                err.init(new exc_t(e));
                func_arg = make_counted<const datum_t>(e.what());
            } else {
                throw;
            }
        } catch (const datum_exc_t &e) {
            if (e.get_type() == base_exc_t::NON_EXISTENCE) {
                err.init(new exc_t(e.get_type(), e.what(), backtrace().get()));
                func_arg = make_counted<const datum_t>(e.what());
            } else {
                throw;
            }
        }
        r_sanity_check(func_arg.has());
        r_sanity_check(func_arg->get_type() == datum_t::R_NULL
                       || func_arg->get_type() == datum_t::R_STR);
        try {
            counted_t<val_t> def = arg(1);
            if (def->get_type().is_convertible(val_t::type_t::FUNC)) {
                return def->as_func()->call(func_arg);
            } else {
                return def;
            }
        } catch (const base_exc_t &e) {
            if (e.get_type() == base_exc_t::EMPTY_USER) {
                if (err.has()) {
                    throw *err;
                } else {
                    r_sanity_check(func_arg->get_type() == datum_t::R_NULL);
                    return v;
                }
            } else {
                throw;
            }
        }
    }
    virtual const char *name() const { return "error"; }
};

counted_t<term_t> make_error_term(env_t *env, protob_t<const Term> term) {
    return make_counted<error_term_t>(env, term);
}
counted_t<term_t> make_default_term(env_t *env, protob_t<const Term> term) {
    return make_counted<default_term_t>(env, term);
}


} //namespace ql
