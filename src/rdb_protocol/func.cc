// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/func.hpp"

#include "pprint/js_pprint.hpp"
#include "pprint/pprint.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/term_walker.hpp"
#include "stl_utils.hpp"

namespace ql {

func_t::func_t(backtrace_id_t bt)
  : bt_rcheckable_t(bt) { }
func_t::~func_t() { }

scoped_ptr_t<val_t> func_t::call(env_t *env, eval_flags_t eval_flags) const {
    return call(env, std::vector<datum_t>(), eval_flags);
}

scoped_ptr_t<val_t> func_t::call(env_t *env,
                                 datum_t arg,
                                 eval_flags_t eval_flags) const {
    return call(env, make_vector(arg), eval_flags);
}

scoped_ptr_t<val_t> func_t::call(env_t *env,
                              datum_t arg1,
                              datum_t arg2,
                              eval_flags_t eval_flags) const {
    return call(env, make_vector(arg1, arg2), eval_flags);
}

void func_t::assert_deterministic(const char *extra_msg) const {
    rcheck(is_deterministic(),
           base_exc_t::LOGIC,
           strprintf("Could not prove function deterministic.  %s", extra_msg));
}

reql_func_t::reql_func_t(const var_scope_t &_captured_scope,
                         std::vector<sym_t> _arg_names,
                         counted_t<const term_t> _body)
    : func_t(_body->backtrace()),
      captured_scope(_captured_scope),
      arg_names(std::move(_arg_names)),
      body(std::move(_body)) { }

reql_func_t::reql_func_t(scoped_ptr_t<term_storage_t> &&_storage,
                         const var_scope_t &_captured_scope,
                         std::vector<sym_t> _arg_names,
                         counted_t<const term_t> _body)
    : func_t(_body->backtrace()),
      captured_scope(_captured_scope),
      arg_names(std::move(_arg_names)),
      term_storage(std::move(_storage)),
      body(std::move(_body)) { }

reql_func_t::~reql_func_t() { }

scoped_ptr_t<val_t> reql_func_t::call(env_t *env,
                                      const std::vector<datum_t> &args,
                                      eval_flags_t eval_flags) const {
    try {
        // We allow arg_names.size() == 0 to specifically permit users (Ruby users
        // especially) to use zero-arity functions without the drivers to know anything
        // about that.  Some server-created functions might also be constructed this
        // way (thanks to entropy).  TODO: This is bad.
        rcheck(arg_names.size() == args.size() || arg_names.size() == 0,
               base_exc_t::LOGIC,
               strprintf("Expected function with %zu argument%s but found function with %zu argument%s.",
                         args.size(),
                         (args.size() == 1 ? "" : "s"),
                         arg_names.size(),
                         (arg_names.size() == 1 ? "" : "s")));

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

boost::optional<size_t> reql_func_t::arity() const {
    return arg_names.size();
}

bool reql_func_t::is_deterministic() const {
    return body->is_deterministic();
}

js_func_t::js_func_t(const std::string &_js_source,
                     uint64_t timeout_ms,
                     backtrace_id_t backtrace)
    : func_t(backtrace),
      js_source(_js_source),
      js_timeout_ms(timeout_ms) { }

js_func_t::~js_func_t() { }

scoped_ptr_t<val_t> js_func_t::call(
    env_t *env,
    const std::vector<datum_t> &args,
    UNUSED eval_flags_t eval_flags) const {
    try {
        js_runner_t::req_config_t config;
        config.timeout_ms = js_timeout_ms;

        r_sanity_check(!js_source.empty());
        js_result_t result;

        try {
            result = env->get_js_runner()->call(js_source, args, config);
        } catch (const extproc_worker_exc_t &e) {
            rfail(base_exc_t::INTERNAL,
                  "Javascript query `%s` caused a crash in a worker process.",
                  js_source.c_str());
        }

        return scoped_ptr_t<val_t>(
                boost::apply_visitor(
                        js_result_visitor_t(js_source, js_timeout_ms, this), result));
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

boost::optional<size_t> js_func_t::arity() const {
    return boost::none;
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

func_term_t::func_term_t(compile_env_t *env, const raw_term_t &t)
        : term_t(t) {
    r_sanity_check(t.type() == Term::FUNC);
    rcheck(t.num_optargs() == 0,
           base_exc_t::LOGIC,
           "FUNC takes no optional arguments.");
    rcheck(t.num_args() == 2,
           base_exc_t::LOGIC,
           strprintf("FUNC takes exactly two arguments (got %zu)", t.num_args()));

    raw_term_t vars = t.arg(0);
    raw_term_t raw_body = t.arg(1);

    std::vector<sym_t> args;
    if (vars.type() == Term::DATUM) {
        datum_t d = vars.datum();
        rcheck(d.get_type() == datum_t::type_t::R_ARRAY,
               base_exc_t::LOGIC,
               "CLIENT ERROR: FUNC variables must be a literal *array* of numbers.");
        for (size_t i = 0; i < d.arr_size(); ++i) {
            datum_t dnum = d.get(i);
            rcheck(dnum.get_type() == datum_t::type_t::R_NUM,
                   base_exc_t::LOGIC,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            args.push_back(sym_t(dnum.as_num()));
        }
    } else if (vars.type() == Term::MAKE_ARRAY) {
        for (size_t i = 0; i < vars.num_args(); ++i) {
            raw_term_t v = vars.arg(i);
            rcheck(v.type() == Term::DATUM,
                   base_exc_t::LOGIC,
                   "CLIENT ERROR: FUNC variables must be a *literal* array of numbers.");
            datum_t d = v.datum();
            rcheck(d.get_type() == datum_t::type_t::R_NUM,
                   base_exc_t::LOGIC,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            args.push_back(sym_t(d.as_num()));
        }
    } else {
        rfail(base_exc_t::LOGIC,
              "CLIENT ERROR: FUNC variables must be a *literal array of numbers*.");
    }

    var_visibility_t varname_visibility = env->visibility.with_func_arg_name_list(args);
    compile_env_t body_env(std::move(varname_visibility));

    counted_t<const term_t> compiled_body = compile_term(&body_env, raw_body);
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

scoped_ptr_t<val_t> func_term_t::term_eval(scope_env_t *env,
                                           UNUSED eval_flags_t flags) const {
    return new_val(eval_to_func(env->scope));
}

counted_t<const func_t> func_term_t::eval_to_func(const var_scope_t &env_scope) const {
    return make_counted<reql_func_t>(env_scope.filtered_by_captures(external_captures),
                                     arg_names, body);
}

bool func_term_t::is_deterministic() const {
    return body->is_deterministic();
}

/* The predicate here is the datum which defines the predicate and the value is
 * the object which we check to make sure matches the predicate. */
bool filter_match(datum_t predicate, datum_t value,
                  const rcheckable_t *parent) {
    if (predicate.is_ptype(pseudo::literal_string)) {
        return predicate.get_field(pseudo::value_key) == value;
    } else {
        for (size_t i = 0; i < predicate.obj_size(); ++i) {
            auto pair = predicate.get_pair(i);
            r_sanity_check(pair.second.has());
            datum_t elt = value.get_field(pair.first, NOTHROW);
            if (!elt.has()) {
                rfail_target(parent, base_exc_t::NON_EXISTENCE,
                        "No attribute `%s` in object.", pair.first.to_std().c_str());
            } else if (pair.second.get_type() == datum_t::R_OBJECT &&
                       elt.get_type() == datum_t::R_OBJECT) {
                if (!filter_match(pair.second, elt, parent)) { return false; }
            } else if (elt != pair.second) {
                return false;
            }
        }
        return true;
    }
}

bool reql_func_t::filter_helper(env_t *env, datum_t arg) const {
    datum_t d = call(env, make_vector(arg), NO_FLAGS)->as_datum();
    if (d.get_type() == datum_t::R_OBJECT &&
        (body->get_src().type() == Term::MAKE_OBJ ||
         body->get_src().type() == Term::DATUM)) {
        return filter_match(d, arg, this);
    } else {
        return d.as_bool();
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
    ret += pprint::pretty_print(80, pprint::render_as_javascript(body->get_src()));
    return ret;
}

std::string js_func_t::print_source() const {
    std::string ret = strprintf("javascript timeout=%" PRIu64 "ms, source=", js_timeout_ms);
    ret += js_source;
    return ret;
}

bool js_func_t::filter_helper(env_t *env, datum_t arg) const {
    datum_t d = call(env, make_vector(arg), NO_FLAGS)->as_datum();
    return d.as_bool();
}

bool func_t::filter_call(env_t *env, datum_t arg, counted_t<const func_t> default_filter_val) const {
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

counted_t<const func_t> new_constant_func(datum_t obj, backtrace_id_t bt) {
    minidriver_t r(bt);
    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term =
        make_counted<func_term_t>(&empty_compile_env,
                                  r.fun(r.expr(obj)).root_term());
    return func_term->eval_to_func(var_scope_t());
}

counted_t<const func_t> new_get_field_func(datum_t key, backtrace_id_t bt) {
    minidriver_t r(bt);
    auto obj = minidriver_t::dummy_var_t::FUNC_GETFIELD;
    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term =
        make_counted<func_term_t>(&empty_compile_env,
                                  r.fun(obj, r.expr(obj)[key]).root_term());
    return func_term->eval_to_func(var_scope_t());
}

counted_t<const func_t> new_pluck_func(datum_t obj, backtrace_id_t bt) {
    minidriver_t r(bt);
    auto var = minidriver_t::dummy_var_t::FUNC_PLUCK;
    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term =
        make_counted<func_term_t>(&empty_compile_env,
                                  r.fun(var, r.expr(var).pluck(obj)).root_term());
    return func_term->eval_to_func(var_scope_t());
}

counted_t<const func_t> new_eq_comparison_func(datum_t obj, backtrace_id_t bt) {
    minidriver_t r(bt);
    auto var = minidriver_t::dummy_var_t::FUNC_EQCOMPARISON;
    compile_env_t empty_compile_env((var_visibility_t()));
    counted_t<func_term_t> func_term =
        make_counted<func_term_t>(&empty_compile_env,
                                  r.fun(var, r.expr(var) == obj).root_term());
    return func_term->eval_to_func(var_scope_t());
}

counted_t<const func_t> new_page_func(datum_t method, backtrace_id_t bt) {
    if (method.get_type() != datum_t::R_NULL) {
        std::string name = method.as_str().to_std();
        if (name == "link-next") {
            minidriver_t r(bt);
            auto info = minidriver_t::dummy_var_t::FUNC_PAGE;
            compile_env_t empty_compile_env((var_visibility_t()));
            counted_t<func_term_t> func_term =
                make_counted<func_term_t>(&empty_compile_env,
                    r.fun(info,
                          r.expr(info)["header"]["link"]["rel=\"next\""]
                           .default_(r.null())).root_term());
            return func_term->eval_to_func(var_scope_t());
        } else {
            std::string msg = strprintf("`page` method '%s' not recognized, "
                                        "only 'link-next' is available.", name.c_str());
            rcheck_src(bt, false, base_exc_t::LOGIC, msg);
        }
    }
    return counted_t<const func_t>();
}

val_t *js_result_visitor_t::operator()(const std::string &err_val) const {
    rfail_target(parent, base_exc_t::LOGIC, "%s", err_val.c_str());
    unreachable();
}
val_t *js_result_visitor_t::operator()(
    const ql::datum_t &datum) const {
    return new val_t(datum, parent->backtrace());
}
// This JS evaluation resulted in an id for a js function
val_t *js_result_visitor_t::operator()(UNUSED const js_id_t id_val) const {
    counted_t<const func_t> func = make_counted<js_func_t>(code,
                                                           timeout_ms,
                                                           parent->backtrace());
    return new val_t(func, parent->backtrace());
}

} // namespace ql
