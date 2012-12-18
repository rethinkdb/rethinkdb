#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

class map_term_t : public op_term_t {
public:
    map_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        datum_stream_t *seq = arg(0)->as_seq();
        func_t *f = arg(1)->as_func();
        return new_val(new datum_stream_t(seq, f));
    }
    RDB_NAME("map");
};

} //namespace ql
