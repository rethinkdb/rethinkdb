// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/env.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "extproc/js_runner.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}

func_cache_t::func_cache_t() { }

counted_t<func_t> func_cache_t::get_or_compile_func(env_t *env, const wire_func_t *wf) {
    env->assert_thread();
    auto it = cached_funcs.find(wf->uuid);
    if (it == cached_funcs.end()) {
        env->scopes.push_scope(&wf->scope);
        try {
            it = cached_funcs.insert(
                std::make_pair(
                    wf->uuid, compile_term(env, wf->source)->eval()->as_func(env))).first;
        } catch (const base_exc_t &e) {
            // If we have a non-`base_exc_t` exception, we don't want to pop the
            // scope because we might have corruption, and we might fail a check
            // and try to throw again while this exception is on the stack.  (We
            // only need to pop the scope for a `base_exc_t` exception because
            // that's the only kind of exception that we might recover from in
            // the ReQL layer, which is the only case where the un-popped scope
            // might matter.)
            // RSI: ^^^^ This is retarded, don't do this.
            env->scopes.pop_scope();
            throw;
        }
        env->scopes.pop_scope();
    }
    return it->second;
}

void func_cache_t::precache_func(const wire_func_t *wf, counted_t<func_t> func) {
    cached_funcs[wf->uuid] = func;
}

global_optargs_t::global_optargs_t() { }

global_optargs_t::global_optargs_t(const std::map<std::string, wire_func_t> &_optargs)
    : optargs(_optargs) { }

bool global_optargs_t::add_optarg(env_t *env, const std::string &key, const Term &val) {
    if (optargs.count(key)) {
        return true;
    }
    protob_t<Term> arg = make_counted_term();
    N2(FUNC, N0(MAKE_ARRAY), *arg = val);
    propagate_backtrace(arg.get(), &val.GetExtension(ql2::extension::backtrace));
    optargs[key] = wire_func_t(*arg, std::map<sym_t, Datum>());
    counted_t<func_t> force_compilation = optargs[key].compile(env);
    r_sanity_check(force_compilation.has());
    return false;
}

void global_optargs_t::init_optargs(env_t *env, const std::map<std::string, wire_func_t> &_optargs) {
    r_sanity_check(optargs.size() == 0);
    optargs = _optargs;
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        counted_t<func_t> force_compilation = it->second.compile(env);
        r_sanity_check(force_compilation.has());
    }
}
counted_t<val_t> global_optargs_t::get_optarg(env_t *env, const std::string &key){
    if (!optargs.count(key)) {
        return counted_t<val_t>();
    }
    return optargs[key].compile(env)->call();
}
const std::map<std::string, wire_func_t> &global_optargs_t::get_all_optargs() {
    return optargs;
}

static const int min_normal_gensym = -1000000;
sym_t gensym_t::gensym(bool allow_implicit) {
    r_sanity_check(0 > next_gensym_val && next_gensym_val >= min_normal_gensym);
    int64_t ret = next_gensym_val--;
    if (!allow_implicit) {
        ret += min_normal_gensym;
        r_sanity_check(ret < min_normal_gensym);
    }
    return sym_t(ret);
}

bool gensym_t::var_allows_implicit(sym_t varnum) {
    return varnum.value >= min_normal_gensym;
}


implicit_vars_t::implicit_vars_t() { }

void implicit_vars_t::push_implicit(counted_t<const datum_t> *val) {
    implicit_var.push(val);
}
counted_t<const datum_t> *implicit_vars_t::top_implicit(const rcheckable_t *caller) {
    rcheck_target(caller, base_exc_t::GENERIC, !implicit_var.empty(),
                  "r.row is not defined in this context.");
    rcheck_target(caller, base_exc_t::GENERIC, implicit_var.size() == 1,
                  "Cannot use r.row in nested queries.  Use functions instead.");
    return implicit_var.top();
}
void implicit_vars_t::pop_implicit() {
    r_sanity_check(implicit_var.size() > 0);
    implicit_var.pop();
}

scopes_t::scopes_t() { }

void scopes_t::push_var(sym_t var, counted_t<const datum_t> *val) {
    vars[var].push(val);
}

static counted_t<const datum_t> sindex_error_dummy_datum;
// RSI: This is probably terrible ^^^.
void scopes_t::push_special_var(sym_t var, special_var_t special_var) {
    switch (special_var) {
    case SINDEX_ERROR_VAR: {
        vars[var].push(&sindex_error_dummy_datum);
    } break;
    default: unreachable();
    }
}

scopes_t::special_var_shadower_t::special_var_shadower_t(
    scopes_t *scopes, special_var_t special_var) : shadow_scopes(scopes) {
    shadow_scopes->dump_scope(&current_scope);
    for (auto it = current_scope.begin(); it != current_scope.end(); ++it) {
        shadow_scopes->push_special_var(it->first, special_var);
    }
}

