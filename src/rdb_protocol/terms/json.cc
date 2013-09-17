#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {
class json_term_t : public op_term_t {
public:
    json_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }

    counted_t<val_t> eval_impl(UNUSED eval_flags_t flags) {
        std::string data = arg(0)->as_str();
        scoped_cJSON_t cjson(cJSON_Parse(data.c_str()));
        rcheck(cjson.get() != NULL, base_exc_t::GENERIC,
               strprintf("Failed to parse \"%s\" as JSON.",
                 (data.size() > 40 ? (data.substr(0, 37) + "...").c_str() : data.c_str())));
        return new_val(make_counted<const datum_t>(cjson.get()));
    }

    virtual const char *name() const { return "json"; }
};

counted_t<term_t> make_json_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<json_term_t>(env, term);
}
} // namespace ql

