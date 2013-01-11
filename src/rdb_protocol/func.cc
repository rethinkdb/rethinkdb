#include "rdb_protocol/func.hpp"
#include "rdb_protocol/env.hpp"
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
    guarantee(env->top_var(args[0]) == &argptrs[0]);

    const Term2 *body_source = &t->args(1);
    body = env->new_term(body_source);

    for (size_t i = 0; i < args.size(); ++i) {
        //debugf("popping %d\n", args[i]);
        env->pop_var(args[i]);
        if (args.size() == 1) env->pop_implicit();
    }
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

wire_func_t::wire_func_t() { }
wire_func_t::wire_func_t(env_t *env, func_t *func) {
    cached_funcs[env] = func;
    source = *func->source;
    env->dump_scope(&scope);
}
func_t *wire_func_t::compile(env_t *env) {
    if (cached_funcs.count(env) > 0) return cached_funcs[env];
    return (cached_funcs[env] = env->new_func(&source));
}

func_term_t::func_term_t(env_t *env, const Term2 *term)
        : term_t(env), func(env->new_func(term)) { }
val_t *func_term_t::eval_impl() { return new_val(func); }

} // namespace ql
