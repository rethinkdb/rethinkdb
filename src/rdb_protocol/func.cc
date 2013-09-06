#include "rdb_protocol/func.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/term_walker.hpp"
#include "stl_utils.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

func_t::func_t(const protob_t<const Term> &term) : pb_rcheckable_t(term) { }
func_t::func_t(const protob_t<const Backtrace> &bt_source) : pb_rcheckable_t(bt_source) { }
func_t::~func_t() { }

counted_t<val_t> func_t::call(env_t *env) const {
    std::vector<counted_t<const datum_t> > args;
    return call(env, args);
}

counted_t<val_t> func_t::call(env_t *env, counted_t<const datum_t> arg) const {
    return call(env, make_vector(arg));
}

counted_t<val_t> func_t::call(env_t *env,
                              counted_t<const datum_t> arg1,
                              counted_t<const datum_t> arg2) const {
    return call(env, make_vector(arg1, arg2));
}

void func_t::assert_deterministic(const char *extra_msg) const {
    rcheck(is_deterministic(),
           base_exc_t::GENERIC,
           strprintf("Could not prove function deterministic.  %s", extra_msg));
}

// RSI: Ugh, the source parameter.
good_func_t::good_func_t(const protob_t<const Term> source,
                         const var_scope_t &_captured_scope,
                         std::vector<sym_t> _arg_names,
                         counted_t<term_t> _body)
    : func_t(source), captured_scope(_captured_scope),
      arg_names(std::move(_arg_names)), body(std::move(_body)) { }

good_func_t::good_func_t(const protob_t<const Backtrace> backtrace,
                         const var_scope_t &_captured_scope,
                         std::vector<sym_t> _arg_names,
                         counted_t<term_t> _body)
    : func_t(backtrace), captured_scope(_captured_scope),
      arg_names(std::move(_arg_names)), body(std::move(_body)) { }

good_func_t::~good_func_t() { }

