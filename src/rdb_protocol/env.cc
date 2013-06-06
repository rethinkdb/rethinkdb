#include "rdb_protocol/env.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/term_walker.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}


bool env_t::add_optarg(const std::string &key, const Term &val) {
    if (optargs.count(key)) return true;
    protob_t<Term> arg = make_counted_term();
    N2(FUNC, N0(MAKE_ARRAY), *arg = val);
    propagate_backtrace(arg.get(), &val.GetExtension(ql2::extension::backtrace));
    optargs[key] = wire_func_t(*arg, std::map<int64_t, Datum>());
    counted_t<func_t> force_compilation = optargs[key].compile(this);
    r_sanity_check(force_compilation != NULL);
    return false;
}
void env_t::init_optargs(const std::map<std::string, wire_func_t> &_optargs) {
    r_sanity_check(optargs.size() == 0);
    optargs = _optargs;
    for (auto it = optargs.begin(); it != optargs.end(); ++it) {
        counted_t<func_t> force_compilation = it->second.compile(this);
        r_sanity_check(force_compilation != NULL);
    }
}
counted_t<val_t> env_t::get_optarg(const std::string &key){
    if (!optargs.count(key)) {
        return counted_t<val_t>();
    }
    return optargs[key].compile(this)->call();
}
const std::map<std::string, wire_func_t> &env_t::get_all_optargs() {
    return optargs;
}


static const int min_normal_gensym = -1000000;
int env_t::gensym(bool allow_implicit) {
    r_sanity_check(0 > next_gensym_val && next_gensym_val >= min_normal_gensym);
    int gensym = next_gensym_val--;
    if (!allow_implicit) {
        gensym += min_normal_gensym;
        r_sanity_check(gensym < min_normal_gensym);
    }
    return gensym;
}

bool env_t::var_allows_implicit(int varnum) {
    return varnum >= min_normal_gensym;
}

void env_t::push_implicit(counted_t<const datum_t> *val) {
    implicit_var.push(val);
}
counted_t<const datum_t> *env_t::top_implicit(const rcheckable_t *caller) {
    rcheck_target(caller, base_exc_t::GENERIC, !implicit_var.empty(),
                  "r.row is not defined in this context.");
    rcheck_target(caller, base_exc_t::GENERIC, implicit_var.size() == 1,
                  "Cannot use r.row in nested queries.  Use functions instead.");
    return implicit_var.top();
}
void env_t::pop_implicit() {
    r_sanity_check(implicit_var.size() > 0);
    implicit_var.pop();
}

void env_t::push_var(int var, counted_t<const datum_t> *val) {
    vars[var].push(val);
}

static counted_t<const datum_t> sindex_error_dummy_datum;
void env_t::push_special_var(int var, special_var_t special_var) {
    switch (special_var) {
    case SINDEX_ERROR_VAR: {
        vars[var].push(&sindex_error_dummy_datum);
    } break;
    default: unreachable();
    }
}

env_t::special_var_shadower_t::special_var_shadower_t(
    env_t *env, special_var_t special_var) : shadow_env(env) {
    shadow_env->dump_scope(&current_scope);
    for (auto it = current_scope.begin(); it != current_scope.end(); ++it) {
        shadow_env->push_special_var(it->first, special_var);
    }
}

env_t::special_var_shadower_t::~special_var_shadower_t() {
    for (auto it = current_scope.begin(); it != current_scope.end(); ++it) {
        shadow_env->pop_var(it->first);
    }
}

counted_t<const datum_t> *env_t::top_var(int var, const rcheckable_t *caller) {
    rcheck_target(caller, base_exc_t::GENERIC, !vars[var].empty(),
                  strprintf("Unrecognized variabled %d", var));
    counted_t<const datum_t> *var_val = vars[var].top();
    rcheck_target(caller, base_exc_t::GENERIC,
                  var_val != &sindex_error_dummy_datum,
                  "Cannot reference external variables from inside an index.");
    return var_val;
}
void env_t::pop_var(int var) {
    vars[var].pop();
}
void env_t::dump_scope(std::map<int64_t, counted_t<const datum_t> *> *out) {
    for (std::map<int64_t, std::stack<counted_t<const datum_t> *> >::iterator
             it = vars.begin(); it != vars.end(); ++it) {
        if (it->second.size() == 0) continue;
        r_sanity_check(it->second.top());
        (*out)[it->first] = it->second.top();
    }
}
void env_t::push_scope(std::map<int64_t, Datum> *in) {
    scope_stack.push(std::vector<std::pair<int, counted_t<const datum_t> > >());

    for (std::map<int64_t, Datum>::iterator it = in->begin(); it != in->end(); ++it) {
        scope_stack.top().push_back(std::make_pair(it->first,
                                                   make_counted<datum_t>(&it->second,
                                                                         this)));
    }

    for (size_t i = 0; i < scope_stack.top().size(); ++i) {
        //        &scope_stack.top()[i].second,
        //        scope_stack.top()[i].second);
        push_var(scope_stack.top()[i].first, &scope_stack.top()[i].second);
    }
}
void env_t::pop_scope() {
    r_sanity_check(scope_stack.size() > 0);
    for (size_t i = 0; i < scope_stack.top().size(); ++i) {
        pop_var(scope_stack.top()[i].first);
    }
    // DO NOT pop the vector off the scope stack.  You might invalidate a
    // pointer too early.
}

void env_t::set_eval_callback(eval_callback_t *callback) {
    eval_callback = callback;
}

void env_t::do_eval_callback() {
    if (eval_callback != NULL) {
        eval_callback->eval_callback();
    }
}

void env_t::join_and_wait_to_propagate(
    const cluster_semilattice_metadata_t &metadata_to_join)
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

boost::shared_ptr<js::runner_t> env_t::get_js_runner() {
    r_sanity_check(pool != NULL && get_thread_id() == pool->home_thread());
    if (!js_runner->connected()) {
        js_runner->begin(pool);
    }
    return js_runner;
}

env_t::env_t(
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
    const std::map<std::string, wire_func_t> &_optargs)
  : uuid(generate_uuid()),
    optargs(_optargs),
    next_gensym_val(-2),
    implicit_depth(0),
    pool(_pool_group->get()),
    ns_repo(_ns_repo),
    namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
    databases_semilattice_metadata(_databases_semilattice_metadata),
    semilattice_metadata(_semilattice_metadata),
    directory_read_manager(_directory_read_manager),
    js_runner(_js_runner),
    DEBUG_ONLY(eval_callback(NULL), )
    interruptor(_interruptor),
    this_machine(_this_machine) {

    guarantee(js_runner);
}

env_t::env_t(signal_t *_interruptor)
  : uuid(generate_uuid()),
    next_gensym_val(-2),
    implicit_depth(0),
    pool(NULL),
    ns_repo(NULL),
    directory_read_manager(NULL),
    DEBUG_ONLY(eval_callback(NULL), )
    interruptor(_interruptor) { }

env_t::~env_t() { }

} // namespace ql
