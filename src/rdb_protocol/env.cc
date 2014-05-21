// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/env.hpp"

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "extproc/js_runner.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}

counted_t<const datum_t> static_optarg(const std::string &key, protob_t<Query> q) {
    for (int i = 0; i < q->global_optargs_size(); ++i) {
        const Query::AssocPair &ap = q->global_optargs(i);
        if (ap.key() == key && ap.val().type() == Term_TermType_DATUM) {
            return make_counted<const datum_t>(&ap.val().datum());
        }
    }

    return counted_t<const datum_t>();
}

global_optargs_t::global_optargs_t() { }

global_optargs_t::global_optargs_t(protob_t<Query> q) {
    if (!q.has()) {
        return;
    }
    Term *t = q->mutable_query();
    preprocess_term(t);

    for (int i = 0; i < q->global_optargs_size(); ++i) {
        const Query::AssocPair &ap = q->global_optargs(i);
        bool conflict = add_optarg(ap.key(), ap.val());
        rcheck_toplevel(
                !conflict, base_exc_t::GENERIC,
                strprintf("Duplicate global optarg: %s", ap.key().c_str()));
    }

    Term arg = r::db("test").get();

    Backtrace *t_bt = t->MutableExtension(ql2::extension::backtrace);
    propagate_backtrace(&arg, t_bt); // duplicate toplevel backtrace
    UNUSED bool _b = add_optarg("db", arg);
    //          ^^ UNUSED because user can override this value safely
}

bool global_optargs_t::add_optarg(const std::string &key, const Term &val) {
    if (optargs.count(key)) {
        return true;
    }
    protob_t<Term> arg = r::fun(r::expr(val)).release_counted();
    propagate_backtrace(arg.get(), &val.GetExtension(ql2::extension::backtrace));

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term
        = make_counted<func_term_t>(&empty_compile_env, arg);
    counted_t<func_t> func = func_term->eval_to_func(var_scope_t());

    optargs[key] = wire_func_t(func);
    return false;
}

void global_optargs_t::init_optargs(
    const std::map<std::string, wire_func_t> &_optargs) {
    r_sanity_check(optargs.size() == 0);
    optargs = _optargs;
}

counted_t<val_t> global_optargs_t::get_optarg(env_t *env, const std::string &key){
    if (!optargs.count(key)) {
        return counted_t<val_t>();
    }
    return optargs[key].compile_wire_func()->call(env);
}
const std::map<std::string, wire_func_t> &global_optargs_t::get_all_optargs() {
    return optargs;
}

void env_t::set_eval_callback(eval_callback_t *callback) {
    eval_callback = callback;
}

void env_t::do_eval_callback() {
    if (eval_callback != NULL) {
        eval_callback->eval_callback();
    }
}

profile_bool_t env_t::profile() {
    return trace.has() ? profile_bool_t::PROFILE : profile_bool_t::DONT_PROFILE;
}

cluster_access_t::cluster_access_t(
        base_namespace_repo_t *_ns_repo,

        clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
            _namespaces_semilattice_metadata,

        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
        uuid_u _this_machine)
    : ns_repo(_ns_repo),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      databases_semilattice_metadata(_databases_semilattice_metadata),
      semilattice_metadata(_semilattice_metadata),
      directory_read_manager(_directory_read_manager),
      this_machine(_this_machine) { }

void cluster_access_t::join_and_wait_to_propagate(
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

env_t::env_t(rdb_context_t *ctx, signal_t *_interruptor)
    : evals_since_yield(0),
      global_optargs(protob_t<Query>()),
      extproc_pool(ctx ? ctx->extproc_pool : NULL),
      reql_http_proxy(ctx ? ctx->reql_http_proxy : ""),
      cluster_access(
          ctx ? ctx->ns_repo : NULL,
          ctx ? ctx->cross_thread_namespace_watchables[get_thread_id().threadnum].get()
                  ->get_watchable()
              : clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >(),
          ctx ? ctx->cross_thread_database_watchables[get_thread_id().threadnum].get()
                  ->get_watchable()
              : clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >(),
          ctx ? ctx->cluster_metadata
              : boost::shared_ptr<
                    semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >(),
          NULL,
          ctx ? ctx->machine_id : uuid_u()),
      interruptor(_interruptor),
      eval_callback(NULL) { }

env_t::env_t(
    extproc_pool_t *_extproc_pool,
    const std::string &_reql_http_proxy,
    base_namespace_repo_t *_ns_repo,
    clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
        _namespaces_semilattice_metadata,
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
        _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
    signal_t *_interruptor,
    uuid_u _this_machine,
    protob_t<Query> query)
  : evals_since_yield(0),
    global_optargs(query),
    extproc_pool(_extproc_pool),
    reql_http_proxy(_reql_http_proxy),
    cluster_access(_ns_repo,
                   _namespaces_semilattice_metadata,
                   _databases_semilattice_metadata,
                   _semilattice_metadata,
                   _directory_read_manager,
                   _this_machine),
    interruptor(_interruptor),
    eval_callback(NULL)
{
    if (query.has()) {
        counted_t<const datum_t> profile_arg = static_optarg("profile", query);
        if (profile_arg.has() && profile_arg->get_type() == datum_t::type_t::R_BOOL &&
            profile_arg->as_bool()) {
            trace.init(new profile::trace_t());
        }
    }
}

env_t::env_t(
    extproc_pool_t *_extproc_pool,
    const std::string &_reql_http_proxy,
    base_namespace_repo_t *_ns_repo,

    clone_ptr_t<watchable_t<cow_ptr_t<ns_metadata_t> > >
        _namespaces_semilattice_metadata,

    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
         _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
    signal_t *_interruptor,
    uuid_u _this_machine,
    profile_bool_t _profile)
  : evals_since_yield(0),
    global_optargs(protob_t<Query>()),
    extproc_pool(_extproc_pool),
    reql_http_proxy(_reql_http_proxy),
    cluster_access(_ns_repo,
                   _namespaces_semilattice_metadata,
                   _databases_semilattice_metadata,
                   _semilattice_metadata,
                   _directory_read_manager,
                   _this_machine),
    interruptor(_interruptor),
    eval_callback(NULL)
{
    if (_profile == profile_bool_t::PROFILE) {
        trace.init(new profile::trace_t());
    }
}

env_t::~env_t() { }

void env_t::maybe_yield() {
    if (++evals_since_yield > EVALS_BEFORE_YIELD) {
        coro_t::yield();
        evals_since_yield = 0;
    }
}

} // namespace ql
