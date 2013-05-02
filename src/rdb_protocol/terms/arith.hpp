#ifndef RDB_PROTOCOL_TERMS_ARITH_HPP_
#define RDB_PROTOCOL_TERMS_ARITH_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class arith_term_t : public op_term_t {
public:
    arith_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1, -1)), namestr(0), op(0) {
        int arithtype = term->type();
        switch (arithtype) {
        case Term_TermType_ADD: namestr = "ADD"; op = &arith_term_t::add; break;
        case Term_TermType_SUB: namestr = "SUB"; op = &arith_term_t::sub; break;
        case Term_TermType_MUL: namestr = "MUL"; op = &arith_term_t::mul; break;
        case Term_TermType_DIV: namestr = "DIV"; op = &arith_term_t::div; break;
        default:unreachable();
        }
        guarantee(namestr && op);
    }

    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> acc = arg(0)->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            acc = (this->*op)(acc, arg(i)->as_datum());
        }
        return new_val(acc);
    }

    virtual const char *name() const { return namestr; }

private:
    counted_t<const datum_t> add(counted_t<const datum_t> lhs, counted_t<const datum_t> rhs) {
        if (lhs->get_type() == datum_t::R_NUM) {
            rhs->check_type(datum_t::R_NUM);
            return make_counted<datum_t>(lhs->as_num() + rhs->as_num());
        } else if (lhs->get_type() == datum_t::R_STR) {
            rhs->check_type(datum_t::R_STR);
            return make_counted<datum_t>(lhs->as_str() + rhs->as_str());
        }

        // If we get here lhs is neither number nor string
        // so we'll just error saying we expect a number
        lhs->check_type(datum_t::R_NUM);

        unreachable();
    }

    counted_t<const datum_t> sub(counted_t<const datum_t> lhs, counted_t<const datum_t> rhs) {
        lhs->check_type(datum_t::R_NUM);
        rhs->check_type(datum_t::R_NUM);
        return make_counted<datum_t>(lhs->as_num() - rhs->as_num());
    }
    counted_t<const datum_t> mul(counted_t<const datum_t> lhs, counted_t<const datum_t> rhs) {
        lhs->check_type(datum_t::R_NUM);
        rhs->check_type(datum_t::R_NUM);
        return make_counted<datum_t>(lhs->as_num() * rhs->as_num());
    }
    counted_t<const datum_t> div(counted_t<const datum_t> lhs, counted_t<const datum_t> rhs) {
        lhs->check_type(datum_t::R_NUM);
        rhs->check_type(datum_t::R_NUM);
        rcheck(rhs->as_num() != 0, "Cannot divide by zero.");
        // throws on non-finite values
        return make_counted<datum_t>(lhs->as_num() / rhs->as_num());
    }

    const char *namestr;
    counted_t<const datum_t> (arith_term_t::*op)(counted_t<const datum_t> lhs, counted_t<const datum_t> rhs);
};

class mod_term_t : public op_term_t {
public:
    mod_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        int64_t i0 = arg(0)->as_int();
        int64_t i1 = arg(1)->as_int();
        rcheck(i1, "Cannot take a number modulo 0.");
        rcheck(!(i0 == INT64_MIN && i1 == -1),
               strprintf("Cannot take %" PRIi64 " mod %" PRIi64, i0, i1));
        return new_val(make_counted<const datum_t>(static_cast<double>(i0 % i1)));
    }
    virtual const char *name() const { return "mod"; }
};

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_ARITH_HPP_
