#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(env_t *env, const Term2 *term,
                   void (*rewrite)(env_t *, const Term2 *, Term2 *))
        : term_t(env), in(term) {
        rewrite(env, in, &out);
        for (int i = 0; i < in->optargs_size(); ++i) {
            *out.add_optargs() = in->optargs(i);
        }
        //debugf("%s\n--->\n%s\n", in->DebugString().c_str(), out.DebugString().c_str());
        real.init(compile_term(env, &out));
    }
private:
    virtual bool is_deterministic() { return real->is_deterministic(); }
    virtual val_t *eval_impl() { return real->eval(use_cached_val); }
    const Term2 *in;
    Term2 out;

    scoped_ptr_t<term_t> real;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
class groupby_term_t : public rewrite_term_t {
public:
    groupby_term_t(env_t *env, const Term2 *term)
        : rewrite_term_t(env, term, rewrite) { }
    static void rewrite(env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 3, "Groupby requires 3 arguments.");
        std::string dc;
        const Term2 *dc_arg;
        parse_dc(&in->args(2), &dc, &dc_arg);
        Term2 *arg = out;
        arg = final_wrap(env, arg, dc, dc_arg);
        N4(GROUPED_MAP_REDUCE,
           *arg = in->args(0),
           group_fn(env, arg, &in->args(1)),
           map_fn(env, arg, dc, dc_arg),
           reduce_fn(env, arg, dc, dc_arg));
    }
private:
    static void parse_dc(const Term2 *t, std::string *dc_out,
                         const Term2 **dc_arg_out) {
        rcheck(t->type() == Term2_TermType_MAKE_OBJ, "Invalid data collector.");
        rcheck(t->optargs_size() == 1, "Invalid data collector.");
        const Term2_AssocPair *ap = &t->optargs(0);
        *dc_out = ap->key();
        rcheck(*dc_out == "SUM" || *dc_out == "AVG" || *dc_out == "COUNT",
               strprintf("Unrecognized data collector `%s`.", dc_out->c_str()));
        *dc_arg_out = &ap->val();
    }
    static void group_fn(env_t *env, Term2 *arg, const Term2 *group_attrs) {
        int obj = env->gensym();
        int attr = env->gensym();
        arg = pb::set_func(arg, obj);
        N2(MAP, *arg = *group_attrs, arg = pb::set_func(arg, attr);
           N3(BRANCH,
              N2(CONTAINS, NVAR(obj), NVAR(attr)),
              N2(GETATTR, NVAR(obj), NVAR(attr)),
              NDATUM(datum_t::R_NULL)));
        // debugf("%s\n", arg->DebugString().c_str());
    }
    static void map_fn(env_t *env, Term2 *arg,
                       const std::string &dc, const Term2 *dc_arg) {
        int obj = env->gensym(), attr = env->gensym();
        arg = pb::set_func(arg, obj);
        if (dc == "COUNT") {
            NDATUM(1);
        } else if (dc == "SUM") {
            N2(FUNCALL, arg = pb::set_func(arg, attr);
               N3(BRANCH,
                  N2(CONTAINS, NVAR(obj), NVAR(attr)),
                  N2(GETATTR, NVAR(obj), NVAR(attr)),
                  NDATUM(0)),
               *arg = *dc_arg);
        } else if (dc == "AVG") {
            N2(FUNCALL, arg = pb::set_func(arg, attr);
               N3(BRANCH,
                  N2(CONTAINS, NVAR(obj), NVAR(attr)),
                  N2(MAKE_ARRAY, N2(GETATTR, NVAR(obj), NVAR(attr)), NDATUM(1)),
                  N2(MAKE_ARRAY, NDATUM(0), NDATUM(0))),
               *arg = *dc_arg);
        } else { unreachable(); }
    }
    static void reduce_fn(env_t *env, Term2 *arg,
                          const std::string &dc, UNUSED const Term2 *dc_arg) {
        int a = env->gensym(), b = env->gensym();
        arg = pb::set_func(arg, a, b);
        if (dc == "COUNT" || dc == "SUM") {
            N2(ADD, NVAR(a), NVAR(b));
        } else if (dc == "AVG") {
            N2(MAKE_ARRAY,
               N2(ADD, N2(NTH, NVAR(a), NDATUM(0)),
                       N2(NTH, NVAR(b), NDATUM(0))),
               N2(ADD, N2(NTH, NVAR(a), NDATUM(1)),
                       N2(NTH, NVAR(b), NDATUM(1))));
        } else { unreachable(); }
    }
    static Term2 *final_wrap(env_t *env, Term2 *arg,
                            const std::string &dc, UNUSED const Term2 *dc_arg) {
        if (dc == "COUNT" || dc == "SUM") return arg;

        int val = env->gensym(), obj = env->gensym();
        Term2 *argout = 0;
        if (dc == "AVG") {
            N2(MAP, argout = arg, arg = pb::set_func(arg, obj);
               OPT2(MAKE_OBJ,
                    "group", N2(GETATTR, NVAR(obj), NDATUM("group")),
                    "reduction",
                    N2(FUNCALL, arg = pb::set_func(arg, val);
                       N2(DIV, N2(NTH, NVAR(val), NDATUM(0)),
                          N2(NTH, NVAR(val), NDATUM(1))),
                       N2(GETATTR, NVAR(obj), NDATUM("reduction")))));
        }
        return argout;
    }
    RDB_NAME("groupby");
};
#pragma GCC diagnostic pop

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(env_t *env, const Term2 *term)
        : rewrite_term_t(env, term, rewrite) { }
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
        // `l`.concatmap { |n|
        N2(CONCATMAP, *arg = *l, arg = pb::set_func(arg, n);
           // `r`.concatmap { |m|
           N2(CONCATMAP, *arg = *r, arg = pb::set_func(arg, m);
              // r.branch(
              N3(BRANCH,
                 // r.funcall(`f`, n, m),
                 N3(FUNCALL, *arg = *f, NVAR(n), NVAR(m)),
                 // [{:left => n, :right => m}],
                 N1(MAKE_ARRAY, OPT2(MAKE_OBJ, "left", NVAR(n), "right", NVAR(m))),
                 // [])}}
                 N0(MAKE_ARRAY))));