counted_t<val_t> good_func_t::call(env_t *env, const std::vector<counted_t<const datum_t> > &args) const {
    try {
        // RSI: This allow arg_names.size() to be 0.  Why do we allow this?  (Is the behavior subtly different in the new implementation?  Because in the old implementation, maybe some stuff saw random old argptrs values...)
        rcheck(arg_names.size() == args.size() || arg_names.size() == 0,
               base_exc_t::GENERIC,
               strprintf("Expected %zd argument(s) but found %zu.",
                         arg_names.size(), args.size()));

        std::vector<std::pair<sym_t, counted_t<const datum_t> > > new_vars;
        for (size_t i = 0; i < arg_names.size(); ++i) {
            new_vars.push_back(std::make_pair(arg_names[i], args[i]));
        }
        var_scope_t new_scope = captured_scope.with_func_arg_list(new_vars);

        scope_env_t scope_env(env, new_scope);
        return body->eval(&scope_env);
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

bool good_func_t::is_deterministic() const {
    return body->is_deterministic();
}

js_func_t::js_func_t(const std::string &_js_source,
                     uint64_t timeout_ms,
                     const pb_rcheckable_t *parent)
    : func_t(parent->backtrace()),
      js_source(_js_source),
      js_timeout_ms(timeout_ms) { }

js_func_t::js_func_t(const std::string &_js_source,
                     uint64_t timeout_ms,
                     protob_t<const Backtrace> backtrace)
    : func_t(std::move(backtrace)),
      js_source(_js_source),
      js_timeout_ms(timeout_ms) { }


js_func_t::~js_func_t() { }

counted_t<val_t> js_func_t::call(env_t *env, const std::vector<counted_t<const datum_t> > &args) const {
    try {
        js_runner_t::req_config_t config;
        config.timeout_ms = js_timeout_ms;

        r_sanity_check(!js_source.empty());
        js_result_t result;

        try {
            result = env->get_js_runner()->call(js_source, args, config);
        } catch (const js_worker_exc_t &e) {
            rfail(base_exc_t::GENERIC,
                  "Javascript query `%s` caused a crash in a worker process.",
                  js_source.c_str());
        } catch (const interrupted_exc_t &e) {
            rfail(base_exc_t::GENERIC,
                  "JavaScript query `%s` timed out after %" PRIu64 ".%03" PRIu64 " seconds.",
                  js_source.c_str(), js_timeout_ms / 1000, js_timeout_ms % 1000);
        }

        return boost::apply_visitor(js_result_visitor_t(js_source,
                                                        js_timeout_ms, this),
                                    result);
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

bool js_func_t::is_deterministic() const {
    return false;
}

void good_func_t::visit(func_visitor_t *visitor) const {
    visitor->on_good_func(this);
}

void js_func_t::visit(func_visitor_t *visitor) const {
    visitor->on_js_func(this);
}

func_term_t::func_term_t(visibility_env_t *env, const protob_t<const Term> &t)
    : term_t(t) {
    r_sanity_check(t.has());
    r_sanity_check(t->type() == Term_TermType_FUNC);
    rcheck(t->optargs_size() == 0,
           base_exc_t::GENERIC,
           "FUNC takes no optional arguments.");
    rcheck(t->args_size() == 2,
           base_exc_t::GENERIC,
           strprintf("Func takes exactly two arguments (got %d)", t->args_size()));

    // RSI: Why are there two ways of specifying the args array???
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

    var_visibility_t varname_visibility = env->visibility.with_func_arg_name_list(args);

    visibility_env_t body_env(env->env, varname_visibility);

    protob_t<const Term> body_source = t.make_child(&t->args(1));
    counted_t<term_t> compiled_body = compile_term(&body_env, body_source);
    r_sanity_check(compiled_body.has());

    arg_names = std::move(args);
    body = std::move(compiled_body);
}

counted_t<val_t> func_term_t::eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
    return new_val(eval_to_func(env));
}

counted_t<func_t> func_term_t::eval_to_func(scope_env_t *env) {
    return make_counted<good_func_t>(get_src(), env->scope, arg_names, body);
}

bool func_term_t::is_deterministic_impl() const {
    return body->is_deterministic();
}

/* The predicate here is the datum which defines the predicate and the value is
 * the object which we check to make sure matches the predicate. */
bool filter_match(counted_t<const datum_t> predicate, counted_t<const datum_t> value,
                  const rcheckable_t *parent) {
    if (predicate->is_ptype(pseudo::literal_string)) {
        return *predicate->get(pseudo::value_key) == *value;
    } else {
        const std::map<std::string, counted_t<const datum_t> > &obj = predicate->as_object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            r_sanity_check(it->second.has());
            counted_t<const datum_t> elt = value->get(it->first, NOTHROW);
            if (!elt.has()) {
                rfail_target(parent, base_exc_t::NON_EXISTENCE,
                        "No attribute `%s` in object.", it->first.c_str());
            } else if (it->second->get_type() == datum_t::R_OBJECT &&
                       elt->get_type() == datum_t::R_OBJECT) {
                if (!filter_match(it->second, elt, parent)) { return false; }
            } else if (*elt != *it->second) {
                return false;
            }
        }
        return true;
    }
}

bool good_func_t::filter_helper(env_t *env, counted_t<const datum_t> arg) const {
    counted_t<const datum_t> d = call(env, make_vector(arg))->as_datum();
    // RSI: This body->get_src() stuff is fragile shit.
    if (d->get_type() == datum_t::R_OBJECT &&
        (body->get_src()->type() == Term::MAKE_OBJ ||
         body->get_src()->type() == Term::DATUM)) {
        return filter_match(d, arg, this);
    } else {
        return d->as_bool();
    }
}

std::string good_func_t::print_source() const {
    std::string ret = "function (captures = " + captured_scope.print() + ") (args = [";
    for (size_t i = 0; i < arg_names.size(); ++i) {
        if (i != 0) {
            ret += ", ";
        }
        ret += strprintf("%" PRIi64, arg_names[i].value);
    }
    ret += "]) ";
    ret += body->get_src()->DebugString();
    return ret;
}

std::string js_func_t::print_source() const {
    std::string ret = strprintf("javascript timeout=%" PRIu64 "ms, source=", js_timeout_ms);
    ret += js_source;
    return ret;
}

bool js_func_t::filter_helper(env_t *env, counted_t<const datum_t> arg) const {
    counted_t<const datum_t> d = call(env, make_vector(arg))->as_datum();
    return d->as_bool();
}

bool func_t::filter_call(env_t *env, counted_t<const datum_t> arg, counted_t<func_t> default_filter_val) const {
    // We have to catch every exception type and save it so we can rethrow it later
    // So we don't trigger a coroutine wait in a catch statement
    std::exception_ptr saved_exception;
    base_exc_t::type_t exception_type;

    try {
        return filter_helper(env, arg);
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
                return default_filter_val->call(env)->as_bool();
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

counted_t<func_t> new_constant_func(env_t *env, counted_t<const datum_t> obj,
                                    const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    N2(FUNC, N0(MAKE_ARRAY), NDATUM(obj));
    propagate_backtrace(twrap.get(), bt_src.get());

    visibility_env_t empty_visibility_env(env, var_visibility_t());
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_visibility_env,
                                                                 twrap);
    scope_env_t empty_scope_env(env, var_scope_t());
    return func_term->eval_to_func(&empty_scope_env);
}

counted_t<func_t> new_get_field_func(env_t *env, counted_t<const datum_t> key,
                                     const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *arg = twrap.get();
    sym_t obj = env->symgen.gensym();
    arg = pb::set_func(arg, obj);
    N2(GET_FIELD, NVAR(obj), NDATUM(key));
    propagate_backtrace(twrap.get(), bt_src.get());

    visibility_env_t empty_visibility_env(env, var_visibility_t());
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_visibility_env,
                                                                 twrap);
    scope_env_t empty_scope_env(env, var_scope_t());
    return func_term->eval_to_func(&empty_scope_env);
}

counted_t<func_t> new_pluck_func(env_t *env, counted_t<const datum_t> obj,
                                 const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    sym_t var = env->symgen.gensym();
    N2(FUNC, N1(MAKE_ARRAY, NDATUM(static_cast<double>(var.value))),
       N2(PLUCK, NVAR(var), NDATUM(obj)));
    propagate_backtrace(twrap.get(), bt_src.get());

    visibility_env_t empty_visibility_env(env, var_visibility_t());
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_visibility_env,
                                                                 twrap);
    scope_env_t empty_scope_env(env, var_scope_t());
    return func_term->eval_to_func(&empty_scope_env);
}

counted_t<func_t> new_eq_comparison_func(env_t *env, counted_t<const datum_t> obj,
                                         const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    sym_t var = env->symgen.gensym();
    N2(FUNC, N1(MAKE_ARRAY, NDATUM(static_cast<double>(var.value))),
       N2(EQ, NDATUM(obj), NVAR(var)));
    propagate_backtrace(twrap.get(), bt_src.get());

    visibility_env_t empty_visibility_env(env, var_visibility_t());
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_visibility_env,
                                                                 twrap);
    scope_env_t empty_scope_env(env, var_scope_t());
    return func_term->eval_to_func(&empty_scope_env);
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
    return make_counted<val_t>(datum, parent->backtrace());
}
// This JS evaluation resulted in an id for a js function
counted_t<val_t> js_result_visitor_t::operator()(UNUSED const id_t id_val) const {
    counted_t<func_t> func = make_counted<js_func_t>(code, timeout_ms, parent);
    return make_counted<val_t>(func, parent->backtrace());
}

} // namespace ql
