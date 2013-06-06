#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/js.hpp"

namespace ql {

class javascript_term_t : public op_term_t {
public:
    javascript_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1), optargspec_t({ "timeout" })) { }
private:

    virtual counted_t<val_t> eval_impl() {
        std::string source = arg(0)->as_datum()->as_str();

        boost::shared_ptr<js::runner_t> js = env->get_js_runner();

        // Optarg seems designed to take a default value as the second argument
        // but nowhere else is this actually used.
        double timeout_s = 5.0;
        counted_t<val_t> timeout_opt = optarg("timeout");
        if (timeout_opt) {
            timeout_s = timeout_opt->as_num();
        }

        // JS runner configuration is limited to setting an execution timeout.
        js::runner_t::req_config_t config;
        config.timeout_ms = timeout_s * 1000;

        try {
            js::js_result_t result = js->eval(source, &config);
            return boost::apply_visitor(js_result_visitor_t(env,
                                                            this->counted_from_this()),
                                        result);
        } catch (const interrupted_exc_t &e) {
            rfail(base_exc_t::GENERIC,
                  "JavaScript query `%s` timed out after %.2G seconds.",
                  source.c_str(), timeout_s);
        }
    }
    virtual const char *name() const { return "javascript"; }

    // No JS term is considered deterministic
    virtual bool is_deterministic_impl() const {
        return false;
    }
};

counted_t<term_t> make_javascript_term(env_t *env, protob_t<const Term> term) {
    return make_counted<javascript_term_t>(env, term);
}

}  // namespace ql
