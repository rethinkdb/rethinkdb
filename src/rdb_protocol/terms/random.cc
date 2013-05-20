#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

class random_term_t : public term_t {
public:
    random_term_t(env_t *env, protob_t<const Term> term)
        : term_t(env, term) { }
    counted_t<val_t> eval_impl() {
        return new_val(counted_t<const datum_t>(new datum_t(randdouble())));
    }
    bool is_deterministic_impl() const {
        return false;
    }
    virtual const char *name() const { return "random"; }
};

counted_t<term_t> make_random_term(env_t *env, protob_t<const Term> term) {
    return counted_t<random_term_t>(new random_term_t(env, term));
}

} //namespace ql