#pragma GCC diagnostic pop
    }
    RDB_NAME("inner_join")
};

class outer_join_term_t : public rewrite_term_t {
public:
    outer_join_term_t(env_t *env, const Term2 *term) :
        rewrite_term_t(env, term, rewrite) { }
    static void rewrite(env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 3, "Outer Join requires 3 arguments.");
        const Term2 *l = &in->args(0), *r = &in->args(1), *f = &in->args(2);
        int n = env->gensym(), m = env->gensym(), lst = env->gensym();

        Term2 *arg = out;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        // `l`.concatmap { |n|
        N2(CONCATMAP, *arg = *l, arg = pb::set_func(arg, n);
           // r.funcall(lambda { |lst
           N2(FUNCALL, arg = pb::set_func(arg, lst);
              // r.branch(
              N3(BRANCH,
                 // r.gt(r.count(lst), 0),
                 N2(GT, N1(COUNT, NVAR(lst)), NDATUM(0)),
                 // lst,
                 NVAR(lst),
                 // [{:left => n}])},
                 N1(MAKE_ARRAY, OPT1(MAKE_OBJ, "left", NVAR(n)))),
              // r.coerce(
              N2(COERCE,
                 // `r`.concatmap { |m|
                 N2(CONCATMAP, *arg = *r, arg = pb::set_func(arg, m);
                    // r.branch(
                    N3(BRANCH,
                       // r.funcall(`f`, n, m),
                       N3(FUNCALL, *arg = *f, NVAR(n), NVAR(m)),
                       // [{:left => n, :right => m}],
                       N1(MAKE_ARRAY, OPT2(MAKE_OBJ, "left", NVAR(n), "right", NVAR(m))),
                       // [])},
                       N0(MAKE_ARRAY))),
                 // "ARRAY"))}
                 NDATUM("ARRAY"))));
#pragma GCC diagnostic pop
    }
    RDB_NAME("outer_join")
};

class eq_join_term_t : public rewrite_term_t {
public:
    eq_join_term_t(env_t *env, const Term2 *term) :
        rewrite_term_t(env, term, rewrite) { }
private:
    static void rewrite(env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 3, "eq_join requires 3 arguments.");
        const Term2 *l = &in->args(0), *lattr = &in->args(1), *r = &in->args(2);
        int row = env->gensym(), v = env->gensym();

        Term2 *arg = out;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        // `l`.concat_map { |row|
        N2(CONCATMAP, *arg = *l, arg = pb::set_func(arg, row);
           // r.funcall(lambda { |v|
           N2(FUNCALL, arg = pb::set_func(arg, v);
              // r.branch(
              N3(BRANCH,
                 // r.ne(v, nil),
                 N2(NE, NVAR(v), NDATUM(datum_t::R_NULL)),
                 // [{:left => row, :right => v}],
                 N1(MAKE_ARRAY, OPT2(MAKE_OBJ, "left", NVAR(row), "right", NVAR(v))),
                 // []),
                 N0(MAKE_ARRAY)),
              // `r`.get(l[`lattr`]))}
              N2(GET, *arg = *r, N2(GETATTR, NVAR(row), *arg = *lattr))));
#pragma GCC diagnostic pop
    }
    RDB_NAME("inner_join")
};

class delete_term_t : public rewrite_term_t {
public:
    delete_term_t(env_t *env, const Term2 *term) : rewrite_term_t(env, term, rewrite) { }
private:
    static void rewrite(env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 1, "delete requires 1 argument");
        int x = env->gensym();

        Term2 *arg = out;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N2(REPLACE, *arg = in->args(0), pb::set_null(pb::set_func(arg, x)));
#pragma GCC diagnostic pop
     }
     RDB_NAME("delete")
};

class update_term_t : public rewrite_term_t {
public:
    update_term_t(env_t *env, const Term2 *term) : rewrite_term_t(env, term, rewrite) { }
private:
    static void rewrite(env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 2, "update requires 2 arguments");
        int old_row = env->gensym();
        int new_row = env->gensym();

        Term2 *arg = out;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N2(REPLACE, *arg = in->args(0), arg = pb::set_func(arg, old_row);
           N3(BRANCH,
              N2(EQ, NVAR(old_row), NDATUM(datum_t::R_NULL)),
              NDATUM(datum_t::R_NULL),
              N2(FUNCALL, arg = pb::set_func(arg, new_row);
                 N3(BRANCH,
                    N2(EQ, NVAR(new_row), NDATUM(datum_t::R_NULL)),
                    NVAR(old_row),
                    N2(MERGE, NVAR(old_row), NVAR(new_row))),
                 N2(FUNCALL, *arg = in->args(1), NVAR(old_row)))));
#pragma GCC diagnostic pop
    }
    RDB_NAME("update")
};

class skip_term_t : public rewrite_term_t {
public:
    skip_term_t(env_t *env, const Term2 *term) : rewrite_term_t(env, term, rewrite) { }
private:
    static void rewrite(UNUSED env_t *env, const Term2 *in, Term2 *out) {
        rcheck(in->args_size() == 2, "skip requires 2 arguments");
        Term2 *arg = out;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        N3(SLICE, *arg = in->args(0), *arg = in->args(1), NDATUM(-1));
#pragma GCC diagnostic pop
     }
     RDB_NAME("skip")
};

} // namespace ql
