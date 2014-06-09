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

class cluster_access_t {
public:
    cluster_access_t(
        base_namespace_repo_t *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
        uuid_u _this_machine);

    base_namespace_repo_t *ns_repo;

    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t > > >
        namespaces_semilattice_metadata;
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
        databases_semilattice_metadata;

    // This is a read-WRITE view because of things like table_create_term_t,
    // db_create_term_t, etc.  Its home thread might be different from ours.
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        semilattice_metadata;

    // This field can be NULL.  Importantly, this field is NULL everywhere except in
    // the parser's env_t.  This is because you "cannot nest meta operations inside
    // queries" -- as meta_write_op_t will complain.  However, term_walker.cc is what
    // actually enforces this property.
    directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;


    // Semilattice modification functions
    void join_and_wait_to_propagate(
            const cluster_semilattice_metadata_t &metadata_to_join,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);


    const uuid_u this_machine;
};

namespace changefeed {
class client_t;
} // namespace changefeed

profile_bool_t profile_bool_optarg(const protob_t<Query> &query);

class env_t : public home_thread_mixin_t {
public:
    env_t(
        extproc_pool_t *_extproc_pool,
        const std::string &_reql_http_proxy,
        base_namespace_repo_t *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        signal_t *_interruptor,
        uuid_u _this_machine);

    env_t(rdb_context_t *ctx, signal_t *interruptor,
          std::map<std::string, wire_func_t> optargs);

    explicit env_t(signal_t *interruptor);

    ~env_t();

    static const uint32_t EVALS_BEFORE_YIELD = 256;
    uint32_t evals_since_yield;

    // Will yield after EVALS_BEFORE_YIELD calls
    void maybe_yield();

    // Returns js_runner, but first calls js_runner->begin() if it hasn't
    // already been called.
    js_runner_t *get_js_runner();

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

    // A pool used for running external JS jobs.  Inexplicably this isn't inside of
    // js_runner_t.
    extproc_pool_t *const extproc_pool;

    // Holds a bunch of mailboxes and maps them to streams.
    changefeed::client_t *changefeed_client;

    // HTTP proxy to use when running `r.http(...)` queries
    const std::string reql_http_proxy;

    // Access to the cluster, for talking over the cluster or about the cluster.
    cluster_access_t cluster_access;

    // The interruptor signal while a query evaluates.
    signal_t *const interruptor;

    // This is _always_ empty, because profiling is not supported in this release.
    // (Unfortunately, all the profiling code expects this field to exist!  Letting
    // this field be empty is the quickest way to disable profiling support in the
    // 1.13 release.  When reintroducing profiling support, please make sure that
    // every env_t constructor contains a profile parameter -- rdb_read_visitor_t in
    // particular no longer passes its profile parameter along.
    const scoped_ptr_t<profile::trace_t> trace;

    // Always returns profile_bool_t::DONT_PROFILE for now, because trace is empty,
    // because we don't support profiling in this release.
    profile_bool_t profile() const;

private:
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
