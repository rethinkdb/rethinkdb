// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ENV_HPP_
#define RDB_PROTOCOL_ENV_HPP_

#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/one_per_thread.hpp"
#include "containers/counted.hpp"
#include "extproc/pool.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {
class datum_t;
class term_t;

class env_t : private home_thread_mixin_t {
public:
    const uuid_u uuid;

public:
    // returns whether or not there was a key conflict
    MUST_USE bool add_optarg(const std::string &key, const Term &val);
    void init_optargs(const std::map<std::string, wire_func_t> &_optargs);
    counted_t<val_t> get_optarg(const std::string &key); // returns NULL if no entry
    const std::map<std::string, wire_func_t> &get_all_optargs();
private:
    std::map<std::string, wire_func_t> optargs;

public:
    // returns a globaly unique variable
    int gensym(bool allow_implicit = false);
    static bool var_allows_implicit(int varnum);
private:
    int next_gensym_val; // always negative

public:
    // Bind a variable in the current scope.
    void push_var(int var, counted_t<const datum_t> *val);

    // Special variables have unusual behavior.
    enum special_var_t {
        SINDEX_ERROR_VAR = 1, // Throws an error when you try to access it.
    };
    // Push a special variable.  (Pop it off with the normal `pop_var`.)
    void push_special_var(int var, special_var_t special_var);
    // This will push a special variable over ever variable currently in scope
    // when constructed, then pop those variables when destructed.
    class special_var_shadower_t {
    public:
        special_var_shadower_t(env_t *env, special_var_t special_var);
        ~special_var_shadower_t();
    private:
        env_t *shadow_env;
        std::map<int64_t, counted_t<const datum_t > *> current_scope;
    };

    // Get the current binding of a variable in the current scope.
    counted_t<const datum_t> *top_var(int var, const rcheckable_t *caller);
    // Unbind a variable in the current scope.
    void pop_var(int var);

    // Dump the current scope.
    void dump_scope(std::map<int64_t, counted_t<const datum_t> *> *out);
    // Swap in a previously-dumped scope.
    void push_scope(std::map<int64_t, Datum> *in);
    // Discard a previously-pushed scope and restore original scope.
    void pop_scope();
private:
    std::map<int64_t, std::stack<counted_t<const datum_t> *> > vars;
    std::stack<std::vector<std::pair<int, counted_t<const datum_t> > > > scope_stack;

public:
    // Implicit Variables (same interface as normal variables above).
    void push_implicit(counted_t<const datum_t> *val);
    counted_t<const datum_t> *top_implicit(const rcheckable_t *caller);
    void pop_implicit();

private:
    friend class implicit_binder_t;
    int implicit_depth;
    std::stack<counted_t<const datum_t> *> implicit_var;

public:
    // This is copied basically verbatim from old code.
    typedef namespaces_semilattice_metadata_t<rdb_protocol_t> ns_metadata_t;
    env_t(
        extproc::pool_group_t *_pool_group,
        base_namespace_repo_t<rdb_protocol_t> *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
        boost::shared_ptr<js::runner_t> _js_runner,
        signal_t *_interruptor,
        uuid_u _this_machine,
        const std::map<std::string, wire_func_t> &_optargs);

    explicit env_t(signal_t *);

    ~env_t();

    extproc::pool_t *pool;      // for running external JS jobs
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
        const cluster_semilattice_metadata_t &metadata_to_join)
        THROWS_ONLY(interrupted_exc_t);

    void throw_if_interruptor_pulsed() {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();
    }
private:
    // Ideally this would be a scoped_ptr_t<js::runner_t>. We used to copy
    // `runtime_environment_t` to capture scope, which is why this is a
    // `boost::shared_ptr`. But now we pass scope around separately, so this
    // could be changed.
    //
    // Note that js_runner is "lazily initialized": we only call
    // js_runner->begin() once we know we need to evaluate javascript. This
    // means we only allocate a worker process to queries that actually need
    // javascript execution.
    //
    // In the future we might want to be even finer-grained than this, and
    // release worker jobs once we know we no longer need JS execution, or
    // multiplex queries onto worker processes.
    boost::shared_ptr<js::runner_t> js_runner;

public:
    // Returns js_runner, but first calls js_runner->begin() if it hasn't
    // already been called.
    boost::shared_ptr<js::runner_t> get_js_runner();

    // This is a callback used in unittests to control things during a query
    class eval_callback_t {
    public:
        virtual ~eval_callback_t() { }
        virtual void eval_callback() = 0;
    };

    void set_eval_callback(eval_callback_t *callback);
    void do_eval_callback();

private:
    eval_callback_t *eval_callback;

public:
    signal_t *interruptor;
    uuid_u this_machine;

private:
    DISABLE_COPYING(env_t);
};

}  // namespace ql

#endif // RDB_PROTOCOL_ENV_HPP_
