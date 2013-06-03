#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class gmr_term_t : public op_term_t {
public:
    gmr_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(4), optargspec_t({ "base" })) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> baseval = optarg("base");
        counted_t<const datum_t> base = baseval.has() ?
            baseval->as_datum() :
            counted_t<const datum_t>();
        counted_t<func_t> g = arg(1)->as_func();
        counted_t<func_t> m = arg(2)->as_func();
        counted_t<func_t> r = arg(3)->as_func();
        return new_val(arg(0)->as_seq()->gmr(g, m, base, r));
    }
    virtual const char *name() const { return "grouped_map_reduce"; }
};

counted_t<term_t> make_gmr_term(env_t *env, protob_t<const Term> term) {
    return make_counted<gmr_term_t>(env, term);
}

} // namespace ql
