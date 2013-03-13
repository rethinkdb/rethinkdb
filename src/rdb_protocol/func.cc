#include "rdb_protocol/func.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

func_t::func_t(env_t *env, js::id_t id, term_t *parent)
    : pb_rcheckable_t(parent), body(0), source(0),
      js_parent(parent), js_env(env), js_id(id) { }

func_t::func_t(env_t *env, const Term *_source)
    : pb_rcheckable_t(_source), body(0), source(_source),
      js_parent(0), js_env(0), js_id(js::INVALID_ID) {
    const Term *t = _source;
    r_sanity_check(t->type() == Term_TermType_FUNC);
    rcheck(t->optargs_size() == 0, "FUNC takes no optional arguments.");
    rcheck(t->args_size() == 2, strprintf("Func takes exactly two arguments (got %d)",
                                          t->args_size()));

    std::vector<int> args;
    const Term *vars = &t->args(0);
    if (vars->type() == Term_TermType_DATUM) {
        const Datum *d = &vars->datum();
        rcheck(d->type() == Datum_DatumType_R_ARRAY,
               "CLIENT ERROR: FUNC variables must be a literal *array* of numbers.");
        for (int i = 0; i < d->r_array_size(); ++i) {
            const Datum *dnum = &d->r_array(i);
            rcheck(dnum->type() == Datum_DatumType_R_NUM,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            args.push_back(dnum->r_num());
        }
    } else if (vars->type() == Term_TermType_MAKE_ARRAY) {
        for (int i = 0; i < vars->args_size(); ++i) {
            const Term *arg = &vars->args(i);
            rcheck(arg->type() == Term_TermType_DATUM,
                   "CLIENT ERROR: FUNC variables must be a *literal* array of numbers.");
            const Datum *dnum = &arg->datum();
            rcheck(dnum->type() == Datum_DatumType_R_NUM,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            args.push_back(dnum->r_num());
        }
    } else {
        rfail("CLIENT ERROR: FUNC variables must be a *literal array of numbers*.");
    }

    guarantee(argptrs.size() == 0);
    argptrs.reserve(args.size()); // NECESSARY FOR POINTERS TO REMAIN VALID
    for (size_t i = 0; i < args.size(); ++i) {
        argptrs.push_back(0);
        env->push_var(args[i], &argptrs[i]);
    }
    if (args.size() == 1 && env_t::var_allows_implicit(args[0])) {
        env->push_implicit(&argptrs[0]);
    }

    if (args.size()) guarantee(env->top_var(args[0], this) == &argptrs[0]);

    const Term *body_source = &t->args(1);
    body = env->new_term(body_source);

    for (size_t i = 0; i < args.size(); ++i) {
        env->pop_var(args[i]);
    }
    if (args.size() == 1 && env_t::var_allows_implicit(args[0])) {
        env->pop_implicit();
    }

    env->dump_scope(&scope);
}

val_t *func_t::call(const std::vector<const datum_t *> &args) {
    try {
        if (js_parent != 0) {
            r_sanity_check(!body && !source && js_env);
            // Convert datum args to cJSON args for the JS runner
            std::vector<boost::shared_ptr<scoped_cJSON_t> > json_args;
            for (const datum_t *arg : args) {
                json_args.push_back(arg->as_json());
            }

            boost::shared_ptr<js::runner_t> js = js_env->get_js_runner();
            js::js_result_t result = js->call(js_id, json_args);

            return boost::apply_visitor(js_result_visitor_t(js_env, js_parent), result);
        } else {
            r_sanity_check(body && source && !js_env);
            rcheck(args.size() == argptrs.size() || argptrs.size() == 0,
                   strprintf("Expected %zu argument(s) but found %zu.",
                             argptrs.size(), args.size()));
            for (size_t i = 0; i < argptrs.size(); ++i) {
                r_sanity_check(args[i]);
                argptrs[i] = args[i];
            }
            return body->eval(false);
            //                ^^^^^ don't use cached value
        }
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

val_t *func_t::call() {
    std::vector<const datum_t *> args;
    return call(args);
}

val_t *func_t::call(const datum_t *arg) {
    std::vector<const datum_t *> args;
    args.push_back(arg);
    return call(args);
}

val_t *func_t::call(const datum_t *arg1, const datum_t *arg2) {
    std::vector<const datum_t *> args;
    args.push_back(arg1);
    args.push_back(arg2);
    return call(args);
}

void func_t::dump_scope(std::map<int64_t, Datum> *out) const {
    r_sanity_check(body && source && !js_env && !js_parent);
    for (std::map<int64_t, const datum_t **>::const_iterator
             it = scope.begin(); it != scope.end(); ++it) {
        if (!*it->second) continue;
        (*it->second)->write_to_protobuf(&(*out)[it->first]);
    }
}
bool func_t::is_deterministic() const {
    return body ? body->is_deterministic() : false;
}

wire_func_t::wire_func_t() { }
wire_func_t::wire_func_t(env_t *env, func_t *func) : source(*func->source) {
    if (env) {
        cached_funcs[env] = func;
    }

    func->dump_scope(&scope);
}
wire_func_t::wire_func_t(const Term &_source, std::map<int64_t, Datum> *_scope)
    : source(_source) {
    if (_scope) scope = *_scope;
}

func_t *wire_func_t::compile(env_t *env) {
    if (cached_funcs.count(env) == 0) {
        env->push_scope(&scope);
        cached_funcs[env] = env->new_func(&source);
        env->pop_scope();
    }
    return cached_funcs[env];
}

func_term_t::func_term_t(env_t *env, const Term *term)
    : term_t(env, term), func(env->new_func(term)) { }
val_t *func_term_t::eval_impl() {
    return new_val(func);
}
bool func_term_t::is_deterministic_impl() const {
    return func->is_deterministic();
}

bool func_t::filter_call(env_t *env, const datum_t *arg) {
    const datum_t *d = call(arg)->as_datum();
    if (d->get_type() == datum_t::R_OBJECT) {
        func_t *f2 = new_filter_func(env, d, this);
        d = f2->call(arg)->as_datum();
    }
    if (d->get_type() == datum_t::R_BOOL) return d->as_bool();
    rfail("FILTER must be passed either an OBJECT or a predicate (got %s).",
          d->get_type_name());
}

func_t *func_t::new_filter_func(env_t *env, const datum_t *obj,
                                const pb_rcheckable_t *bt_src) {
    env_wrapper_t<Term> *twrap = env->add_ptr(new env_wrapper_t<Term>());
    int x = env->gensym();
    Term *t = pb::set_func(&twrap->t, x);
    pb::set(t, Term_TermType_ALL, 0, 0);
    for (std::map<const std::string, const datum_t *>::const_iterator
             it = obj->as_object().begin(); it != obj->as_object().end(); ++it) {
        std::string key = it->first;
        const datum_t *val = it->second;

        Term *arg = t->add_args();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N2(EQ,
           N2(GETATTR, pb::set_var(arg, x), pb::set_str(arg, key)),
           val->write_to_protobuf(pb::set_datum(arg)));
#pragma GCC diagnostic pop
    }
    bt_src->propagate(&twrap->t);
    return env->add_ptr(new func_t(env, &twrap->t));
}


func_t *func_t::new_identity_func(env_t *env, const datum_t *obj,
                                  const pb_rcheckable_t *bt_src) {
    env_wrapper_t<Term> *twrap = env->add_ptr(new env_wrapper_t<Term>());
    Term *arg = &twrap->t;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N2(FUNC, N0(MAKE_ARRAY), NDATUM(obj));
#pragma GCC diagnostic pop
    bt_src->propagate(&twrap->t);
    return env->add_ptr(new func_t(env, &twrap->t));
}

} // namespace ql
