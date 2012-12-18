#include "rdb_protocol/func.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

func_t::func_t(env_t *env, const Term2 *_source) : source(_source) {
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
    for (size_t i = 0; i < args.size(); ++i) {
        argptrs.push_back(0);
        env->push_var(args[i], &argptrs[i]);
    }

    const Term2 *body_source = &t->args(1);
    body.init(compile_term(env, body_source));

    for (size_t i = 0; i < args.size(); ++i) env->pop_var(args[i]);
}

val_t *func_t::call(const std::vector<datum_t *> &args) {
    rcheck(args.size() == argptrs.size(),
           strprintf("Passed %lu arguments to function of arity %lu.",
                     args.size(), argptrs.size()));
    for (size_t i = 0; i < args.size(); ++i) argptrs[i] = args[i];
    return body->eval(false);
    //                ^^^^^ don't use cached value
}

wire_func_t::wire_func_t() : func(0) { }
wire_func_t::wire_func_t(env_t *env, func_t *_func) : func(_func) {
    source = *func->source;
    env->dump_scope(&scope);
}
func_t *wire_func_t::compile(env_t *env) {
    if (!func) func = env->new_func(&source);
    r_sanity_check(func);
    return func;
}

func_term_t::func_term_t(env_t *env, const Term2 *term)
        : term_t(env), func(env->new_func(term)) { }
val_t *func_term_t::eval_impl() { return new_val(func); }

} // namespace ql
