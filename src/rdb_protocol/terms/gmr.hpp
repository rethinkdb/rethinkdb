#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

static const char *const gmr_optargs[] = {"base"};
class gmr_term_t : public op_term_t {
public:
    gmr_term_t(env_t *env, const Term2 *term)
        : op_term_t(env, term, argspec_t(4), LEGAL_OPTARGS(gmr_optargs)) { }
private:
    virtual val_t *eval_impl() {
        val_t *v = optarg("base", 0);
        return new_val(arg(0)->as_seq()->gmr(arg(1)->as_func(),
                                             arg(2)->as_func(),
                                             v ? v->as_datum() : 0,
                                             arg(3)->as_func()));
    }
    RDB_NAME("grouped_map_reduce")
};

} // namespace ql
