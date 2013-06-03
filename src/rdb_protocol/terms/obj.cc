#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class getattr_term_t : public op_term_t {
public:
    getattr_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        return new_val(arg(0)->as_datum()->get(arg(1)->as_str()));
    }
    virtual const char *name() const { return "getattr"; }
};

counted_t<term_t> make_getattr_term(env_t *env, protob_t<const Term> term) {
    return make_counted<getattr_term_t>(env, term);
}

} // namespace ql
