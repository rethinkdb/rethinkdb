// Copyright 2010-2012 RethinkDB, all rights reserved.
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
#include "containers/ptr_bag.hpp"
#include "extproc/pool.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {
class term_t;

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}

class env_t : private home_thread_mixin_t {
public:
    // returns whether or not there was a key conflict
    MUST_USE bool add_optarg(const std::string &key, const Term &val);
    void init_optargs(const std::map<std::string, wire_func_t> &_optargs);
    val_t *get_optarg(const std::string &key); // returns NULL if no entry
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
    void push_var(int var, const datum_t **val);
    // Get the current binding of a variable in the current scope.
    const datum_t **top_var(int var, const rcheckable_t *caller);
    // Unbind a variable in the current scope.
    void pop_var(int var);

    // Dump the current scope.
    void dump_scope(std::map<int64_t, const datum_t **> *out);
    // Swap in a previously-dumped scope.
    void push_scope(std::map<int64_t, Datum> *in);
    // Discard a previously-pushed scope and restore original scope.
    void pop_scope();
private:
    std::map<int64_t, std::stack<const datum_t **> > vars;
    std::stack<std::vector<std::pair<int, const datum_t *> > > scope_stack;

public:
    // Implicit Variables (same interface as normal variables above).
    void push_implicit(const datum_t **val);
    const datum_t **top_implicit(const rcheckable_t *caller);
    void pop_implicit();
private:
    friend class implicit_binder_t;
    int implicit_depth;
    std::stack<const datum_t **> implicit_var;

    // Allocation Functions
public:
    template<class T>
    T *add_ptr(T *p) {
        assert_thread();
        r_sanity_check(bags.size() > 0);
        if (some_bag_has(p)) return p;
        bags[bags.size()-1]->add(p);
        return p;
    }
    func_t *new_func(const Term *term) {
        return add_ptr(new func_t(this, term));
    }
    template<class T>
    val_t *new_val(T *ptr, term_t *parent) {
        return add_ptr(new val_t(add_ptr(ptr), parent, this));
    }
    template<class T, class U>
    val_t *new_val(T *ptr, U *ptr2, term_t *parent) {
        return add_ptr(new val_t(add_ptr(ptr), add_ptr(ptr2), parent, this));
    }
    val_t *new_val(uuid_u db, term_t *parent) {
        return add_ptr(new val_t(db, parent, this));
    }
    term_t *new_term(const Term *source) {
        return add_ptr(compile_term(this, source));
    }

    // Checkpoint code (most of the logic is in `env_checkpoint_t` and
    // env_gc_checkpoint_t` below).  Checkpoints should be created by
    // constructing a `*_checkpoint_t`, which is why the `checkpoint` function
    // is private.
public:
    void merge_checkpoint(); // Merge in all allocations since checkpoint
    void discard_checkpoint(); // Discard all allocations since checkpoint
private:
    void checkpoint(); // create a new checkpoint
    friend class env_checkpoint_t;
    friend class env_gc_checkpoint_t;
    size_t num_checkpoints(); // number of checkpoints
    bool some_bag_has(const ptr_baggable_t *p);

private:
    // `old_bag` and `new_bag` are so that `gc` can communicate with `gc_callback`.
    ptr_bag_t *old_bag, *new_bag;
    class gc_callback_caller_t {
    public:
        gc_callback_caller_t(env_t *_env) : env(_env) { }
        bool operator()(const datum_t *el) { return env->gc_callback(el); }
    private:
        env_t *env;
    };
    bool gc_callback(const datum_t *el);
    void gc(const datum_t *root);

    ptr_bag_t *current_bag(); // gets the top bag
    ptr_bag_t **current_bag_ptr();
    std::vector<ptr_bag_t *> bags;

public:
    // This is copied basically verbatim from old code.
    typedef namespaces_semilattice_metadata_t<rdb_protocol_t> ns_metadata_t;
    env_t(
        extproc::pool_group_t *_pool_group,
        namespace_repo_t<rdb_protocol_t> *_ns_repo,

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
    ~env_t();

    extproc::pool_t *pool;      // for running external JS jobs
    namespace_repo_t<rdb_protocol_t> *ns_repo;

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
    //TODO(bill) should the implementation of this go into a different file?
    boost::shared_ptr<js::runner_t> get_js_runner() {
        pool->assert_thread();
        if (!js_runner->connected()) {
            js_runner->begin(pool);
        }
        return js_runner;
    }

    signal_t *interruptor;
    uuid_u this_machine;

private:
    DISABLE_COPYING(env_t);
};

// Construct this to checkpoint the environment.  You should pass it a pointer
// to either `env_t::merge_checkpoint` or `env_t::discard_checkpoint`, which
// will be its default action when deconstructed.  You can change this action
// with `reset`.
class env_checkpoint_t {
public:
    enum destructor_op_t { MERGE, DISCARD };
    env_checkpoint_t(env_t *_env, destructor_op_t _destructor_op);
    ~env_checkpoint_t();
    void reset(destructor_op_t new_destructor_op);
    // This will garbage-collect the checkpoint so that only `root` and data it
    // points to remain.
    void gc(const datum_t *root);
private:
    env_t *env;
    destructor_op_t destructor_op;
};

// This is a checkpoint (as above) that also does shitty generational garbage
// collection for `reduce` and `gmr` queries.
class env_gc_checkpoint_t {
    static const int DEFAULT_GEN1_CUTOFF;
    static const int DEFAULT_GEN2_SIZE_MULTIPLIER;
public:
    env_gc_checkpoint_t(env_t *_env, size_t _gen1 = 0, size_t _gen2 = 0);
    ~env_gc_checkpoint_t();
    const datum_t *maybe_gc(const datum_t *root);
    const datum_t *finalize(const datum_t *root);
private:
    bool finalized;
    env_t *env;
    size_t gen1;
    size_t gen2;
};

template<class T>
struct env_wrapper_t : public ptr_baggable_t {
    T t;
};

} // ql

#endif // RDB_PROTOCOL_ENV_HPP_
