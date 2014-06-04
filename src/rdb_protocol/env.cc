// Copyright 2010-2014 RethinkDB, all rights reserved.
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

wire_func_t construct_optarg_wire_func(const Term &val) {
    protob_t<Term> arg = r::fun(r::expr(val)).release_counted();
    propagate_backtrace(arg.get(), &val.GetExtension(ql2::extension::backtrace));

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term
        = make_counted<func_term_t>(&empty_compile_env, arg);
    counted_t<func_t> func = func_term->eval_to_func(var_scope_t());
    return wire_func_t(func);
}

std::map<std::string, wire_func_t> global_optargs(protob_t<Query> q) {
    rassert(q.has());

    Term *t = q->mutable_query();
    preprocess_term(t);

    std::map<std::string, wire_func_t> optargs;

    for (int i = 0; i < q->global_optargs_size(); ++i) {
        const Query::AssocPair &ap = q->global_optargs(i);
        if (optargs.count(ap.key())) {
            rfail_toplevel(
                    base_exc_t::GENERIC,
                    "Duplicate global optarg: %s", ap.key().c_str());
        }
        optargs[ap.key()] = construct_optarg_wire_func(ap.val());
    }

    // Supply a default db of "test" if there is no "db" optarg.
    if (!optargs.count("db")) {
        Term arg = r::db("test").get();
        Backtrace *t_bt = t->MutableExtension(ql2::extension::backtrace);
        propagate_backtrace(&arg, t_bt); // duplicate toplevel backtrace
        optargs["db"] = construct_optarg_wire_func(arg);
    }

    return optargs;
}

global_optargs_t::global_optargs_t() { }

global_optargs_t::global_optargs_t(std::map<std::string, wire_func_t> _optargs)
    : optargs(_optargs) { }

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

        clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
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

    boost::function<bool (const cow_ptr_t<namespaces_semilattice_metadata_t> s)> p
        = boost::bind(&is_joined<cow_ptr_t<namespaces_semilattice_metadata_t > >,
                      _1,
                      sl_metadata.rdb_namespaces);

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

// Used in constructing the env for unsharding.
env_t::env_t(rdb_context_t *ctx, signal_t *_interruptor)
    : evals_since_yield(0),
      global_optargs(),
      extproc_pool(ctx != NULL ? ctx->extproc_pool : NULL),
      changefeed_client(ctx != NULL && ctx->changefeed_client.has()
                        ? ctx->changefeed_client.get()
                        : NULL),
      reql_http_proxy(ctx != NULL ? ctx->reql_http_proxy : ""),
      cluster_access(
          ctx != NULL ? ctx->ns_repo : NULL,
          ctx != NULL
              ? ctx->cross_thread_namespace_watchables[get_thread_id().threadnum].get()
                  ->get_watchable()
              : clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >(),
          ctx != NULL
              ? ctx->cross_thread_database_watchables[get_thread_id().threadnum].get()
                  ->get_watchable()
              : clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >(),
          ctx != NULL
              ? ctx->cluster_metadata
              : boost::shared_ptr<
                    semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >(),
          NULL,
          ctx != NULL ? ctx->machine_id : uuid_u()),
      interruptor(_interruptor),
      eval_callback(NULL) {
    rassert(interruptor != NULL);
}


// Used in constructing the env for unsharding.  Used in constructing the env for
// rdb_update_single_sindex.  Used in unittest/rdb_btree.cc.  Used in
// rdb_backfill.cc.
env_t::env_t(signal_t *_interruptor)
    : evals_since_yield(0),
      global_optargs(),
      extproc_pool(NULL),
      changefeed_client(NULL),
      reql_http_proxy(""),
      cluster_access(
          NULL,
          clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >(),
          clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >(),
          boost::shared_ptr<
              semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >(),
          NULL,
          uuid_u()),
      interruptor(_interruptor),
      eval_callback(NULL) {
    rassert(interruptor != NULL);
}

profile_bool_t profile_bool_optarg(const protob_t<Query> &query) {
    rassert(query.has());
    counted_t<const datum_t> profile_arg = static_optarg("profile", query);
    if (profile_arg.has() && profile_arg->get_type() == datum_t::type_t::R_BOOL &&
        profile_arg->as_bool()) {
        return profile_bool_t::PROFILE;
    } else {
        return profile_bool_t::DONT_PROFILE;
    }
}

