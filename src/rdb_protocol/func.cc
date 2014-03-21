#include "rdb_protocol/func.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/term_walker.hpp"
#include "stl_utils.hpp"

namespace ql {

func_t::func_t(const protob_t<const Backtrace> &bt_source)
  : pb_rcheckable_t(bt_source) { }
func_t::~func_t() { }

counted_t<val_t> func_t::call(env_t *env, eval_flags_t eval_flags) const {
    std::vector<counted_t<const datum_t> > args;
    return call(env, args, eval_flags);
}

counted_t<val_t> func_t::call(env_t *env,
                              counted_t<const datum_t> arg,
                              eval_flags_t eval_flags) const {
    return call(env, make_vector(arg), eval_flags);
}

counted_t<val_t> func_t::call(env_t *env,
                              counted_t<const datum_t> arg1,
                              counted_t<const datum_t> arg2,
                              eval_flags_t eval_flags) const {
    return call(env, make_vector(arg1, arg2), eval_flags);
}

void func_t::assert_deterministic(const char *extra_msg) const {
    rcheck(is_deterministic(),
           base_exc_t::GENERIC,
           strprintf("Could not prove function deterministic.  %s", extra_msg));
}

reql_func_t::reql_func_t(const protob_t<const Backtrace> backtrace,
                         const var_scope_t &_captured_scope,
                         std::vector<sym_t> _arg_names,
                         counted_t<term_t> _body)
    : func_t(backtrace), captured_scope(_captured_scope),
      arg_names(std::move(_arg_names)), body(std::move(_body)) { }

reql_func_t::~reql_func_t() { }

counted_t<val_t> reql_func_t::call(
    env_t *env,
    const std::vector<counted_t<const datum_t> > &args,
    eval_flags_t eval_flags) const {
    try {
        // We allow arg_names.size() == 0 to specifically permit users (Ruby users
        // especially) to use zero-arity functions without the drivers to know anything
        // about that.  Some server-created functions might also be constructed this
        // way (thanks to entropy).  TODO: This is bad.
        rcheck(arg_names.size() == args.size() || arg_names.size() == 0,
               base_exc_t::GENERIC,
               strprintf("Expected %zd argument(s) but found %zu.",
                         arg_names.size(), args.size()));

        var_scope_t new_scope = arg_names.size() == 0
            ? captured_scope
            : captured_scope.with_func_arg_list(arg_names, args);

        scope_env_t scope_env(env, std::move(new_scope));
        return body->eval(&scope_env, eval_flags);
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

bool reql_func_t::is_deterministic() const {
    return body->is_deterministic();
}

js_func_t::js_func_t(const std::string &_js_source,
                     uint64_t timeout_ms,
                     protob_t<const Backtrace> backtrace)
    : func_t(backtrace),
      js_source(_js_source),
      js_timeout_ms(timeout_ms) { }

js_func_t::~js_func_t() { }

counted_t<val_t> js_func_t::call(
    env_t *env,
    const std::vector<counted_t<const datum_t> > &args,
    UNUSED eval_flags_t eval_flags) const {
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
                  "JavaScript query `%s` timed out after "
                  "%" PRIu64 ".%03" PRIu64 " seconds.",
                  js_source.c_str(), js_timeout_ms / 1000, js_timeout_ms % 1000);
        }

        return boost::apply_visitor(
            js_result_visitor_t(js_source, js_timeout_ms, this), result);
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

bool js_func_t::is_deterministic() const {
    return false;
}

void reql_func_t::visit(func_visitor_t *visitor) const {
    visitor->on_reql_func(this);
}

void js_func_t::visit(func_visitor_t *visitor) const {
    visitor->on_js_func(this);
}

func_term_t::func_term_t(compile_env_t *env, const protob_t<const Term> &t)
    : term_t(t) {
    r_sanity_check(t.has());
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

    var_visibility_t varname_visibility = env->visibility.with_func_arg_name_list(args);

    compile_env_t body_env(std::move(varname_visibility));

    protob_t<const Term> body_source = t.make_child(&t->args(1));
    counted_t<term_t> compiled_body = compile_term(&body_env, body_source);
    r_sanity_check(compiled_body.has());

    var_captures_t captures;
    compiled_body->accumulate_captures(&captures);
    for (auto it = args.begin(); it != args.end(); ++it) {
        captures.vars_captured.erase(*it);
    }
    if (function_emits_implicit_variable(args)) {
        captures.implicit_is_captured = false;
    }

    arg_names = std::move(args);
    body = std::move(compiled_body);
    external_captures = std::move(captures);
}

void func_term_t::accumulate_captures(var_captures_t *captures) const {
    captures->vars_captured.insert(external_captures.vars_captured.begin(),
                                   external_captures.vars_captured.end());
    captures->implicit_is_captured |= external_captures.implicit_is_captured;
}

counted_t<val_t> func_term_t::term_eval(scope_env_t *env, UNUSED eval_flags_t flags) {
    return new_val(eval_to_func(env->scope));
}

counted_t<func_t> func_term_t::eval_to_func(const var_scope_t &env_scope) {
    return make_counted<reql_func_t>(get_backtrace(get_src()),
                                     env_scope.filtered_by_captures(external_captures),
                                     arg_names, body);
}

bool func_term_t::is_deterministic() const {
    return body->is_deterministic();
}

/* The predicate here is the datum which defines the predicate and the value is
 * the object which we check to make sure matches the predicate. */
bool filter_match(counted_t<const datum_t> predicate, counted_t<const datum_t> value,
                  const rcheckable_t *parent) {
    if (predicate->is_ptype(pseudo::literal_string)) {
        return *predicate->get(pseudo::value_key) == *value;
    } else {
        const std::map<std::string, counted_t<const datum_t> > &obj
            = predicate->as_object();
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

bool reql_func_t::filter_helper(env_t *env, counted_t<const datum_t> arg) const {
    counted_t<const datum_t> d = call(env, make_vector(arg), NO_FLAGS)->as_datum();
    if (d->get_type() == datum_t::R_OBJECT &&
        (body->get_src()->type() == Term::MAKE_OBJ ||
         body->get_src()->type() == Term::DATUM)) {
        return filter_match(d, arg, this);
    } else {
        return d->as_bool();
    }
}

std::string reql_func_t::print_source() const {
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
    counted_t<const datum_t> d = call(env, make_vector(arg), NO_FLAGS)->as_datum();
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

counted_t<func_t> new_constant_func(counted_t<const datum_t> obj,
                                    const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = r::fun(r::expr(obj)).release_counted();
    propagate_backtrace(twrap.get(), bt_src.get());

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_compile_env,
                                                                 twrap);
    return func_term->eval_to_func(var_scope_t());
}

counted_t<func_t> new_get_field_func(counted_t<const datum_t> key,
                                     const protob_t<const Backtrace> &bt_src) {
    pb::dummy_var_t obj = pb::dummy_var_t::FUNC_GETFIELD;
    protob_t<Term> twrap = r::fun(obj, r::var(obj)[key]).release_counted();

    propagate_backtrace(twrap.get(), bt_src.get());

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_compile_env,
                                                                 twrap);
    return func_term->eval_to_func(var_scope_t());
}

counted_t<func_t> new_pluck_func(counted_t<const datum_t> obj,
                                 const protob_t<const Backtrace> &bt_src) {
    pb::dummy_var_t var = pb::dummy_var_t::FUNC_PLUCK;
    protob_t<Term> twrap = r::fun(var, r::var(var).pluck(obj)).release_counted();
    propagate_backtrace(twrap.get(), bt_src.get());

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_compile_env,
                                                                 twrap);
    return func_term->eval_to_func(var_scope_t());
}

counted_t<func_t> new_eq_comparison_func(counted_t<const datum_t> obj,
                                         const protob_t<const Backtrace> &bt_src) {
    pb::dummy_var_t var = pb::dummy_var_t::FUNC_EQCOMPARISON;
    protob_t<Term> twrap = r::fun(var, r::var(var) == obj).release_counted();
    propagate_backtrace(twrap.get(), bt_src.get());

    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term = make_counted<func_term_t>(&empty_compile_env,
                                                                 twrap);
    return func_term->eval_to_func(var_scope_t());
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
    counted_t<func_t> func = make_counted<js_func_t>(code, timeout_ms, parent->backtrace());
    return make_counted<val_t>(func, parent->backtrace());
}

} // namespace ql
