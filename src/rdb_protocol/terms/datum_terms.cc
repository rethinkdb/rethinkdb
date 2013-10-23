// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"

namespace ql {

class datum_term_t : public term_t {
public:
    explicit datum_term_t(protob_t<const Term> t)
        : term_t(t), raw_val(new_val(make_counted<const datum_t>(&t->datum()))) { }
private:
    virtual void accumulate_captures(var_captures_t *) const { /* do nothing */ }
    virtual bool is_deterministic() const { return true; }
    virtual counted_t<val_t> eval_impl(scope_env_t *, UNUSED eval_flags_t flags) {
        return raw_val;
    }
    virtual const char *name() const { return "datum"; }
    counted_t<val_t> raw_val;
};

class constant_term_t : public op_term_t {
public:
    constant_term_t(compile_env_t *env, protob_t<const Term> t,
                    double constant, const char *name)
        : op_term_t(env, t, argspec_t(0)), _constant(constant), _name(name) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *, UNUSED eval_flags_t flags) {
        return new_val(make_counted<const datum_t>(_constant));
    }
    virtual const char *name() const { return _name; }
    const double _constant;
    const char *const _name;
};

class make_array_term_t : public op_term_t {
public:
    make_array_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        datum_ptr_t acc(datum_t::R_ARRAY);
        {
            explain::sampler_t sampler("Evaluating elements in make_array.", env->env->trace);
            for (size_t i = 0; i < num_args(); ++i) {
                acc.add(arg(env, i)->as_datum());
                sampler.new_sample();
            }
        }
        return new_val(acc.to_counted());
    }
    virtual const char *name() const { return "make_array"; }
};

class make_obj_term_t : public op_term_t {
public:
    make_obj_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0), optargspec_t::make_object()) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, eval_flags_t flags) {
        bool literal_ok = flags & LITERAL_OK;
        eval_flags_t new_flags = literal_ok ? LITERAL_OK : NO_FLAGS;
        datum_ptr_t acc(datum_t::R_OBJECT);
        {
            explain::sampler_t sampler("Evaluating elements in make_obj.", env->env->trace);
            for (auto it = optargs.begin(); it != optargs.end(); ++it) {
                bool dup = acc.add(it->first, it->second->eval(env, new_flags)->as_datum());
                rcheck(!dup, base_exc_t::GENERIC,
                       strprintf("Duplicate key in object: %s.", it->first.c_str()));
                sampler.new_sample();
            }
        }
        return new_val(acc.to_counted());
    }
    virtual const char *name() const { return "make_obj"; }
};

counted_t<term_t> make_datum_term(const protob_t<const Term> &term) {
    return make_counted<datum_term_t>(term);
}
counted_t<term_t> make_constant_term(compile_env_t *env, const protob_t<const Term> &term,
                                     double constant, const char *name) {
    return make_counted<constant_term_t>(env, term, constant, name);
}
counted_t<term_t> make_make_array_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<make_array_term_t>(env, term);
}
counted_t<term_t> make_make_obj_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<make_obj_term_t>(env, term);
}

} // namespace ql
