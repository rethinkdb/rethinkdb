// Copyright 2010-2013 RethinkDB, all rights reserved.
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
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/val.hpp"

class extproc_pool_t;

namespace ql {
class datum_t;
class term_t;
class compile_env_t;

class global_optargs_t {
public:
    global_optargs_t();
    global_optargs_t(const std::map<std::string, wire_func_t> &_optargs);

    // Returns whether or not there was a key conflict.
    MUST_USE bool add_optarg(const std::string &key, const Term &val);
    void init_optargs(const std::map<std::string, wire_func_t> &_optargs);
    counted_t<val_t> get_optarg(env_t *env, const std::string &key); // returns NULL if no entry
    const std::map<std::string, wire_func_t> &get_all_optargs();
private:
    std::map<std::string, wire_func_t> optargs;
};

class cluster_access_t {
public:
    typedef namespaces_semilattice_metadata_t<rdb_protocol_t> ns_metadata_t;
    cluster_access_t(
        base_namespace_repo_t<rdb_protocol_t> *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
        uuid_u _this_machine);

    base_namespace_repo_t<rdb_protocol_t> *ns_repo;

    clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t > > >
        namespaces_semilattice_metadata;
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
        databases_semilattice_metadata;
    // TODO this should really just be the namespace metadata... but
    // constructing views is too hard :-/
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        semilattice_metadata;
    directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;

    // Semilattice modification functions
    void join_and_wait_to_propagate(
            const cluster_semilattice_metadata_t &metadata_to_join,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);


    const uuid_u this_machine;
};

// The env_t.
class env_t : public home_thread_mixin_t {
public:
    typedef namespaces_semilattice_metadata_t<rdb_protocol_t> ns_metadata_t;
    // This is copied basically verbatim from old code.
    env_t(
        extproc_pool_t *_extproc_pool,
        base_namespace_repo_t<rdb_protocol_t> *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
        signal_t *_interruptor,
        uuid_u _this_machine,
        const std::map<std::string, wire_func_t> &_optargs);

    explicit env_t(signal_t *);

    ~env_t();
    void throw_if_interruptor_pulsed() THROWS_ONLY(interrupted_exc_t) {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();
    }


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
    extproc_pool_t *extproc_pool;

    // Access to the cluster, for talking over the cluster or about the cluster.
    cluster_access_t cluster_access;

    // The interruptor signal while a query evaluates.  This can get overwritten!
    signal_t *interruptor;

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
    env_t *env;
    var_scope_t scope;
};

}  // namespace ql

#endif // RDB_PROTOCOL_ENV_HPP_
