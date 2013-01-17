#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class gmr_term_t : public op_term_t {
public:
    gmr_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(4)) { }
private:
    virtual val_t *eval_impl() {
        return new_val(arg(0)->as_seq()->gmr(arg(1)->as_func(),
                                             arg(2)->as_func(),
                                             arg(3)->as_func()));
    }
    RDB_NAME("grouped_map_reduce")
};

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(env_t *env, const Term2 *term,
                   void (*rewrite)(env_t *, const Term2 *, Term2 *))
        : term_t(env), in(term) {
        rewrite(env, in, &out);
        debugf("%s\n--->\n%s\n", in->DebugString().c_str(), out.DebugString().c_str());
        real.init(compile_term(env, &out));
    }
private:
    virtual val_t *eval_impl() { return real->eval(use_cached_val); }
    const Term2 *in;
    Term2 out;

    scoped_ptr_t<term_t> real;
};

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(env_t *env, const Term2 *term) :
        rewrite_term_t(env, term, rewrite) { }
    static void rewrite(env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 3, "Inner Join requires 3 arguments.");
        const Term2 *l = &in->args(0);
        const Term2 *r = &in->args(1);
        const Term2 *f = &in->args(2);
        int n = env->gensym();
        int m = env->gensym();

        Term2 *arg = out;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N2(CONCATMAP, *arg = *l, arg = pb::set_func(arg, n);
           N2(CONCATMAP, *arg = *r, arg = pb::set_func(arg, m);
              N3(BRANCH,
                 N3(FUNCALL, *arg = *f, pb::set_var(arg, n), pb::set_var(arg, m)),
                 N1(MAKE_ARRAY,
                    OPT2(MAKE_OBJ,
                         "left", pb::set_var(arg, n),
                         "right", pb::set_var(arg, m))),
                 N0(MAKE_ARRAY))))
#pragma GCC diagnostic pop
    }
    RDB_NAME("inner_join")
};

} // namespace ql
