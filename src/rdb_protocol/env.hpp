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
#include "extproc/js_runner.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/val.hpp"

class extproc_pool_t;

namespace ql {
class datum_t;
class term_t;

class func_cache_t {
public:
    func_cache_t();
    counted_t<func_t> get_or_compile_func(env_t *env, const wire_func_t *wf);
    void precache_func(const wire_func_t *wf, counted_t<func_t> func);
private:
    std::map<uuid_u, counted_t<func_t> > cached_funcs;
    DISABLE_COPYING(func_cache_t);
};

class global_optargs_t {
public:
    global_optargs_t();
    global_optargs_t(const std::map<std::string, wire_func_t> &_optargs);

    // RSI: Why is there an add_optarg function?
    // Returns whether or not there was a key conflict.
    MUST_USE bool add_optarg(env_t *env, const std::string &key, const Term &val);
    void init_optargs(env_t *env, const std::map<std::string, wire_func_t> &_optargs);
    counted_t<val_t> get_optarg(env_t *env, const std::string &key); // returns NULL if no entry
    const std::map<std::string, wire_func_t> &get_all_optargs();
private:
    std::map<std::string, wire_func_t> optargs;
};

class gensym_t {
public:
    gensym_t() : next_gensym_val(-2) { }
    // Returns a globally unique variable.
    sym_t gensym(bool allow_implicit = false);
    static bool var_allows_implicit(sym_t varnum);
private:
    int64_t next_gensym_val; // always negative
    DISABLE_COPYING(gensym_t);
};

class scopes_t {
public:
    scopes_t();

    // Bind a variable in the current scope.
    void push_var(sym_t var, counted_t<const datum_t> *val);

    // Special variables have unusual behavior.
    enum special_var_t {
        SINDEX_ERROR_VAR = 1, // Throws an error when you try to access it.
    };
    // Push a special variable.  (Pop it off with the normal `pop_var`.)
    void push_special_var(sym_t var, special_var_t special_var);
    // This will push a special variable over ever variable currently in scope
    // when constructed, then pop those variables when destructed.
    class special_var_shadower_t {
    public:
        special_var_shadower_t(env_t *env, special_var_t special_var);
        ~special_var_shadower_t();
    private:
        env_t *shadow_env;
        std::map<sym_t, counted_t<const datum_t > *> current_scope;
    };

    // Get the current binding of a variable in the current scope.
    counted_t<const datum_t> *top_var(sym_t var, const rcheckable_t *caller);
    // Unbind a variable in the current scope.
    void pop_var(sym_t var);

    // Dump the current scope.
    void dump_scope(std::map<sym_t, counted_t<const datum_t> *> *out);
    // Swap in a previously-dumped scope.
    void push_scope(const std::map<sym_t, Datum> *in);
    // Discard a previously-pushed scope and restore original scope.
    void pop_scope();

private:
    std::map<sym_t, std::stack<counted_t<const datum_t> *> > vars;
    std::stack<std::vector<std::pair<sym_t, counted_t<const datum_t> > > > scope_stack;
    DISABLE_COPYING(scopes_t);
};

class implicit_vars_t {
public:
    implicit_vars_t();
    // Implicit Variables (same interface as normal variables above).
    void push_implicit(counted_t<const datum_t> *val);
    counted_t<const datum_t> *top_implicit(const rcheckable_t *caller);
    void pop_implicit();

private:
    friend class implicit_binder_t;
    int implicit_depth;
    std::stack<counted_t<const datum_t> *> implicit_var;
    DISABLE_COPYING(implicit_vars_t);
};

class cluster_env_t {
public:
    typedef namespaces_semilattice_metadata_t<rdb_protocol_t> ns_metadata_t;
    cluster_env_t(
        base_namespace_repo_t<rdb_protocol_t> *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager);

    base_namespace_repo_t<rdb_protocol_t> *ns_repo;

    clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t > > >
        namespaces_semilattice_metadata;
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
        databases_semilattice_metadata;
    // TODO this should really just be the namespace metadata... but
    // constructing views is too hard :-/
    // RSI: Constructing views isn't hard, construct a view.
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        semilattice_metadata;
    directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;

    // Semilattice modification functions
    void join_and_wait_to_propagate(
            const cluster_semilattice_metadata_t &metadata_to_join,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);
};

class env_t : public home_thread_mixin_t {
public:
    func_cache_t func_cache;
    global_optargs_t global_optargs;
    gensym_t symgen;

    scopes_t scopes;
    implicit_vars_t implicits;

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

    extproc_pool_t *extproc_pool;      // for running external JS jobs

    cluster_env_t cluster_env;

    void throw_if_interruptor_pulsed() THROWS_ONLY(interrupted_exc_t) {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();
    }
private:
    js_runner_t js_runner;

public:
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

private:
    eval_callback_t *eval_callback;

public:
    // RSI: This variable isn't const, thanks to stream_cache2.cc.  It should be
    // const.
    signal_t *interruptor;
    const uuid_u this_machine;

private:
    DISABLE_COPYING(env_t);
};

}  // namespace ql

#endif // RDB_PROTOCOL_ENV_HPP_
