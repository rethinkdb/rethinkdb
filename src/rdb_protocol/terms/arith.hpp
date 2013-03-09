#ifndef RDB_PROTOCOL_TERMS_ARITH_HPP_
#define RDB_PROTOCOL_TERMS_ARITH_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

class arith_term_t : public op_term_t {
public:
    arith_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(1, -1)), namestr(0), op(0) {
        int arithtype = term->type();
        switch(arithtype) {
        case Term_TermType_ADD: namestr = "ADD"; op = &arith_term_t::add; break;
        case Term_TermType_SUB: namestr = "SUB"; op = &arith_term_t::sub; break;
        case Term_TermType_MUL: namestr = "MUL"; op = &arith_term_t::mul; break;
        case Term_TermType_DIV: namestr = "DIV"; op = &arith_term_t::div; break;
        default:unreachable();
        }
        guarantee(namestr && op);
    }
    virtual val_t *eval_impl() {
        // I'm not sure what I was smoking when I wrote this.  I think I was
        // trying to avoid undue allocations or something.
        datum_t *acc = env->add_ptr(new datum_t(datum_t::R_NULL));
        *acc = *arg(0)->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            *acc = (this->*op)(*acc, *arg(i)->as_datum());
        }
        return new_val(acc);
    }
    virtual const char *name() const { return namestr; }
private:
    datum_t add(const datum_t &lhs, const datum_t &rhs) {
        if (lhs.get_type() == datum_t::R_NUM) {
            rhs.check_type(datum_t::R_NUM);
            return datum_t(lhs.as_num() + rhs.as_num());
        } else if (lhs.get_type() == datum_t::R_STR) {
            rhs.check_type(datum_t::R_STR);
            return datum_t(lhs.as_str() + rhs.as_str());
        }

        // If we get here lhs is neither number nor string
        // so we'll just error saying we expect a number
        lhs.check_type(datum_t::R_NUM);

        unreachable();
    }

    datum_t sub(const datum_t &lhs, const datum_t &rhs) {
        lhs.check_type(datum_t::R_NUM);
        rhs.check_type(datum_t::R_NUM);
        return datum_t(lhs.as_num() - rhs.as_num());
    }
    datum_t mul(const datum_t &lhs, const datum_t &rhs) {
        lhs.check_type(datum_t::R_NUM);
        rhs.check_type(datum_t::R_NUM);
        return datum_t(lhs.as_num() * rhs.as_num());
    }
    datum_t div(const datum_t &lhs, const datum_t &rhs) {
        lhs.check_type(datum_t::R_NUM);
        rhs.check_type(datum_t::R_NUM);
        rcheck(rhs.as_num() != 0, "Cannot divide by zero.");
        return datum_t(lhs.as_num() / rhs.as_num()); // throws on non-finite values
    }

    const char *namestr;
    datum_t (arith_term_t::*op)(const datum_t &lhs, const datum_t &rhs);
};

class mod_term_t : public op_term_t {
public:
    mod_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        int64_t i0 = arg(0)->as_int();
        int64_t i1 = arg(1)->as_int();
        rcheck(i1, "Cannot take a number modulo 0.");
        // Sam says this is a floating-point exception
        rcheck(!(i0 == INT64_MIN && i1 == -1),
               strprintf("Cannot take %" PRIi64 " mod %" PRIi64, i0, i1));
        return new_val(i0 % i1);
    }
    RDB_NAME("mod");
};

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_ARITH_HPP_
