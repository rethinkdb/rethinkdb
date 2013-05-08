#include "rdb_protocol/terms/error.hpp"


#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class error_term_t : public op_term_t {
public:
    error_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        rfail("%s", arg(0)->as_str().c_str());
        unreachable();
    }
    virtual const char *name() const { return "error"; }
};

counted_t<term_t> make_error_term(env_t *env, protob_t<const Term> term) {
    return make_counted<error_term_t>(env, term);
}


} //namespace ql
