#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {
class json_term_t : public op_term_t {
    json_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1)) { }

    counted_t<val_t> eval_impl() {
        std::string json_string = arg(0)->as_str();
        scoped_cJSON_t cjson(cJSON_Parse(json_string.c_str()));
        return new_val(make_counted<const datum_t>(cjson.get(), env));
    }

    bool is_deterministic_impl() const {
        return true;
    }
    virtual const char *name() const { return "json"; }
};
} //namespace ql

