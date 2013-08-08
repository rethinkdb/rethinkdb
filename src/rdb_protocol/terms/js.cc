#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <string>

#include "rdb_protocol/terms/terms.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "extproc/js_runner.hpp"

namespace ql {

class javascript_term_t : public op_term_t {
public:
    javascript_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1), optargspec_t({ "timeout" })) { }
private:

    virtual counted_t<val_t> eval_impl(UNUSED eval_flags_t flags) {
        // Optarg seems designed to take a default value as the second argument
        // but nowhere else is this actually used.
        uint64_t timeout_ms = 5000;
        counted_t<val_t> timeout_opt = optarg("timeout");
        if (timeout_opt) {
            if (timeout_opt->as_num() > static_cast<double>(UINT64_MAX) / 1000) {
                timeout_ms = UINT64_MAX;
            } else {
                timeout_ms = timeout_opt->as_num() * 1000;
            }
        }

        std::string source = arg(0)->as_datum()->as_str();

        // JS runner configuration is limited to setting an execution timeout.
        js_runner_t::req_config_t config;
        config.timeout_ms = timeout_ms;

        try {
            js_result_t result = env->get_js_runner()->eval(source, config);
            return boost::apply_visitor(
                js_result_visitor_t(env, source, timeout_ms, this->counted_from_this()),
                result);
        } catch (const js_worker_exc_t &e) {
            rfail(base_exc_t::GENERIC,
                  "Javascript query `%s` caused a crash in a worker process.",
                  source.c_str());
        } catch (const interrupted_exc_t &e) {
            rfail(base_exc_t::GENERIC,
                  "JavaScript query `%s` timed out after %" PRIu64 ".%03" PRIu64 " seconds.",
                  source.c_str(), timeout_ms / 1000, timeout_ms % 1000);
        }
    }
    virtual const char *name() const { return "javascript"; }

    // No JS term is considered deterministic
    virtual bool is_deterministic_impl() const {
        return false;
    }
};

counted_t<term_t> make_javascript_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<javascript_term_t>(env, term);
}

}  // namespace ql
