// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ENV_HPP_
#define RDB_PROTOCOL_ENV_HPP_

#include <map>
#include <stack>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/one_per_thread.hpp"
#include "containers/ptr_bag.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {
class term_t;

class env_t {
public:
    int gensym() {
        r_sanity_check(next_gensym_val > -100000);
        return next_gensym_val--;
    }

private:
    int next_gensym_val;

public:
    void push_implicit(const datum_t **val) {
        implicit_var.push(val);
    }
    const datum_t **top_implicit() {
        rcheck(!implicit_var.empty(), "No implicit variable in scope.");
        rcheck(implicit_var.size() == 1,
               "Cannot use implicit variable in nested queries; name your variables.");
        return implicit_var.top();
    }
    void pop_implicit() {
        implicit_var.pop();
    }
private:
    friend class implicit_binder_t;
    int implicit_depth;
    std::stack<const datum_t **> implicit_var;

public:
    template<class T>
    T *add_ptr(T *p) {
        r_sanity_check(bags.size() > 0);
        for (size_t i = 0; i < bags.size(); ++i) {
            if (bags[i]->has(p)) return p;
        }
        bags[bags.size()-1]->add(p);
        return p;
    }
    func_t *new_func(const Term2 *term) {
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
    val_t *new_val(uuid_t db, term_t *parent) {
        return add_ptr(new val_t(db, parent, this));
    }
    term_t *new_term(const Term2 *source) {
        return add_ptr(compile_term(this, source));
    }

    size_t num_checkpoints() {
        return bags.size()-1;
    }
private:
    friend class env_checkpointer_t;
    void checkpoint() {
        bags.push_back(new ptr_bag_t());
    }
public:
    void merge_checkpoint() {
        r_sanity_check(bags.size() >= 2);
        bags[bags.size()-2]->add(bags[bags.size()-1]);
        bags.pop_back();
    }
    void discard_checkpoint() {
        r_sanity_check(bags.size() >= 2);
        delete bags[bags.size()-1];
        bags.pop_back();
    }
    ~env_t() {
        guarantee(bags.size() == 1);
        delete bags[0];
    }
private:
    ptr_bag_t *get_bag() {
        r_sanity_check(bags.size() > 0);
        return bags[bags.size()-1];
    }
    std::vector<ptr_bag_t *> bags;

public:
    void push_var(int var, const datum_t **val) {
        vars[var].push(val);
    }
    const datum_t **top_var(int var) {
        rcheck(!vars[var].empty(), strprintf("Unrecognized variabled %d", var));
        return vars[var].top();
    }
    void pop_var(int var) { vars[var].pop(); }
    const datum_t **get_var(int var) { return vars[var].top(); }
    void dump_scope(std::map<int, Datum> *out) {
        for (std::map<int, std::stack<const datum_t **> >::iterator
                 it = vars.begin(); it != vars.end(); ++it) {
            if (it->second.size() == 0) continue;
            r_sanity_check(it->second.top());
            rcheck(*it->second.top(),
                   strprintf("Variable %d was never bound!  (Probably a client error.)",
                             it->first));
            (*it->second.top())->write_to_protobuf(&(*out)[it->first]);
        }
    }
private:
    std::map<int, std::stack<const datum_t **> > vars;

public:
    env_t(
        extproc::pool_group_t *_pool_group,
        namespace_repo_t<rdb_protocol_t> *_ns_repo,
        clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > >
             _namespaces_semilattice_metadata,
        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
        boost::shared_ptr<js::runner_t> _js_runner,
        signal_t *_interruptor,
        uuid_t _this_machine)
        : next_gensym_val(-2),
          implicit_depth(0),
          pool(_pool_group->get()),
          ns_repo(_ns_repo),
          namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
          databases_semilattice_metadata(_databases_semilattice_metadata),
          semilattice_metadata(_semilattice_metadata),
          directory_read_manager(_directory_read_manager),
          js_runner(_js_runner),
          interruptor(_interruptor),
          this_machine(_this_machine) {
        guarantee(js_runner);
        bags.push_back(new ptr_bag_t());
    }

    extproc::pool_t *pool;      // for running external JS jobs
    namespace_repo_t<rdb_protocol_t> *ns_repo;

    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > namespaces_semilattice_metadata;
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> > databases_semilattice_metadata;
    //TODO this should really just be the namespace metadata... but
    //constructing views is too hard :-/
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        semilattice_metadata;
    directory_read_manager_t<cluster_directory_metadata_t> *directory_read_manager;

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

    signal_t *interruptor;
    uuid_t this_machine;

private:
    DISABLE_COPYING(env_t);
};

class env_checkpointer_t {
public:
    env_checkpointer_t(env_t *_env, void (env_t::*_f)()) : env(_env) , f(_f) {
        env->checkpoint();
    }
    ~env_checkpointer_t() { (env->*f)(); }
    void reset(void (env_t::*_f)()) { f = _f; }
private:
    env_t *env;
    void (env_t::*f)();
};

} // ql

#endif // RDB_PROTOCOL_ENV_HPP_
