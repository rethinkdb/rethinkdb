#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class keys_term_t : public op_term_t {
public:
    keys_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(UNUSED eval_flags_t flags) {
        counted_t<const datum_t> d = arg(0)->as_datum();
        const std::map<std::string, counted_t<const datum_t> > &obj = d->as_object();

        std::vector<counted_t<const datum_t> > arr;
        arr.reserve(obj.size());
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            arr.push_back(make_counted<const datum_t>(std::string(it->first)));
        }

        return new_val(make_counted<const datum_t>(std::move(arr)));
    }
    virtual const char *name() const { return "keys"; }
};

counted_t<term_t> make_keys_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<keys_term_t>(env, term);
}

} // namespace ql
