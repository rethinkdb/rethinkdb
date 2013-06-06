#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"

namespace ql {

class datum_term_t : public term_t {
public:
    datum_term_t(env_t *env, protob_t<const Term> t)
        : term_t(env, t),
          raw_val(new_val(make_counted<const datum_t>(&t->datum(), env))) {
        guarantee(raw_val.has());
    }
private:
    virtual bool is_deterministic_impl() const { return true; }
    virtual counted_t<val_t> eval_impl() { return raw_val; }
    virtual const char *name() const { return "datum"; }
    counted_t<val_t> raw_val;
};

class make_array_term_t : public op_term_t {
public:
    make_array_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        scoped_ptr_t<datum_t> acc(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < num_args(); ++i) {
            acc->add(arg(i)->as_datum());
        }
        return new_val(counted_t<const datum_t>(acc.release()));
    }
    virtual const char *name() const { return "make_array"; }
};

class make_obj_term_t : public op_term_t {
public:
    make_obj_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(0), optargspec_t::make_object()) { }
private:
    virtual counted_t<val_t> eval_impl() {
        scoped_ptr_t<datum_t> acc(new datum_t(datum_t::R_OBJECT));
        for (auto it = optargs.begin(); it != optargs.end(); ++it) {
            bool dup = acc->add(it->first, it->second->eval()->as_datum());
            rcheck(!dup, base_exc_t::GENERIC,
                   strprintf("Duplicate key in object: %s.", it->first.c_str()));
        }
        return new_val(counted_t<const datum_t>(acc.release()));
    }
    virtual const char *name() const { return "make_obj"; }
};

counted_t<term_t> make_datum_term(env_t *env, protob_t<const Term> term) {
    return make_counted<datum_term_t>(env, term);
}
counted_t<term_t> make_make_array_term(env_t *env, protob_t<const Term> term) {
    return make_counted<make_array_term_t>(env, term);
}
counted_t<term_t> make_make_obj_term(env_t *env, protob_t<const Term> term) {
    return make_counted<make_obj_term_t>(env, term);
}

} // namespace ql
