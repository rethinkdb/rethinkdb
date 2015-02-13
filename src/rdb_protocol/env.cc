// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/env.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/cross_thread_watchable.hpp"
#include "extproc/js_runner.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term_walker.hpp"

#include "debug.hpp"

// This is a totally arbitrary constant limiting the size of the regex cache.  1000
// was chosen out of a hat; if you have a good argument for it being something else
// (apart from cache line concerns, which are irrelevant due to the implementation)
// you're probably right.
const size_t LRU_CACHE_SIZE = 1000;

namespace ql {

datum_t static_optarg(const std::string &key, protob_t<Query> q) {
    // need to parse these to figure out what user wants; resulting
    // bootstrap problem is a headache.  Just use default.
    const configured_limits_t limits;
    for (int i = 0; i < q->global_optargs_size(); ++i) {
        const Query::AssocPair &ap = q->global_optargs(i);
        if (ap.key() == key && ap.val().type() == Term_TermType_DATUM) {
            return to_datum(&ap.val().datum(), limits, reql_version_t::LATEST);
        }
    }

    return datum_t();
}

wire_func_t construct_optarg_wire_func(const Term &val) {
    protob_t<Term> arg = r::fun(r::expr(val)).release_counted();
    propagate_backtrace(arg.get(), &val.GetExtension(ql2::extension::backtrace));

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term
        = make_counted<func_term_t>(&empty_compile_env, arg);
    counted_t<const func_t> func = func_term->eval_to_func(var_scope_t());
    return wire_func_t(func);
}

std::map<std::string, wire_func_t> global_optargs(protob_t<Query> q) {
    rassert(q.has());

    Term *t = q->mutable_query();
    preprocess_term(t);

    std::map<std::string, wire_func_t> optargs;

    for (int i = 0; i < q->global_optargs_size(); ++i) {
        const Query::AssocPair &ap = q->global_optargs(i);
        auto insert_res
            = optargs.insert(std::make_pair(ap.key(),
                                            construct_optarg_wire_func(ap.val())));
        if (!insert_res.second) {
            rfail_toplevel(
                    base_exc_t::GENERIC,
                    "Duplicate global optarg: %s", ap.key().c_str());
        }
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

bool global_optargs_t::has_optarg(const std::string &key) const {
    return optargs.count(key) > 0;
}
scoped_ptr_t<val_t> global_optargs_t::get_optarg(env_t *env, const std::string &key) {
    auto it = optargs.find(key);
    if (it == optargs.end()) {
        return scoped_ptr_t<val_t>();
    }
    return it->second.compile_wire_func()->call(env);
}
const std::map<std::string, wire_func_t> &global_optargs_t::get_all_optargs() const {
    return optargs;
}

void env_t::set_eval_callback(eval_callback_t *callback) {
    eval_callback_ = callback;
}

void env_t::do_eval_callback() {
    if (eval_callback_ != NULL) {
        eval_callback_->eval_callback();
    }
}

profile_bool_t env_t::profile() const {
    return trace != nullptr ? profile_bool_t::PROFILE : profile_bool_t::DONT_PROFILE;
}

reql_cluster_interface_t *env_t::reql_cluster_interface() {
    r_sanity_check(rdb_ctx_ != NULL);
    return rdb_ctx_->cluster_interface;
}

std::string env_t::get_reql_http_proxy() {
    r_sanity_check(rdb_ctx_ != NULL);
    return rdb_ctx_->reql_http_proxy;
}

extproc_pool_t *env_t::get_extproc_pool() {
    assert_thread();
    r_sanity_check(rdb_ctx_ != NULL);
    r_sanity_check(rdb_ctx_->extproc_pool != NULL);
    return rdb_ctx_->extproc_pool;
}

js_runner_t *env_t::get_js_runner() {
    assert_thread();
    extproc_pool_t *extproc_pool = get_extproc_pool();
    if (!js_runner_.connected()) {
        js_runner_.begin(extproc_pool, interruptor, limits());
    }
    return &js_runner_;
}

scoped_ptr_t<profile::trace_t> maybe_make_profile_trace(profile_bool_t profile) {
    return profile == profile_bool_t::PROFILE
        ? make_scoped<profile::trace_t>()
        : scoped_ptr_t<profile::trace_t>();
}

env_t::env_t(rdb_context_t *ctx,
             return_empty_normal_batches_t _return_empty_normal_batches,
             signal_t *_interruptor,
             std::map<std::string, wire_func_t> optargs,
             profile::trace_t *_trace)
    : global_optargs_(std::move(optargs)),
      limits_(from_optargs(ctx, _interruptor, &global_optargs_)),
      reql_version_(reql_version_t::LATEST),
      cache_(LRU_CACHE_SIZE),
      return_empty_normal_batches(_return_empty_normal_batches),
      interruptor(_interruptor),
      trace(_trace),
      evals_since_yield_(0),
      rdb_ctx_(ctx),
      eval_callback_(NULL) {
    rassert(ctx != NULL);
    rassert(interruptor != NULL);
}


// Used in constructing the env for rdb_update_single_sindex and many unit tests.
env_t::env_t(signal_t *_interruptor,
             return_empty_normal_batches_t _return_empty_normal_batches,
             reql_version_t reql_version)
    : global_optargs_(),
      reql_version_(reql_version),
      cache_(LRU_CACHE_SIZE),
      return_empty_normal_batches(_return_empty_normal_batches),
      interruptor(_interruptor),
      trace(NULL),
      evals_since_yield_(0),
      rdb_ctx_(NULL),
      eval_callback_(NULL) {
    rassert(interruptor != NULL);
}

profile_bool_t profile_bool_optarg(const protob_t<Query> &query) {
    rassert(query.has());
    datum_t profile_arg = static_optarg("profile", query);
    if (profile_arg.has() && profile_arg.get_type() == datum_t::type_t::R_BOOL &&
        profile_arg.as_bool()) {
        return profile_bool_t::PROFILE;
    } else {
        return profile_bool_t::DONT_PROFILE;
    }
}

env_t::~env_t() { }

void env_t::maybe_yield() {
    if (++evals_since_yield_ > EVALS_BEFORE_YIELD) {
        evals_since_yield_ = 0;
        coro_t::yield();
    }
}


} // namespace ql
