#ifndef RDB_PROTOCOL_TERMS_GMR_HPP_
#define RDB_PROTOCOL_TERMS_GMR_HPP_

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

static const char *const gmr_optargs[] = {"base"};
class gmr_term_t : public op_term_t {
public:
    gmr_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(4), optargspec_t(gmr_optargs)) { }
private:
    virtual val_t *eval_impl() {
        val_t *baseval = optarg("base", 0);
        counted_t<const datum_t> base = baseval ? baseval->as_datum() : counted_t<const datum_t>();
        func_t *g = arg(1)->as_func(), *m = arg(2)->as_func(), *r = arg(3)->as_func();
        return new_val(arg(0)->as_seq()->gmr(g, m, base, r));
    }
    virtual const char *name() const { return "grouped_map_reduce"; }
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_GMR_HPP_
