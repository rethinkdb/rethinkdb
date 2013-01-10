#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {
class all_term_t : public op_term_t {
public:
    all_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1,-1)) { }
private:
    virtual val_t *eval_impl() {
        for (size_t i = 0; i < num_args(); ++i) {
            env_checkpointer_t ect(env, &env_t::discard_checkpoint);
            val_t *v = arg(i);
            if (!v->as_datum()->as_bool()) {
                ect.reset(&env_t::merge_checkpoint);
                return v;
            } else if (i == num_args()-1) {
                ect.reset(&env_t::merge_checkpoint);
                return v;
            }
        }
        unreachable();
    }
    RDB_NAME("all")
};

class any_term_t : public op_term_t {
public:
    any_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1,-1)) { }
private:
    virtual val_t *eval_impl() {
        for (size_t i = 0; i < num_args(); ++i) {
            env_checkpointer_t ect(env, &env_t::discard_checkpoint);
            val_t *v = arg(i);
            if (v->as_datum()->as_bool()) {
                ect.reset(&env_t::merge_checkpoint);
                return v;
            }
        }
        return new_val(new datum_t(false));
        unreachable();
    }
    RDB_NAME("any")
};
} // namespace ql
