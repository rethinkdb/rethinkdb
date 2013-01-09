#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

class getattr_term_t : public op_term_t {
public:
    getattr_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_datum()->el(arg(1)->as_datum()->as_str()));
    }
    RDB_NAME("getattr")
};

class contains_term_t : public op_term_t {
public:
    contains_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        bool b = arg(0)->as_datum()->el(arg(1)->as_datum()->as_str(), false);
        //                     Return 0 instead of throwing on error. ^^^^^
        return new_val(new datum_t(b));
    }
    RDB_NAME("contains")
};

} // namespace ql
