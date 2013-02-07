#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

class var_term_t : public op_term_t {
public:
    var_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) {
        int var = arg(0)->as_int();
        datum_val = env->top_var(var);
    }
private:
    const datum_t **datum_val;
    virtual val_t *eval_impl() {
        // debugf("VARTERM %p -> %p\n", datum_val, *datum_val);
        return new_val(*datum_val);
    }
    RDB_NAME("var");
};

class implicit_var_term_t : public op_term_t {
public:
    implicit_var_term_t(env_t *env, const Term2 *term) :
        op_term_t(env, term, argspec_t(0)) {
        datum_val = env->top_implicit();
    }
private:
    const datum_t **datum_val;
    virtual val_t *eval_impl() {
        return new_val(*datum_val);
    }
    RDB_NAME("var");
};


} //namespace ql
