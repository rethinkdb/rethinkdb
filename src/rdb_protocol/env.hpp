// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ENV_HPP_
#define RDB_PROTOCOL_ENV_HPP_

#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "concurrency/one_per_thread.hpp"
#include "containers/counted.hpp"
#include "extproc/js_runner.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/val.hpp"

class extproc_pool_t;

namespace ql {
class datum_t;
class term_t;

/* If and optarg with the given key is present and is of type DATUM it will be
 * returned. Otherwise an empty counted_t<const datum_t> will be returned. */
counted_t<const datum_t> static_optarg(const std::string &key, protob_t<Query> q);

std::map<std::string, wire_func_t> global_optargs(protob_t<Query> q);

class global_optargs_t {
public:
    global_optargs_t();
    explicit global_optargs_t(std::map<std::string, wire_func_t> optargs);

    void init_optargs(const std::map<std::string, wire_func_t> &_optargs);
    // returns NULL if no entry
    counted_t<val_t> get_optarg(env_t *env, const std::string &key);
    const std::map<std::string, wire_func_t> &get_all_optargs();
private:
    std::map<std::string, wire_func_t> optargs;
};

namespace changefeed {
class client_t;
} // namespace changefeed

profile_bool_t profile_bool_optarg(const protob_t<Query> &query);

class env_t : public home_thread_mixin_t {
public:
    env_t(rdb_context_t *ctx, signal_t *interruptor,
          std::map<std::string, wire_func_t> optargs,
          profile_bool_t is_profile_requested);

    explicit env_t(signal_t *interruptor);

    ~env_t();

    static const uint32_t EVALS_BEFORE_YIELD = 256;
    uint32_t evals_since_yield;

    // Will yield after EVALS_BEFORE_YIELD calls
    void maybe_yield();

    extproc_pool_t *get_extproc_pool();

    // Returns js_runner, but first calls js_runner->begin() if it hasn't
    // already been called.
    js_runner_t *get_js_runner();

    base_namespace_repo_t *ns_repo();
    reql_admin_interface_t *reql_admin_interface();

    changefeed::client_t *get_changefeed_client();

    std::string get_reql_http_proxy();

    // This is a callback used in unittests to control things during a query
    class eval_callback_t {
    public:
        virtual ~eval_callback_t() { }
        virtual void eval_callback() = 0;
    };

    void set_eval_callback(eval_callback_t *callback);
    void do_eval_callback();


    // The global optargs values passed to .run(...) in the Python, Ruby, and JS
    // drivers.
    global_optargs_t global_optargs;

    // The interruptor signal while a query evaluates.
    signal_t *const interruptor;

    // This is non-empty when profiling is enabled.
    const scoped_ptr_t<profile::trace_t> trace;

    profile_bool_t profile() const;

private:
    rdb_context_t *const rdb_ctx;

    js_runner_t js_runner;

    eval_callback_t *eval_callback;

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
