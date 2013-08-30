#include "rdb_protocol/func.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/term_walker.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

func_t::func_t(env_t *env,
               const std::string &_js_source,
               uint64_t timeout_ms,
               counted_t<term_t> parent)
    : pb_rcheckable_t(parent->backtrace()),
      source(parent->get_src()),
      js_parent(parent),
      js_env(env),
      js_source(_js_source),
      js_timeout_ms(timeout_ms) {
    env->scopes.dump_scope(&scope);
}

func_t::func_t(env_t *env, protob_t<const Term> _source)
    : pb_rcheckable_t(_source), source(_source),
      js_env(NULL), js_timeout_ms(0) {
    // RSI: This function is absurdly long and complicated.

    protob_t<const Term> t = _source;
    r_sanity_check(t->type() == Term_TermType_FUNC);
    rcheck(t->optargs_size() == 0,
           base_exc_t::GENERIC,
           "FUNC takes no optional arguments.");
    rcheck(t->args_size() == 2,
           base_exc_t::GENERIC,
           strprintf("Func takes exactly two arguments (got %d)", t->args_size()));

    std::vector<sym_t> args;
    const Term *vars = &t->args(0);
    if (vars->type() == Term_TermType_DATUM) {
        const Datum *d = &vars->datum();
        rcheck(d->type() == Datum_DatumType_R_ARRAY,
               base_exc_t::GENERIC,
               "CLIENT ERROR: FUNC variables must be a literal *array* of numbers.");
        for (int i = 0; i < d->r_array_size(); ++i) {
            const Datum *dnum = &d->r_array(i);
            rcheck(dnum->type() == Datum_DatumType_R_NUM,
                   base_exc_t::GENERIC,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            // this is fucking retarded
            args.push_back(sym_t(dnum->r_num()));
        }
    } else if (vars->type() == Term_TermType_MAKE_ARRAY) {
        for (int i = 0; i < vars->args_size(); ++i) {
            const Term *arg = &vars->args(i);
            rcheck(arg->type() == Term_TermType_DATUM,
                   base_exc_t::GENERIC,
                   "CLIENT ERROR: FUNC variables must be a *literal* array of numbers.");
            const Datum *dnum = &arg->datum();
            rcheck(dnum->type() == Datum_DatumType_R_NUM,
                   base_exc_t::GENERIC,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            // this is fucking retarded
            args.push_back(sym_t(dnum->r_num()));
        }
    } else {
        rfail(base_exc_t::GENERIC,
              "CLIENT ERROR: FUNC variables must be a *literal array of numbers*.");
    }

    // RSI: I hope an exception doesn't happen...
    argptrs.init(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        env->scopes.push_var(args[i], &argptrs[i]);
    }
    if (args.size() == 1 && gensym_t::var_allows_implicit(args[0])) {
        env->implicits.push_implicit(&argptrs[0]);
    }
    if (args.size() != 0) {
        guarantee(env->scopes.top_var(args[0], this) == &argptrs[0]);
    }

    protob_t<const Term> body_source = t.make_child(&t->args(1));
    body = compile_term(env, body_source);

    for (size_t i = 0; i < args.size(); ++i) {
        env->scopes.pop_var(args[i]);
    }
    if (args.size() == 1 && gensym_t::var_allows_implicit(args[0])) {
        env->implicits.pop_implicit();
    }

    env->scopes.dump_scope(&scope);
}

counted_t<val_t> func_t::call(const std::vector<counted_t<const datum_t> > &args) const {
    try {
        if (js_parent.has()) {
            r_sanity_check(!body.has() && source.has() && js_env != NULL);

            js_runner_t::req_config_t config;
            config.timeout_ms = js_timeout_ms;

            r_sanity_check(!js_source.empty());
            js_result_t result;

            try {
                result = js_env->get_js_runner()->call(js_source, args, config);
            } catch (const js_worker_exc_t &e) {
                rfail(base_exc_t::GENERIC,
                      "Javascript query `%s` caused a crash in a worker process.",
                      js_source.c_str());
            } catch (const interrupted_exc_t &e) {
                rfail(base_exc_t::GENERIC,
                      "JavaScript query `%s` timed out after %" PRIu64 ".%03" PRIu64 " seconds.",
                      js_source.c_str(), js_timeout_ms / 1000, js_timeout_ms % 1000);
            }

            return boost::apply_visitor(
                js_result_visitor_t(js_env, js_source, js_timeout_ms, js_parent),
                result);
        } else {
            r_sanity_check(body.has() && source.has() && js_env == NULL);
            rcheck(args.size() == argptrs.size() || argptrs.size() == 0,
                   base_exc_t::GENERIC,
                   strprintf("Expected %zd argument(s) but found %zu.",
                             argptrs.size(), args.size()));
            for (size_t i = 0; i < argptrs.size(); ++i) {
                r_sanity_check(args[i].has());
                argptrs[i] = args[i];
            }
            return body->eval();
        }
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

counted_t<val_t> func_t::call() const {
    std::vector<counted_t<const datum_t> > args;
    return call(args);
}

counted_t<val_t> func_t::call(counted_t<const datum_t> arg) const {
    std::vector<counted_t<const datum_t> > args;
    args.push_back(arg);
    return call(args);
}

counted_t<val_t> func_t::call(counted_t<const datum_t> arg1,
                              counted_t<const datum_t> arg2) const {
    std::vector<counted_t<const datum_t> > args;
    args.push_back(arg1);
    args.push_back(arg2);
    return call(args);
}

void func_t::dump_scope(std::map<sym_t, Datum> *out) const {
    for (std::map<sym_t, counted_t<const datum_t> *>::const_iterator
             it = scope.begin(); it != scope.end(); ++it) {
        if (!it->second->has()) continue;
        (*it->second)->write_to_protobuf(&(*out)[it->first]);
    }
}
bool func_t::is_deterministic() const {
    return body.has() ? body->is_deterministic() : false;
}
void func_t::assert_deterministic(const char *extra_msg) const {
    rcheck(is_deterministic(),
           base_exc_t::GENERIC,
           strprintf("Could not prove function deterministic.  %s", extra_msg));
}

std::string func_t::print_src() const {
    r_sanity_check(source.has());
    return source->DebugString();
}

protob_t<const Term> func_t::get_source() const {
    return source;
}

func_term_t::func_term_t(env_t *env, const protob_t<const Term> &term)
    : term_t(env, term), func(make_counted<func_t>(env, term)) { }

counted_t<val_t> func_term_t::eval_impl(UNUSED eval_flags_t flags) {
    return new_val(func);
}

bool func_term_t::is_deterministic_impl() const {
    return func->is_deterministic();
}

bool func_t::filter_call(counted_t<const datum_t> arg, counted_t<func_t> default_filter_val) const {
    // We have to catch every exception type and save it so we can rethrow it later
    // So we don't trigger a coroutine wait in a catch statement
    std::exception_ptr saved_exception;
    base_exc_t::type_t exception_type;

    try {
        counted_t<const datum_t> d = call(arg)->as_datum();
        if (d->get_type() == datum_t::R_OBJECT &&
            (source->args(1).type() == Term::MAKE_OBJ ||
             source->args(1).type() == Term::DATUM)) {
            const std::map<std::string, counted_t<const datum_t> > &obj = d->as_object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                r_sanity_check(it->second.has());
                counted_t<const datum_t> elt = arg->get(it->first, NOTHROW);
                if (!elt.has()) {
                    rfail(base_exc_t::NON_EXISTENCE,
                          "No attribute `%s` in object.", it->first.c_str());
                } else if (*elt != *it->second) {
                    return false;
                }
            }
            return true;
        } else {
            return d->as_bool();
        }
    } catch (const base_exc_t &e) {
        saved_exception = std::current_exception();
        exception_type = e.get_type();
    }

    // We can't call the default_filter_val earlier because it could block,
    //  which would crash since we were in an exception handler
    guarantee(saved_exception != std::exception_ptr());

    if (exception_type == base_exc_t::NON_EXISTENCE) {
        // If a non-existence error is thrown inside a `filter`, we return
        // the default value.  Note that we will enter this code if the
        // function passed to `filter` returns NULL, since the type error
        // above will produce a non-existence error in the case where `d` is
        // NULL.
        try {
            if (default_filter_val) {
                return default_filter_val->call()->as_bool();
            } else {
                return false;
            }
        } catch (const base_exc_t &e) {
            if (e.get_type() != base_exc_t::EMPTY_USER) {
                // If the default value throws a non-EMPTY_USER exception,
                // we re-throw that exception.
                throw;
            }
        }
    }

    // If we caught a non-NON_EXISTENCE exception or we caught a
    // NON_EXISTENCE exception and the default value threw an EMPTY_USER
    // exception, we re-throw the original exception.
    std::rethrow_exception(saved_exception);
}

counted_t<func_t> func_t::new_constant_func(env_t *env, counted_t<const datum_t> obj,
                                            const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    N2(FUNC, N0(MAKE_ARRAY), NDATUM(obj));
    propagate_backtrace(twrap.get(), bt_src.get());
    return make_counted<func_t>(env, twrap);
}

counted_t<func_t> func_t::new_get_field_func(env_t *env, counted_t<const datum_t> key,
                                            const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *arg = twrap.get();
    sym_t obj = env->symgen.gensym();
    arg = pb::set_func(arg, obj);
    N2(GET_FIELD, NVAR(obj), NDATUM(key));
    propagate_backtrace(twrap.get(), bt_src.get());
    return make_counted<func_t>(env, twrap);
}

counted_t<func_t> func_t::new_pluck_func(env_t *env, counted_t<const datum_t> obj,
                                 const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    sym_t var = env->symgen.gensym();
    // RSI: We construct a ReQL array to hold sym_t values???
    N2(FUNC, N1(MAKE_ARRAY, NDATUM(static_cast<double>(var.value))),
       N2(PLUCK, NVAR(var), NDATUM(obj)));
    propagate_backtrace(twrap.get(), bt_src.get());
    return make_counted<func_t>(env, twrap);
}

counted_t<func_t> func_t::new_eq_comparison_func(env_t *env, counted_t<const datum_t> obj,
                    const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    sym_t var = env->symgen.gensym();
    N2(FUNC, N1(MAKE_ARRAY, NDATUM(static_cast<double>(var.value))),
       N2(EQ, NDATUM(obj), NVAR(var)));
    propagate_backtrace(twrap.get(), bt_src.get());
    return make_counted<func_t>(env, twrap);
}

void debug_print(printf_buffer_t *buf, const wire_func_t &func) {
    debug_print(buf, func.debug_str());
}

counted_t<val_t> js_result_visitor_t::operator()(const std::string &err_val) const {
    rfail_target(parent, base_exc_t::GENERIC, "%s", err_val.c_str());
    unreachable();
}
counted_t<val_t> js_result_visitor_t::operator()(
    const counted_t<const ql::datum_t> &datum) const {
    return parent->new_val(datum);
}
// This JS evaluation resulted in an id for a js function
counted_t<val_t> js_result_visitor_t::operator()(UNUSED const id_t id_val) const {
    return parent->new_val(make_counted<func_t>(env, code, timeout_ms, parent));
}

} // namespace ql
