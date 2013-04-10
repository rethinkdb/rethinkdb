#ifndef RDB_PROTOCOL_TERMS_ERROR_HPP_
#define RDB_PROTOCOL_TERMS_ERROR_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class error_term_t : public op_term_t {
public:
    error_term_t(env_t *env, const Term *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        rfail("%s", arg(0)->as_str().c_str());
        unreachable();
    }
    virtual const char *name() const { return "error"; }
};

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_ERROR_HPP_
