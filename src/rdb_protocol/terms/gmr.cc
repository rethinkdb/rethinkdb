// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {

class gmr_term_t : public op_term_t {
public:
    gmr_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(4), optargspec_t({ "base" })) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> baseval = optarg(env, "base");
        counted_t<const datum_t> base = baseval.has() ?
            baseval->as_datum() :
            counted_t<const datum_t>();
        counted_t<func_t> g = arg(env, 1)->as_func(PLUCK_SHORTCUT);
        counted_t<func_t> m = arg(env, 2)->as_func();
        counted_t<func_t> r = arg(env, 3)->as_func();
        return new_val(arg(env, 0)->as_seq(env->env)->gmr(env->env, g, m, base, r));
    }
    virtual const char *name() const { return "grouped_map_reduce"; }
};

counted_t<term_t> make_gmr_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<gmr_term_t>(env, term);
}

} // namespace ql
