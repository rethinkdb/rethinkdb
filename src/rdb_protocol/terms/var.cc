#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class var_term_t : public op_term_t {
public:
    var_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) {
        sym_t var = sym_t(arg(env, 0)->as_int<int>());
        datum_val = env->scopes.top_var(var, this);
    }
private:
    counted_t<const datum_t> *datum_val; // pointer to variable's slot in argument array
    virtual counted_t<val_t> eval_impl(env_t *, UNUSED eval_flags_t flags) {
        return new_val(*datum_val);
    }
    virtual const char *name() const { return "var"; }
};

class implicit_var_term_t : public op_term_t {
public:
    implicit_var_term_t(env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(0)) {
        datum_val = env->implicits.top_implicit(this);
    }
private:
    counted_t<const datum_t> *datum_val;
    virtual counted_t<val_t> eval_impl(env_t *, UNUSED eval_flags_t flags) {
        return new_val(*datum_val);
    }
    virtual const char *name() const { return "implicit_var"; }
};

counted_t<term_t> make_var_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<var_term_t>(env, term);
}
counted_t<term_t> make_implicit_var_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<implicit_var_term_t>(env, term);
}

} // namespace ql