// Called by run (in term.cc, the parser's query-running method).  This time _with_ a
// directory_read_manager.
env_t::env_t(
    extproc_pool_t *_extproc_pool,
    changefeed::client_t *_changefeed_client,
    const std::string &_reql_http_proxy,
    base_namespace_repo_t *_ns_repo,
    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
        _namespaces_semilattice_metadata,
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
        _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
    signal_t *_interruptor,
    uuid_u _this_machine,
    std::map<std::string, wire_func_t> optargs,
    profile_bool_t profile)
  : evals_since_yield(0),
    global_optargs(std::move(optargs)),
    extproc_pool(_extproc_pool),
    changefeed_client(_changefeed_client),
    reql_http_proxy(_reql_http_proxy),
    cluster_access(_ns_repo,
                   _namespaces_semilattice_metadata,
                   _databases_semilattice_metadata,
                   _semilattice_metadata,
                   _directory_read_manager,
                   _this_machine),
    interruptor(_interruptor),
    trace(profile == profile_bool_t::PROFILE
          ? make_scoped<profile::trace_t>()
          : scoped_ptr_t<profile::trace_t>()),
    eval_callback(NULL) {
    rassert(interruptor != NULL);
}

// Called by rdb_write_visitor_t, with a _possibly_ NULL changefeed client (fucking
// Christ, what the fuck is this shit).  Called by unittest/rdb_env.cc, with a NULL
// changefeed_client.
env_t::env_t(
    extproc_pool_t *_extproc_pool,
    changefeed::client_t *_changefeed_client,
    const std::string &_reql_http_proxy,
    base_namespace_repo_t *_ns_repo,
    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
        _namespaces_semilattice_metadata,
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
        _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    signal_t *_interruptor,
    uuid_u _this_machine)
  : evals_since_yield(0),
    global_optargs(),
    extproc_pool(_extproc_pool),
    changefeed_client(_changefeed_client),
    reql_http_proxy(_reql_http_proxy),
    cluster_access(_ns_repo,
                   _namespaces_semilattice_metadata,
                   _databases_semilattice_metadata,
                   _semilattice_metadata,
                   NULL,
                   _this_machine),
    interruptor(_interruptor),
    trace(),
    eval_callback(NULL) {
    rassert(interruptor != NULL);
}

// Used to construct the env on the store_t (for reads).
env_t::env_t(
    extproc_pool_t *_extproc_pool,
    changefeed::client_t *_changefeed_client,
    const std::string &_reql_http_proxy,
    base_namespace_repo_t *_ns_repo,

    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
        _namespaces_semilattice_metadata,

    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
         _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    signal_t *_interruptor,
    uuid_u _this_machine,
    profile_bool_t _profile)
  : evals_since_yield(0),
    global_optargs(),
    extproc_pool(_extproc_pool),
    changefeed_client(_changefeed_client),
    reql_http_proxy(_reql_http_proxy),
    cluster_access(_ns_repo,
                   _namespaces_semilattice_metadata,
                   _databases_semilattice_metadata,
                   _semilattice_metadata,
                   NULL,
                   _this_machine),
    interruptor(_interruptor),
    trace(_profile == profile_bool_t::PROFILE
          ? make_scoped<profile::trace_t>()
          : scoped_ptr_t<profile::trace_t>()),
    eval_callback(NULL) {
    rassert(interruptor != NULL);
}

env_t::~env_t() { }

void env_t::maybe_yield() {
    if (++evals_since_yield > EVALS_BEFORE_YIELD) {
        evals_since_yield = 0;
        coro_t::yield();
    }
}

// Constructs a fully-functional env_t, for use in term.cc and stream cache code.
// (Eventually this should be a constructor.  The env_t constructor that takes an
// rdb_context_t currently uses NULL instead of the directory_read_manager for some
// reason, and I'm in too much of a rush to figure out why.)
scoped_ptr_t<env_t> make_complete_env(rdb_context_t *ctx,
                                      signal_t *interruptor,
                                      std::map<std::string, wire_func_t> optargs,
                                      profile_bool_t profile) {
    const threadnum_t th = get_thread_id();
    return make_scoped<ql::env_t>(
            ctx->extproc_pool,
            ctx->changefeed_client.get(),
            ctx->reql_http_proxy,
            ctx->ns_repo,
            ctx->cross_thread_namespace_watchables[th.threadnum]->get_watchable(),
            ctx->cross_thread_database_watchables[th.threadnum]->get_watchable(),
            ctx->cluster_metadata,
            ctx->directory_read_manager,
            interruptor,
            ctx->machine_id,
            std::move(optargs),
            profile);
}

} // namespace ql
