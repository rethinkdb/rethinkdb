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

class keys_term_t : public op_term_t {
public:
    keys_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> d = arg(0)->as_datum();
        const std::map<std::string, counted_t<const datum_t> > &obj = d->as_object();
        scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            arr->add(make_counted<const datum_t>(it->first));
        }
        return new_val(counted_t<const datum_t>(arr.release()));
    }
    virtual const char *name() const { return "keys"; }
};

counted_t<term_t> make_keys_term(env_t *env, protob_t<const Term> term) {
    return make_counted<keys_term_t>(env, term);
}

counted_t<term_t> make_getattr_term(env_t *env, protob_t<const Term> term) {
    return make_counted<getattr_term_t>(env, term);
}

} // namespace ql
