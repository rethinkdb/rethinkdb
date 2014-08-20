// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"

namespace ql {

bool datum_eq(reql_version_t, const datum_t &lhs, const datum_t &rhs) {
    // Behavior of cmp with respect to datum equality didn't change between versions.
    return lhs == rhs;
}

bool datum_lt(reql_version_t v, const datum_t &lhs, const datum_t &rhs) {
    return lhs.cmp(v, rhs) < 0;
}

bool datum_le(reql_version_t v, const datum_t &lhs, const datum_t &rhs) {
    return lhs.cmp(v, rhs) <= 0;
}

bool datum_gt(reql_version_t v, const datum_t &lhs, const datum_t &rhs) {
    return lhs.cmp(v, rhs) > 0;
}

bool datum_ge(reql_version_t v, const datum_t &lhs, const datum_t &rhs) {
    return lhs.cmp(v, rhs) >= 0;
}

class predicate_term_t : public op_term_t {
public:
    predicate_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2, -1)), namestr(0), invert(false), pred(0) {
        int predtype = term->type();
        switch (predtype) {
        case Term_TermType_EQ: {
            namestr = "EQ";
            pred = &datum_eq;
        } break;
        case Term_TermType_NE: {
            namestr = "NE";
            pred = &datum_eq;
            invert = true; // we invert the == operator so (!= 1 2 3) makes sense
        } break;
        case Term_TermType_LT: {
            namestr = "LT";
            pred = &datum_lt;
        } break;
        case Term_TermType_LE: {
            namestr = "LE";
            pred = &datum_le;
        } break;
        case Term_TermType_GT: {
            namestr = "GT";
            pred = &datum_gt;
        } break;
        case Term_TermType_GE: {
            namestr = "GE";
            pred = &datum_ge;
        } break;
        default: unreachable();
        }
        guarantee(namestr && pred);
    }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const datum_t> lhs = args->arg(env, 0)->as_datum();
        for (size_t i = 1; i < args->num_args(); ++i) {
            counted_t<const datum_t> rhs = args->arg(env, i)->as_datum();
            if (!(pred)(env->env->reql_version(), *lhs, *rhs)) {
                return new_val_bool(static_cast<bool>(false ^ invert));
            }
            lhs = rhs;
        }
        return new_val_bool(static_cast<bool>(true ^ invert));
    }
    const char *namestr;
    virtual const char *name() const { return namestr; }
    bool invert;
    bool (*pred)(reql_version_t, const datum_t &lhs, const datum_t &rhs);
};

class not_term_t : public op_term_t {
public:
    not_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return new_val_bool(!args->arg(env, 0)->as_bool());
    }
    virtual const char *name() const { return "not"; }
};

counted_t<term_t> make_predicate_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<predicate_term_t>(env, term);
}
counted_t<term_t> make_not_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<not_term_t>(env, term);
}

} // namespace ql
