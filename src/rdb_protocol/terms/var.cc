// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "math.hpp"

namespace ql {

class var_term_t : public term_t {
public:
    var_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : term_t(term) {
        rcheck(term->args_size() == 1, base_exc_t::GENERIC,
               "A variable term has the wrong number of arguments.");

        const Term &arg0 = term->args(0);
        rcheck(arg0.type() == Term::DATUM, base_exc_t::GENERIC,
               "A variable term has a non-numeric argument.");
        rcheck(arg0.has_datum(), base_exc_t::GENERIC,
               "A datum term (in a variable term) is missing its datum field.");

        const Datum &datum = arg0.datum();
        rcheck(datum.type() == Datum::R_NUM, base_exc_t::GENERIC,
               "A variable term has a non-numeric variable name argument.");
        rcheck(datum.has_r_num(), base_exc_t::GENERIC,
               "A variable term's datum term is missing its r_num field.");

        const double number = datum.r_num();
        const int64_t var_value = checked_convert_to_int(this, number);

        sym_t var(var_value);
        rcheck(env->visibility.contains_var(var), base_exc_t::GENERIC,
               "Variable name not found.");

        varname = var;
    }

private:
    virtual void accumulate_captures(var_captures_t *captures) const {
        captures->vars_captured.insert(varname);
    }

    virtual bool is_deterministic() const {
        return true;
    }

    sym_t varname;
    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env, eval_flags_t) const {
        return new_val(env->scope.lookup_var(varname));
    }
    virtual const char *name() const { return "var"; }
};

class implicit_var_term_t : public term_t {
public:
    implicit_var_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : term_t(term) {
        rcheck(
            term->args_size() == 0 && term->optargs_size() == 0, base_exc_t::GENERIC,
            "Expected no arguments or optional arguments on implicit variable term.");

        rcheck(env->visibility.implicit_is_accessible(), base_exc_t::GENERIC,
               env->visibility.get_implicit_depth() == 0
               ? "r.row is not defined in this context."
               : "Cannot use r.row in nested queries.  Use functions instead.");
    }
private:
    virtual void accumulate_captures(var_captures_t *captures) const {
        captures->implicit_is_captured = true;
    }

    virtual bool is_deterministic() const {
        return true;
    }

    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env, UNUSED eval_flags_t flags) const {
        return new_val(env->scope.lookup_implicit());
    }
    virtual const char *name() const { return "implicit_var"; }
};

counted_t<term_t> make_var_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<var_term_t>(env, term);
}
counted_t<term_t> make_implicit_var_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<implicit_var_term_t>(env, term);
}

} // namespace ql
