// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <limits>

#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"

namespace ql {

class arith_term_t : public op_term_t {
public:
    arith_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1)), namestr(0), op(0) {
        int arithtype = term->type();
        switch (arithtype) {
        case Term_TermType_ADD: namestr = "ADD"; op = &arith_term_t::add; break;
        case Term_TermType_SUB: namestr = "SUB"; op = &arith_term_t::sub; break;
        case Term_TermType_MUL: namestr = "MUL"; op = &arith_term_t::mul; break;
        case Term_TermType_DIV: namestr = "DIV"; op = &arith_term_t::div; break;
        default: unreachable();
        }
        guarantee(namestr && op);
    }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const datum_t> acc = args->arg(env, 0)->as_datum();
        for (size_t i = 1; i < args->num_args(); ++i) {
            acc = (this->*op)(acc, args->arg(env, i)->as_datum(), env->env->limits());
        }
        return new_val(acc);
    }

    virtual const char *name() const { return namestr; }

private:
    counted_t<const datum_t> add(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs,
                                 const configured_limits_t &limits) const {
        if (lhs->is_ptype(pseudo::time_string) ||
            rhs->is_ptype(pseudo::time_string)) {
            return pseudo::time_add(lhs, rhs);
        } else if (lhs->get_type() == datum_t::R_NUM) {
            rhs->check_type(datum_t::R_NUM);
            return make_counted<datum_t>(lhs->as_num() + rhs->as_num());
        } else if (lhs->get_type() == datum_t::R_STR) {
            rhs->check_type(datum_t::R_STR);
            return make_counted<datum_t>(concat(lhs->as_str(), rhs->as_str()));
        } else if (lhs->get_type() == datum_t::R_ARRAY) {
            rhs->check_type(datum_t::R_ARRAY);
            datum_array_builder_t out(limits);
            for (size_t i = 0; i < lhs->size(); ++i) {
                out.add(lhs->get(i));
            }
            for (size_t i = 0; i < rhs->size(); ++i) {
                out.add(rhs->get(i));
            }
            return std::move(out).to_counted();
        } else {
            // If we get here lhs is neither number nor string
            // so we'll just error saying we expect a number
            lhs->check_type(datum_t::R_NUM);
        }
        unreachable();
    }

    counted_t<const datum_t> sub(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs,
                                 const configured_limits_t &limits) const {
        if (lhs->is_ptype(pseudo::time_string)) {
            return pseudo::time_sub(lhs, rhs);
        } else if (lhs->is_ptype(pseudo::geometry_string)) {
            try {
                return pseudo::geo_sub(lhs, rhs, limits);
            } catch (const geo_exception_t &e) {
                rfail(base_exc_t::GENERIC, "%s", e.what());
            }
        } else {
            lhs->check_type(datum_t::R_NUM);
            rhs->check_type(datum_t::R_NUM);
            return make_counted<datum_t>(lhs->as_num() - rhs->as_num());
        }
    }
    counted_t<const datum_t> mul(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs,
                                 const configured_limits_t &limits) const {
        if (lhs->get_type() == datum_t::R_ARRAY ||
            rhs->get_type() == datum_t::R_ARRAY) {
            counted_t<const datum_t> array =
                (lhs->get_type() == datum_t::R_ARRAY ? lhs : rhs);
            counted_t<const datum_t> num =
                (lhs->get_type() == datum_t::R_ARRAY ? rhs : lhs);

            datum_array_builder_t out(limits);
            const int64_t num_copies = num->as_int();
            rcheck(num_copies >= 0, base_exc_t::GENERIC,
                   "Cannot multiply an ARRAY by a negative number.");

            for (int64_t j = 0; j < num_copies; ++j) {
                for (size_t i = 0; i < array->size(); ++i) {
                    out.add(array->get(i));
                }
            }
            return std::move(out).to_counted();
        } else {
            lhs->check_type(datum_t::R_NUM);
            rhs->check_type(datum_t::R_NUM);
            return make_counted<datum_t>(lhs->as_num() * rhs->as_num());
        }
    }
    counted_t<const datum_t> div(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs,
                                 UNUSED const configured_limits_t &limits) const {
        lhs->check_type(datum_t::R_NUM);
        rhs->check_type(datum_t::R_NUM);
        rcheck(rhs->as_num() != 0, base_exc_t::GENERIC, "Cannot divide by zero.");
        // throws on non-finite values
        return make_counted<datum_t>(lhs->as_num() / rhs->as_num());
    }

    const char *namestr;
    counted_t<const datum_t> (arith_term_t::*op)(counted_t<const datum_t> lhs,
                                                 counted_t<const datum_t> rhs,
                                                 const configured_limits_t &limits) const;
};

class mod_term_t : public op_term_t {
public:
    mod_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        int64_t i0 = args->arg(env, 0)->as_int();
        int64_t i1 = args->arg(env, 1)->as_int();
        rcheck(i1, base_exc_t::GENERIC, "Cannot take a number modulo 0.");
        rcheck(!(i0 == std::numeric_limits<int64_t>::min() && i1 == -1),
               base_exc_t::GENERIC,
               strprintf("Cannot take %" PRIi64 " mod %" PRIi64, i0, i1));
        return new_val(make_counted<const datum_t>(static_cast<double>(i0 % i1)));
    }
    virtual const char *name() const { return "mod"; }
};


counted_t<term_t> make_arith_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<arith_term_t>(env, term);
}

counted_t<term_t> make_mod_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<mod_term_t>(env, term);
}


}  // namespace ql
