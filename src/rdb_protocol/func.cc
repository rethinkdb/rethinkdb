#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

func_t::func_t(env_t *env, const Term2 *_source) : body(0), source(_source) {
    const Term2 *t = _source;
    r_sanity_check(t->type() == Term2_TermType_FUNC);
    rcheck(t->optargs_size() == 0, "FUNC takes no optional arguments.");
    rcheck(t->args_size() == 2, strprintf("Func takes exactly two arguments (got %d)",
                                          t->args_size()));

    std::vector<int> args;
    const Term2 *vars = &t->args(0);
    if (vars->type() == Term2_TermType_DATUM) {
        const Datum *d = &vars->datum();
        rcheck(d->type() == Datum_DatumType_R_ARRAY,
               "CLIENT ERROR: FUNC variables must be a literal *array* of numbers.");
        for (int i = 0; i < d->r_array_size(); ++i) {
            const Datum *dnum = &d->r_array(i);
            rcheck(dnum->type() == Datum_DatumType_R_NUM,
                   "CLIENT ERROR: FUNC variables must be a literal array of *numbers*.");
            args.push_back(dnum->r_num());
        }
    } else if (vars->type() == Term2_TermType_MAKE_ARRAY) {
        for (int i = 0; i < vars->args_size(); ++i) {
            const Term2 *arg = &vars->args(i);
            rcheck(arg->type() == Term2_TermType_DATUM,
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
        //debugf("pushing %d -> %p\n", args[i], &argptrs[i]);
        env->push_var(args[i], &argptrs[i]);
        if (args.size() == 1) env->push_implicit(&argptrs[i]);
    }
    if (args.size()) guarantee(env->top_var(args[0]) == &argptrs[0]);

    const Term2 *body_source = &t->args(1);
    body = env->new_term(body_source);

    for (size_t i = 0; i < args.size(); ++i) {
        //debugf("popping %d\n", args[i]);
        env->pop_var(args[i]);
        if (args.size() == 1) env->pop_implicit();
    }

    env->dump_scope(&scope);
}

func_t *func_t::new_shortcut_func(env_t *env, const datum_t *obj) {
    env_wrapper_t<Term2> *twrap = env->add_ptr(new env_wrapper_t<Term2>());
    int x = env->gensym();
    Term2 *t = pb::set_func(&twrap->t, x);
    pb::set(t, Term2_TermType_ALL, 0, 0);
    for (std::map<const std::string, const datum_t *>::const_iterator
             it = obj->as_object().begin(); it != obj->as_object().end(); ++it) {
        std::string key = it->first;
        const datum_t *val = it->second;

        Term2 *arg = t->add_args();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N2(EQ,
           N2(GETATTR, pb::set_var(arg, x), pb::set_str(arg, key)),
           val->write_to_protobuf(pb::set_datum(arg)));
#pragma GCC diagnostic pop
    }
    return env->add_ptr(new func_t(env, &twrap->t));
}

val_t *func_t::_call(const std::vector<const datum_t *> &args) {
    rcheck(args.size() == argptrs.size(),
           strprintf("Passed %lu arguments to function of arity %lu.",
                     args.size(), argptrs.size()));
    for (size_t i = 0; i < args.size(); ++i) {
        r_sanity_check(args[i]);
        //debugf("Setting %p to %p\n", &argptrs[i], args[i]);
        argptrs[i] = args[i];
    }
    return body->eval(false);
    //                ^^^^^ don't use cached value
}

val_t *func_t::call(const datum_t *arg) {
    std::vector<const datum_t *> args;
    args.push_back(arg);
    return _call(args);
}

val_t *func_t::call(const datum_t *arg1, const datum_t *arg2) {
    std::vector<const datum_t *> args;
    args.push_back(arg1);
    args.push_back(arg2);
    return _call(args);
}

void func_t::dump_scope(std::map<int, Datum> *out) const {
    for (std::map<int, const datum_t **>::const_iterator
             it = scope.begin(); it != scope.end(); ++it) {
        if (!*it->second) continue;
        (*it->second)->write_to_protobuf(&(*out)[it->first]);
    }
}
bool func_t::is_deterministic() {
    return body->is_deterministic();
}

wire_func_t::wire_func_t() { }
wire_func_t::wire_func_t(env_t *env, func_t *func) {
    if (env) cached_funcs[env] = func;
    source = *func->source;
    func->dump_scope(&scope);
}
wire_func_t::wire_func_t(const Term2 &_source, std::map<int, Datum> *_scope)
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

func_term_t::func_term_t(env_t *env, const Term2 *term)
        : term_t(env), func(env->new_func(term)) { }
val_t *func_term_t::eval_impl() { return new_val(func); }
bool func_term_t::is_deterministic() {
    return func->is_deterministic();
}

} // namespace ql
