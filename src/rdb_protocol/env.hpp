// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ENV_HPP_
#define RDB_PROTOCOL_ENV_HPP_

#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "concurrency/one_per_thread.hpp"
#include "containers/counted.hpp"
#include "containers/lru_cache.hpp"
#include "extproc/js_runner.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/optargs.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/val.hpp"
#include "rdb_protocol/var_types.hpp"
#include "rdb_protocol/wire_func.hpp"

class extproc_pool_t;

namespace re2 {
class RE2;
}

namespace ql {
class datum_t;
class term_t;

scoped_ptr_t<profile::trace_t> maybe_make_profile_trace(profile_bool_t profile);

struct regex_cache_t {
    explicit regex_cache_t(size_t cache_size) : regexes(cache_size) {}
    lru_cache_t<std::string, std::shared_ptr<re2::RE2> > regexes;
};

class env_t : public home_thread_mixin_t {
public:
    // This is _not_ to be used for secondary index function evaluation -- it doesn't
    // take a reql_version parameter.
    env_t(rdb_context_t *ctx,
          return_empty_normal_batches_t return_empty_normal_batches,
          signal_t *interruptor,
          global_optargs_t optargs,
          profile::trace_t *trace);

    // Used in unittest and for some secondary index environments (hence the
    // reql_version parameter).  (For secondary indexes, the interruptor definitely
    // should be a dummy cond.)
    explicit env_t(signal_t *interruptor,
                   return_empty_normal_batches_t return_empty_normal_batches,
                   reql_version_t reql_version);

    ~env_t();

    // Will yield after EVALS_BEFORE_YIELD calls
    void maybe_yield();

    extproc_pool_t *get_extproc_pool();

    // Returns js_runner, but first calls js_runner->begin() if it hasn't
    // already been called.
    js_runner_t *get_js_runner();

    reql_cluster_interface_t *reql_cluster_interface();

    std::string get_reql_http_proxy();

    // This is a callback used in unittests to control things during a query
    class eval_callback_t {
    public:
        virtual ~eval_callback_t() { }
        virtual void eval_callback() = 0;
    };

    void set_eval_callback(eval_callback_t *callback);
    void do_eval_callback();


    const global_optargs_t &get_all_optargs() const {
        return global_optargs_;
    }

    scoped_ptr_t<val_t> get_optarg(env_t *env, const std::string &key) {
        return global_optargs_.get_optarg(env, key);
    }

    configured_limits_t limits() const {
        return limits_;
    }

    configured_limits_t limits_with_changefeed_queue_size(
        scoped_ptr_t<val_t> changefeed_queue_size) {
        if (changefeed_queue_size.has()) {
            return configured_limits_t(
                check_limit("changefeed queue size",
                            changefeed_queue_size->as_int()),
                limits_.array_size_limit());
        } else {
            return limits_;
        }
    }

    regex_cache_t &regex_cache() { return regex_cache_; }

    reql_version_t reql_version() const { return reql_version_; }

private:
    // The global optargs values passed to .run(...) in the Python, Ruby, and JS
    // drivers.
    global_optargs_t global_optargs_;

    // User specified configuration limits; e.g. array size limits
    const configured_limits_t limits_;

    // The version of ReQL behavior that we should use.  Normally this is
    // LATEST_DISK, but when evaluating secondary index functions, it could be an
    // earlier value.
    const reql_version_t reql_version_;

    // query specific cache parameters; for example match regexes.
    regex_cache_t regex_cache_;

public:
    const return_empty_normal_batches_t return_empty_normal_batches;

    // The interruptor signal while a query evaluates.
    signal_t *const interruptor;

    // This is non-empty when profiling is enabled.
    profile::trace_t *const trace;

    profile_bool_t profile() const;

    rdb_context_t *get_rdb_ctx() { return rdb_ctx_; }

private:
    static const uint32_t EVALS_BEFORE_YIELD = 256;
    uint32_t evals_since_yield_;

    rdb_context_t *const rdb_ctx_;

    js_runner_t js_runner_;

    eval_callback_t *eval_callback_;

    DISABLE_COPYING(env_t);
};

// An environment in which expressions are compiled.  Since compilation doesn't
// evaluate anything, it doesn't need an env_t *.
class compile_env_t {
public:
    explicit compile_env_t(var_visibility_t &&_visibility)
        : visibility(std::move(_visibility)) { }
    var_visibility_t visibility;
};

// This is an environment for evaluating things that use variables in scope.  It
// supplies the variables along with the "global" evaluation environment.
class scope_env_t {
public:
    scope_env_t(env_t *_env, var_scope_t &&_scope)
        : env(_env), scope(std::move(_scope)) { }
    env_t *const env;
    const var_scope_t scope;

    DISABLE_COPYING(scope_env_t);
};

}  // namespace ql

#endif // RDB_PROTOCOL_ENV_HPP_
