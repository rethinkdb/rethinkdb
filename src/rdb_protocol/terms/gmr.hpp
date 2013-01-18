#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class gmr_term_t : public op_term_t {
public:
    gmr_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(4)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->gmr(arg(1)->as_func(),
                                             arg(2)->as_func(),
                                             arg(3)->as_func()));
    }
    RDB_NAME("grouped_map_reduce")
};

} // namespace ql
