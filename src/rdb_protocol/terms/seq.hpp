#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

class map_term_t : public op_term_t {
public:
    map_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->map(arg(1)->as_func()));
    }
    RDB_NAME("map");
};

class filter_term_t : public op_term_t {
public:
    filter_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->filter(arg(1)->as_func()));
    }
    RDB_NAME("filter")
};

} //namespace ql
