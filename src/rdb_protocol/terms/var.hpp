#ifndef RDB_PROTOCOL_TERMS_VAR_HPP_
#define RDB_PROTOCOL_TERMS_VAR_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class var_term_t : public op_term_t {
public:
    var_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(1)) {
        int var = arg(0)->as_int<int>();
        datum_val = env->top_var(var, this);
    }
private:
    const datum_t **datum_val; // pointer to variable's slot in argument array
    virtual val_t *eval_impl() {
        // debugf("VARTERM %p -> %p\n", datum_val, *datum_val);
        return new_val(*datum_val);
    }
    virtual const char *name() const { return "var"; }
};

class implicit_var_term_t : public op_term_t {
public:
    implicit_var_term_t(env_t *env, const Term *term) :
        op_term_t(env, term, argspec_t(0)) {
        datum_val = env->top_implicit(this);
    }
private:
    const datum_t **datum_val;
    virtual val_t *eval_impl() {
        return new_val(*datum_val);
    }
    virtual const char *name() const { return "var"; }
};


} //namespace ql

#endif // RDB_PROTOCOL_TERMS_VAR_HPP_
