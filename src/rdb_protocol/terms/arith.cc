// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <cmath>
#include <limits>

#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pseudo_time.hpp"

namespace ql {

class arith_term_t : public op_term_t {
public:
    arith_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, -1)), namestr(0), op(0) {
        switch (static_cast<int>(term.type())) {
        case Term::ADD: namestr = "ADD"; op = &arith_term_t::add; break;
        case Term::SUB: namestr = "SUB"; op = &arith_term_t::sub; break;
        case Term::MUL: namestr = "MUL"; op = &arith_term_t::mul; break;
        case Term::DIV: namestr = "DIV"; op = &arith_term_t::div; break;
        default: unreachable();
        }
        guarantee(namestr && op);
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_t acc = args->arg(env, 0)->as_datum();
        for (size_t i = 1; i < args->num_args(); ++i) {
            acc = (this->*op)(acc, args->arg(env, i)->as_datum(), env->env->limits());
        }
        return new_val(acc);
    }

    virtual const char *name() const { return namestr; }

private:
    datum_t add(datum_t lhs,
                datum_t rhs,
                const configured_limits_t &limits) const {
        if (lhs.is_ptype(pseudo::time_string) ||
            rhs.is_ptype(pseudo::time_string)) {
            return pseudo::time_add(lhs, rhs);
        } else if (lhs.get_type() == datum_t::R_NUM) {
            rhs.check_type(datum_t::R_NUM);
            return datum_t(lhs.as_num() + rhs.as_num());
        } else if (lhs.get_type() == datum_t::R_STR) {
            rhs.check_type(datum_t::R_STR);
            return datum_t(concat(lhs.as_str(), rhs.as_str()));
        } else if (lhs.get_type() == datum_t::R_ARRAY) {
            rhs.check_type(datum_t::R_ARRAY);
            datum_array_builder_t out(limits);
            for (size_t i = 0; i < lhs.arr_size(); ++i) {
                out.add(lhs.get(i));
            }
            for (size_t i = 0; i < rhs.arr_size(); ++i) {
                out.add(rhs.get(i));
            }
            return std::move(out).to_datum();
        } else {
            // If we get here lhs is neither number nor string
            // so we'll just error saying we expect a number
            lhs.check_type(datum_t::R_NUM);
        }
        unreachable();
    }

    datum_t sub(datum_t lhs,
                datum_t rhs,
                UNUSED const configured_limits_t &limits) const {
        if (lhs.is_ptype(pseudo::time_string)) {
            return pseudo::time_sub(lhs, rhs);
        } else {
            lhs.check_type(datum_t::R_NUM);
            rhs.check_type(datum_t::R_NUM);
            return datum_t(lhs.as_num() - rhs.as_num());
        }
    }
    datum_t mul(datum_t lhs,
                datum_t rhs,
                const configured_limits_t &limits) const {
        if (lhs.get_type() == datum_t::R_ARRAY ||
            rhs.get_type() == datum_t::R_ARRAY) {
            datum_t array =
                (lhs.get_type() == datum_t::R_ARRAY ? lhs : rhs);
            datum_t num =
                (lhs.get_type() == datum_t::R_ARRAY ? rhs : lhs);

            datum_array_builder_t out(limits);
            const int64_t num_copies = num.as_int();
            rcheck(num_copies >= 0, base_exc_t::LOGIC,
                   "Cannot multiply an ARRAY by a negative number.");

            for (int64_t j = 0; j < num_copies; ++j) {
                for (size_t i = 0; i < array.arr_size(); ++i) {
                    out.add(array.get(i));
                }
            }
            return std::move(out).to_datum();
        } else {
            lhs.check_type(datum_t::R_NUM);
            rhs.check_type(datum_t::R_NUM);
            return datum_t(lhs.as_num() * rhs.as_num());
        }
    }
    datum_t div(datum_t lhs,
                datum_t rhs,
                UNUSED const configured_limits_t &limits) const {
        lhs.check_type(datum_t::R_NUM);
        rhs.check_type(datum_t::R_NUM);
        rcheck(rhs.as_num() != 0, base_exc_t::LOGIC, "Cannot divide by zero.");
        // throws on non-finite values
        return datum_t(lhs.as_num() / rhs.as_num());
    }

    const char *namestr;
    datum_t (arith_term_t::*op)(datum_t lhs,
                                datum_t rhs,
                                const configured_limits_t &limits) const;
};

class mod_term_t : public op_term_t {
public:
    mod_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        int64_t i0 = args->arg(env, 0)->as_int();
        int64_t i1 = args->arg(env, 1)->as_int();
        rcheck(i1, base_exc_t::LOGIC, "Cannot take a number modulo 0.");
        rcheck(!(i0 == std::numeric_limits<int64_t>::min() && i1 == -1),
               base_exc_t::LOGIC,
               strprintf("Cannot take %" PRIi64 " mod %" PRIi64, i0, i1));
        return new_val(datum_t(static_cast<double>(i0 % i1)));
    }
    virtual const char *name() const { return "mod"; }
};


