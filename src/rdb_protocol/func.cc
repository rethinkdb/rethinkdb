#include "rdb_protocol/func.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/term_walker.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

func_t::func_t(env_t *env, js::id_t id, counted_t<term_t> parent)
    : pb_rcheckable_t(parent->backtrace()), source(parent->get_src()),
      js_parent(parent), js_env(env), js_id(id) {
    env->dump_scope(&scope);
}

func_t::func_t(env_t *env, protob_t<const Term> _source)
    : pb_rcheckable_t(_source), source(_source),
      js_env(NULL), js_id(js::INVALID_ID) {
    protob_t<const Term> t = _source;
    r_sanity_check(t->type() == Term_TermType_FUNC);
    rcheck(t->optargs_size() == 0,
           base_exc_t::GENERIC,
           "FUNC takes no optional arguments.");
    rcheck(t->args_size() == 2,
           base_exc_t::GENERIC,
           strprintf("Func takes exactly two arguments (got %d)", t->args_size()));

    std::vector<int> args;
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
            args.push_back(dnum->r_num());
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
            args.push_back(dnum->r_num());
        }
    } else {
        rfail(base_exc_t::GENERIC,
              "CLIENT ERROR: FUNC variables must be a *literal array of numbers*.");
    }

    argptrs.init(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        env->push_var(args[i], &argptrs[i]);
    }
    if (args.size() == 1 && env_t::var_allows_implicit(args[0])) {
        env->push_implicit(&argptrs[0]);
    }
    if (args.size() != 0) {
        guarantee(env->top_var(args[0], this) == &argptrs[0]);
    }

    protob_t<const Term> body_source = t.make_child(&t->args(1));
    body = compile_term(env, body_source);

    for (size_t i = 0; i < args.size(); ++i) {
        env->pop_var(args[i]);
    }
    if (args.size() == 1 && env_t::var_allows_implicit(args[0])) {
        env->pop_implicit();
    }

    env->dump_scope(&scope);
}

