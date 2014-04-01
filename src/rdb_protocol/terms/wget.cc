// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <stdint.h>

#include <string>
#include "debug.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "extproc/wget_runner.hpp"

namespace ql {

class wget_result_visitor_t : public boost::static_visitor<counted_t<val_t> > {
public:
    wget_result_visitor_t(const pb_rcheckable_t *_parent) :
          parent(_parent) { }

    // This wget resulted in an error
    counted_t<val_t> operator()(const std::string &err_val) const {
        rfail_target(parent, base_exc_t::GENERIC, "%s", err_val.c_str());
        unreachable();
    }

    // This wget resulted in data
    counted_t<val_t> operator()(const counted_t<const datum_t> &datum) const {
        return make_counted<val_t>(datum, parent->backtrace());
    }

private:
    const pb_rcheckable_t *parent;
};

class wget_term_t : public op_term_t {
public:
    wget_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(1, -1),
                  optargspec_t({ "timeout", "header", "rate_limit" }))
    { }
private:
    uint64_t get_timeout(scope_env_t *env) {
        uint64_t timeout_ms = 120000;
        counted_t<val_t> timeout_opt = optarg(env, "timeout");
        if (timeout_opt) {
            if (timeout_opt->as_num() > static_cast<double>(UINT64_MAX) / 1000) {
                timeout_ms = UINT64_MAX;
            } else {
                timeout_ms = timeout_opt->as_num() * 1000;
            }
        }
        return timeout_ms;
    }

    uint64_t get_rate_limit(scope_env_t *env) {
        uint64_t rate_limit = 0;
        counted_t<val_t> rate_limit_opt = optarg(env, "rate_limit");
        if (rate_limit_opt) {
            if (rate_limit_opt->as_num() > static_cast<double>(UINT64_MAX)) {
                rate_limit = 0;
            } else {
                rate_limit = rate_limit_opt->as_num();
            }
        }
        return rate_limit;
    }

    std::vector<std::string> get_headers(scope_env_t *env) {
        std::vector<std::string> res;
        counted_t<val_t> headers = optarg(env, "header");

        if (headers.has()) {
            counted_t<const datum_t> datum_headers = headers->as_datum();
            if (datum_headers->get_type() == datum_t::R_STR) {
                res.push_back(datum_headers->as_str().to_std());
            } else if (datum_headers->get_type() == datum_t::R_ARRAY) {
                res.resize(datum_headers->size());
                for (size_t i = 0; i < res.size(); ++i) {
                    counted_t<const datum_t> line = datum_headers->get(i);
                    if (line->get_type() != datum_t::R_STR) {
                        rfail_target(this, base_exc_t::GENERIC,
                                     "header optarg must be a string or array of strings");
                    }
                    res.push_back(line->as_str().to_std());
                }
            } else {
                rfail_target(this, base_exc_t::GENERIC,
                             "header optarg must be a string or array of strings");
            }
        }

        return std::move(res);
    }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        uint64_t timeout_ms = get_timeout(env);
        uint64_t rate_limit = get_rate_limit(env);
        std::vector<std::string> headers = get_headers(env);
        std::string url = arg(env, 0)->as_str().to_std();

        wget_result_t wget_result;
        try {
            wget_runner_t runner(env->env->extproc_pool);
            wget_result = runner.wget(url, headers, rate_limit, timeout_ms, env->env->interruptor);
        } catch (const wget_worker_exc_t &e) {
            wget_result = strprintf("wget of `%s` caused a crash in a worker process.", url.c_str());
        } catch (const interrupted_exc_t &e) {
            wget_result = strprintf("wget of `%s` timed out after %" PRIu64 ".%03" PRIu64 " seconds.",
                                    url.c_str(), timeout_ms / 1000, timeout_ms % 1000);
        } catch (const std::exception &e) {
            wget_result = std::string(e.what());
        } catch (...) {
            wget_result = std::string("wget encountered an unknown exception");
        }

        return boost::apply_visitor(wget_result_visitor_t(this), wget_result);
    }

    virtual const char *name() const { return "javascript"; }

    // No JS term is considered deterministic
    virtual bool is_deterministic() const {
        return false;
    }
};

counted_t<term_t> make_wget_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<wget_term_t>(env, term);
}

}  // namespace ql