scopes_t::special_var_shadower_t::~special_var_shadower_t() {
    for (auto it = current_scope.begin(); it != current_scope.end(); ++it) {
        shadow_scopes->pop_var(it->first);
    }
}

counted_t<const datum_t> *scopes_t::top_var(sym_t var, const rcheckable_t *caller) {
    rcheck_target(caller, base_exc_t::GENERIC, !vars[var].empty(),
                  strprintf("Unrecognized variabled %" PRIi64, var.value));
    counted_t<const datum_t> *var_val = vars[var].top();
    rcheck_target(caller, base_exc_t::GENERIC,
                  var_val != &sindex_error_dummy_datum,
                  "Cannot reference external variables from inside an index.");
    return var_val;
}
void scopes_t::pop_var(sym_t var) {
    vars[var].pop();
}
void scopes_t::dump_scope(std::map<sym_t, counted_t<const datum_t> *> *out) {
    for (auto it = vars.begin(); it != vars.end(); ++it) {
        if (!it->second.empty()) {
            r_sanity_check(it->second.top());
            (*out)[it->first] = it->second.top();
        }
    }
}
void scopes_t::push_scope(const std::map<sym_t, Datum> *in) {
    scope_stack.push(std::vector<std::pair<sym_t, counted_t<const datum_t> > >());

    for (auto it = in->begin(); it != in->end(); ++it) {
        scope_stack.top().push_back(
            std::make_pair(it->first, make_counted<datum_t>(&it->second)));
    }

    for (size_t i = 0; i < scope_stack.top().size(); ++i) {
        push_var(scope_stack.top()[i].first, &scope_stack.top()[i].second);
    }
}
void scopes_t::pop_scope() {
    r_sanity_check(scope_stack.size() > 0);
    for (size_t i = 0; i < scope_stack.top().size(); ++i) {
        pop_var(scope_stack.top()[i].first);
    }
    // DO NOT pop the vector off the scope stack.  You might invalidate a
    // pointer too early.
    // RSI: ^^^ Pointers into a vector???
}

void env_t::set_eval_callback(eval_callback_t *callback) {
    eval_callback = callback;
}

void env_t::do_eval_callback() {
    if (eval_callback != NULL) {
        eval_callback->eval_callback();
    }
}

cluster_env_t::cluster_env_t(
        base_namespace_repo_t<rdb_protocol_t> *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager)
    : ns_repo(_ns_repo),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      databases_semilattice_metadata(_databases_semilattice_metadata),
      semilattice_metadata(_semilattice_metadata),
      directory_read_manager(_directory_read_manager) { }

void cluster_env_t::join_and_wait_to_propagate(
        const cluster_semilattice_metadata_t &metadata_to_join,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    cluster_semilattice_metadata_t sl_metadata;
    {
        on_thread_t switcher(semilattice_metadata->home_thread());
        semilattice_metadata->join(metadata_to_join);
        sl_metadata = semilattice_metadata->get();
    }

    boost::function<bool (const cow_ptr_t<ns_metadata_t> s)> p = boost::bind(
        &is_joined<cow_ptr_t<ns_metadata_t > >,
        _1,
        sl_metadata.rdb_namespaces
    );

    {
        on_thread_t switcher(namespaces_semilattice_metadata->home_thread());
        namespaces_semilattice_metadata->run_until_satisfied(p,
                                                             interruptor);
        databases_semilattice_metadata->run_until_satisfied(
            boost::bind(&is_joined<databases_semilattice_metadata_t>,
                        _1,
                        sl_metadata.databases),
            interruptor);
    }
}

js_runner_t *env_t::get_js_runner() {
    assert_thread();
    r_sanity_check(extproc_pool != NULL);
    if (!js_runner.connected()) {
        js_runner.begin(extproc_pool, interruptor);
    }
    return &js_runner;
}

env_t::env_t(
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
    const std::map<std::string, wire_func_t> &_optargs)
  : global_optargs(_optargs),
    extproc_pool(_extproc_pool),
    cluster_env(_ns_repo,
                _namespaces_semilattice_metadata,
                _databases_semilattice_metadata,
                _semilattice_metadata,
                _directory_read_manager),
    DEBUG_ONLY(eval_callback(NULL), )
    interruptor(_interruptor),
    this_machine(_this_machine) { }

// RSI: Do we really want people calling this constructor?
env_t::env_t(signal_t *_interruptor)
  : extproc_pool(NULL),
    cluster_env(NULL,
                clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >(),
                clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >(),
                boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >(),
                NULL),
    DEBUG_ONLY(eval_callback(NULL), )
    interruptor(_interruptor) { }

env_t::~env_t() { }

} // namespace ql