counted_t<val_t> func_t::call(const std::vector<counted_t<const datum_t> > &args) {
    try {
        if (js_parent.has()) {
            r_sanity_check(!body.has() && source.has() && js_env != NULL);
            // Convert datum args to cJSON args for the JS runner
            std::vector<boost::shared_ptr<scoped_cJSON_t> > json_args;
            for (auto arg_iter = args.begin(); arg_iter != args.end(); ++arg_iter) {
                json_args.push_back((*arg_iter)->as_json());
            }

            boost::shared_ptr<js::runner_t> js = js_env->get_js_runner();
            js::js_result_t result = js->call(js_id, json_args);

            return boost::apply_visitor(js_result_visitor_t(js_env, js_parent), result);
        } else {
            r_sanity_check(body.has() && source.has() && js_env == NULL);
            rcheck(args.size() == static_cast<size_t>(argptrs.size())
                   || argptrs.size() == 0,
                   base_exc_t::GENERIC,
                   strprintf("Expected %zd argument(s) but found %zu.",
                             argptrs.size(), args.size()));
            for (ssize_t i = 0; i < argptrs.size(); ++i) {
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

counted_t<val_t> func_t::call() {
    std::vector<counted_t<const datum_t> > args;
    return call(args);
}

counted_t<val_t> func_t::call(counted_t<const datum_t> arg) {
    std::vector<counted_t<const datum_t> > args;
    args.push_back(arg);
    return call(args);
}

counted_t<val_t> func_t::call(counted_t<const datum_t> arg1,
                              counted_t<const datum_t> arg2) {
    std::vector<counted_t<const datum_t> > args;
    args.push_back(arg1);
    args.push_back(arg2);
    return call(args);
}

void func_t::dump_scope(std::map<int64_t, Datum> *out) const {
    for (std::map<int64_t, counted_t<const datum_t> *>::const_iterator
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

void func_t::set_default_filter_val(counted_t<func_t> func) {
    default_filter_val = func;
}

// This JS evaluation resulted in an error
counted_t<val_t> js_result_visitor_t::operator()(const std::string err_val) const {
    rfail_target(parent, base_exc_t::GENERIC, "%s", err_val.c_str());
    unreachable();
}

counted_t<val_t>
js_result_visitor_t::operator()(const boost::shared_ptr<scoped_cJSON_t> json_val) const {
    return parent->new_val(make_counted<const datum_t>(json_val, env));
}

// This JS evaluation resulted in an id for a js function
counted_t<val_t> js_result_visitor_t::operator()(const id_t id_val) const {
    return parent->new_val(make_counted<func_t>(env, id_val, parent));
}

wire_func_t::wire_func_t() : source(make_counted_term()) { }
wire_func_t::wire_func_t(env_t *env, counted_t<func_t> func)
    : source(make_counted_term_copy(*func->source)) {
    if (func->default_filter_val.has()) {
        default_filter_val = *func->default_filter_val->source.get();
    }
    if (env) {
        cached_funcs[env->uuid] = func;
    }

    func->dump_scope(&scope);
}
wire_func_t::wire_func_t(const Term &_source, const std::map<int64_t, Datum> &_scope)
    : source(make_counted_term_copy(_source)), scope(_scope) { }

counted_t<func_t> wire_func_t::compile(env_t *env) {
    if (cached_funcs.count(env->uuid) == 0) {
        env->push_scope(&scope);
        cached_funcs[env->uuid] = compile_term(env, source)->eval()->as_func();
        if (default_filter_val) {
            cached_funcs[env->uuid]->set_default_filter_val(
                make_counted<func_t>(env, make_counted_term_copy(*default_filter_val)));
        }
        env->pop_scope();
    }
    return cached_funcs[env->uuid];
}

void wire_func_t::rdb_serialize(write_message_t &msg) const {  // NOLINT(runtime/references)
    guarantee(source.has());
    msg << *source;
    msg << default_filter_val;
    msg << scope;
}

archive_result_t wire_func_t::rdb_deserialize(read_stream_t *stream) {
    guarantee(source.has());
    archive_result_t res = deserialize(stream, source.get());
    if (res != ARCHIVE_SUCCESS) { return res; }
    res = deserialize(stream, &default_filter_val);
    if (res != ARCHIVE_SUCCESS) { return res; }
    return deserialize(stream, &scope);
}

func_term_t::func_term_t(env_t *env, protob_t<const Term> term)
    : term_t(env, term), func(make_counted<func_t>(env, term)) { }

counted_t<val_t> func_term_t::eval_impl() {
    return new_val(func);
}

bool func_term_t::is_deterministic_impl() const {
    return func->is_deterministic();
}

bool func_t::filter_call(counted_t<const datum_t> arg) {
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
        if (e.get_type() == base_exc_t::NON_EXISTENCE) {
            // If a non-existence error is thrown inside a `filter`, we return
            // the default value.  Note that we will enter this branch if the
            // function passed to `filter` returns NULL, since the type error
            // above will produce a non-existence error in the case where `d` is
            // NULL.
            try {
                if (default_filter_val) {
                    return default_filter_val->call()->as_bool();
                } else {
                    return false;
                }
            } catch (const base_exc_t &e2) {
                if (e2.get_type() != base_exc_t::EMPTY_USER) {
                    // If the default value throws a non-EMPTY_USER exception,
                    // we re-throw that exception.
                    throw;
                }
            }
        }
        // If we caught a non-NON_EXISTENCE exception or we caught a
        // NON_EXISTENCE exception and the default value threw an EMPTY_USER
        // exception, we re-throw the original exception.
        throw;
    }
}

counted_t<func_t> func_t::new_identity_func(env_t *env, counted_t<const datum_t> obj,
                                            const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    N2(FUNC, N0(MAKE_ARRAY), NDATUM(obj));
    propagate_backtrace(twrap.get(), bt_src.get());
    return make_counted<func_t>(env, twrap);
}

counted_t<func_t> func_t::new_eq_comparison_func(env_t *env, counted_t<const datum_t> obj,
                    const protob_t<const Backtrace> &bt_src) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    int var = env->gensym();
    N2(FUNC, N1(MAKE_ARRAY, NDATUM(static_cast<double>(var))),
       N2(EQ, NDATUM(obj), NVAR(var)));
    propagate_backtrace(twrap.get(), bt_src.get());
    return make_counted<func_t>(env, twrap);
}

void debug_print(printf_buffer_t *buf, const wire_func_t &func) {
    debug_print(buf, func.debug_str());
}

} // namespace ql
