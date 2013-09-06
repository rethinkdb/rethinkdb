#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class var_term_t : public op_term_t {
public:
    var_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) {
        // RSI: This shouldn't call arg(&scope_env, 0), this should pull the number straight
        // out of the protobuf.
        scope_env_t empty_scope_env(env->env, var_scope_t());
        sym_t var = sym_t(arg(&empty_scope_env, 0)->as_int());
        // RSI: Is this right?
        rcheck(env->visibility.contains_var(var), base_exc_t::GENERIC,
               "Variable name not found.");
        varname = var;
    }
private:
    sym_t varname;
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env->scope.lookup_var(varname));
    }
    virtual const char *name() const { return "var"; }
};

class implicit_var_term_t : public op_term_t {
public:
    implicit_var_term_t(visibility_env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(0)) {
        // RSI: Is this the right way to do this?  We could certainly be more explicit
        // (nested functions vs. no function).
        rcheck(env->visibility.implicit_is_accessible(), base_exc_t::GENERIC,
               env->visibility.get_implicit_depth() == 0
               ? "r.row is not defined in this context."
               : "Cannot use r.row in nested queries.  Use functions instead.");
    }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env->scope.lookup_implicit());
    }
    virtual const char *name() const { return "implicit_var"; }
};

counted_t<term_t> make_var_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<var_term_t>(env, term);
}
counted_t<term_t> make_implicit_var_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<implicit_var_term_t>(env, term);
}

} // namespace ql