int64_t bit_arith_ranged_int(const bt_rcheckable_t *target, scoped_ptr_t<val_t> &&val) {
    // All bitwise ops return values in the interval -(1<<53) <= ... < (1<<53) if the
    // parameters are in that interval.  (This is because the operations bit_and,
    // bit_or, bit_xor, and bit_not are closed under it.  Prohibiting 1<<53 is a UI/API
    // design question, so don't be _too_ afraid to enable it (and check return values)
    // if a real-world problem is encountered.)
    int64_t ret = val->as_int();
    static_assert(DBL_MANT_DIG == 53, "double has unsupported representation");
    rcheck_target(target, ret >= -(1LL << 53) && ret < (1LL << 53),
        base_exc_t::LOGIC,
        strprintf("Integer too large: %" PRIi64, ret));
    return ret;
}

class bit_arith_term_t : public op_term_t {
public:
    bit_arith_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, -1)), namestr(nullptr), op(nullptr) {
        switch (static_cast<int>(term.type())) {
        case Term::BIT_AND:
            namestr = "BIT_AND";
            op = [](int64_t lhs, int64_t rhs) { return lhs & rhs; };
            break;
        case Term::BIT_OR:
            namestr = "BIT_OR";
            op = [](int64_t lhs, int64_t rhs) { return lhs | rhs; };
            break;
        case Term::BIT_XOR:
            namestr = "BIT_XOR";
            op = [](int64_t lhs, int64_t rhs) { return lhs ^ rhs; };
            break;
        default: unreachable();
        }
        guarantee(namestr && op);
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        int64_t acc = bit_arith_ranged_int(this, args->arg(env, 0));
        for (size_t i = 1, n = args->num_args(); i < n; ++i) {
            int64_t rhs = bit_arith_ranged_int(this, args->arg(env, i));
            acc = (*op)(acc, rhs);
        }
        return new_val(datum_t(static_cast<double>(acc)));
    }

    virtual const char *name() const { return namestr; }

private:
    const char *namestr;
    int64_t (*op)(int64_t lhs, int64_t rhs);
};

class bit_not_term_t : public op_term_t {
public:
    bit_not_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        int64_t arg = bit_arith_ranged_int(this, args->arg(env, 0));
        return new_val(datum_t(static_cast<double>(~arg)));
    }
    virtual const char *name() const { return "BIT_NOT"; }
};

class bit_shift_term_t : public op_term_t {
public:
    bit_shift_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)), namestr(nullptr), op(nullptr) {
        switch (static_cast<int>(term.type())) {
        case Term::BIT_SAL: namestr = "BIT_SAL"; op = &bit_shift_term_t::bit_sal; break;
        case Term::BIT_SAR: namestr = "BIT_SAR"; op = &bit_shift_term_t::bit_sar; break;
        default: unreachable();
        }
        guarantee(namestr && op);
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        // First argument is a bounded "bit-flippable" integer, the second is any old integer.
        int64_t lhs = bit_arith_ranged_int(this, args->arg(env, 0));
        int64_t rhs = args->arg(env, 1)->as_int();
        rcheck(rhs >= 0, base_exc_t::LOGIC, "Cannot bit-shift by a negative value");

        double ret = (*op)(lhs, rhs);
        return new_val(datum_t(ret));
    }

    virtual const char *name() const { return namestr; }

private:
    static double bit_sal(int64_t lhs, int64_t rhs) {
        // ldexp returns HUGE_VAL on overflow, we check that this is +infinity on
        // this implementation.  (+infinity is handled upon datum conversion later.)
        static_assert(HUGE_VAL == INFINITY, "This implementation assumes HUGE_VAL == INFINITY.");
        if (rhs < 4096) {
            // ldexp takes an int -- the comparison with 4096 ensures this.
            return ldexp(lhs, rhs);
        } else {
            // Everything will overflow with such a large shift except 0.
            if (lhs == 0) {
                return 0;
            }
            return HUGE_VAL;
        }
    }
    static double bit_sar(int64_t lhs, int64_t rhs) {
        // This case is easy -- just have to avoid undefined behavior.
        if (rhs > 63) {
            return lhs < 0 ? -1 : 0;
        }
        return lhs >> rhs;
    }

    const char *namestr;
    double (*op)(int64_t lhs, int64_t rhs);
};

class floor_term_t : public op_term_t {
public:
    floor_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        return new_val(datum_t(std::floor(args->arg(env, 0)->as_num())));
    }

    virtual const char *name() const { return "floor"; }
};

class ceil_term_t : public op_term_t {
public:
    ceil_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        return new_val(datum_t(std::ceil(args->arg(env, 0)->as_num())));
    }

    virtual const char *name() const { return "ceil"; }
};

class round_term_t : public op_term_t {
public:
    round_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        return new_val(datum_t(std::round(args->arg(env, 0)->as_num())));
    }

    virtual const char *name() const { return "round"; }
};

counted_t<term_t> make_arith_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<arith_term_t>(env, term);
}

counted_t<term_t> make_mod_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<mod_term_t>(env, term);
}

counted_t<term_t> make_bit_arith_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<bit_arith_term_t>(env, term);
}

counted_t<term_t> make_bit_not_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<bit_not_term_t>(env, term);
}

counted_t<term_t> make_bit_shift_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<bit_shift_term_t>(env, term);
}

counted_t<term_t> make_floor_term(
            compile_env_t *env, const raw_term_t &term) {
    return make_counted<floor_term_t>(env, term);
}

counted_t<term_t> make_ceil_term(
            compile_env_t *env, const raw_term_t &term) {
    return make_counted<ceil_term_t>(env, term);
}

counted_t<term_t> make_round_term(
            compile_env_t *env, const raw_term_t &term) {
    return make_counted<round_term_t>(env, term);
}

}  // namespace ql
