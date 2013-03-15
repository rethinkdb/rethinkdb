#ifndef RDB_PROTOCOL_TERMS_REWRITES_HPP_
#define RDB_PROTOCOL_TERMS_REWRITES_HPP_

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/pb_utils.hpp"

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace ql {

// This file implements terms that are rewritten into other terms.  See
// pb_utils.hpp for explanations of the macros.

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(env_t *env, const Term *term, argspec_t argspec,
                   void (*rewrite)(env_t *, const Term *, Term *,
                                   const pb_rcheckable_t *))
        : term_t(env, term), in(term) {
        int args_size = in->args_size();
        rcheck(argspec.contains(args_size),
               strprintf("Expected %s but found %d.",
                         argspec.print().c_str(), args_size));
        rewrite(env, in, &out, this);
        propagate(&out); // duplicates `in` backtrace (see `pb_rcheckable_t`)
        for (int i = 0; i < in->optargs_size(); ++i) {
            *out.add_optargs() = in->optargs(i);
        }
        //debugf("%s\n--->\n%s\n", in->DebugString().c_str(), out.DebugString().c_str());
        real.init(compile_term(env, &out));
    }
private:

    virtual bool is_deterministic_impl() const { return real->is_deterministic(); }
    virtual val_t *eval_impl() { return real->eval(use_cached_val); }
    const Term *in;
    Term out;

    scoped_ptr_t<term_t> real;
};

class groupby_term_t : public rewrite_term_t {
public:
    groupby_term_t(env_t *env, const Term *term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }
    static void rewrite(env_t *env, const Term *in, Term *out,
                        const pb_rcheckable_t *bt_src) {
        std::string dc;
        const Term *dc_arg;
        parse_dc(&in->args(2), &dc, &dc_arg, bt_src);
        Term *arg = out;
        arg = final_wrap(env, arg, dc, dc_arg);
        N4(GROUPED_MAP_REDUCE,
           *arg = in->args(0),
           group_fn(env, arg, &in->args(1)),
           map_fn(env, arg, dc, dc_arg),
           reduce_fn(env, arg, dc, dc_arg));
    }
private:
    static void parse_dc(const Term *t, std::string *dc_out,
                         const Term **dc_arg_out, const pb_rcheckable_t *bt_src) {
        rcheck_target(bt_src, t->type() == Term_TermType_MAKE_OBJ,
                      "Invalid data collector.");
        rcheck_target(bt_src, t->optargs_size() == 1, "Invalid data collector.");
        const Term_AssocPair *ap = &t->optargs(0);
        *dc_out = ap->key();
        rcheck_target(bt_src, *dc_out == "SUM" || *dc_out == "AVG" || *dc_out == "COUNT",
               strprintf("Unrecognized data collector `%s`.", dc_out->c_str()));
        *dc_arg_out = &ap->val();
    }
    static void group_fn(env_t *env, Term *arg, const Term *group_attrs) {
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
    static void map_fn(env_t *env, Term *arg,
                       const std::string &dc, const Term *dc_arg) {
        int obj = env->gensym(), attr = env->gensym();
        arg = pb::set_func(arg, obj);
        if (dc == "COUNT") {
            NDATUM(1.0);
        } else if (dc == "SUM") {
            N2(FUNCALL, arg = pb::set_func(arg, attr);
               N3(BRANCH,
                  N2(CONTAINS, NVAR(obj), NVAR(attr)),
                  N2(GETATTR, NVAR(obj), NVAR(attr)),
                  NDATUM(0.0)),
               *arg = *dc_arg);
        } else if (dc == "AVG") {
            N2(FUNCALL, arg = pb::set_func(arg, attr);
               N3(BRANCH,
                  N2(CONTAINS, NVAR(obj), NVAR(attr)),
                  N2(MAKE_ARRAY, N2(GETATTR, NVAR(obj), NVAR(attr)), NDATUM(1.0)),
                  N2(MAKE_ARRAY, NDATUM(0.0), NDATUM(0.0))),
               *arg = *dc_arg);
        } else if (dc == "AVG") {
        } else { unreachable(); }
    }
    static void reduce_fn(env_t *env, Term *arg,
                          const std::string &dc, UNUSED const Term *dc_arg) {
        int a = env->gensym(), b = env->gensym();
        arg = pb::set_func(arg, a, b);
        if (dc == "COUNT" || dc == "SUM") {
            N2(ADD, NVAR(a), NVAR(b));
        } else if (dc == "AVG") {
            N2(MAKE_ARRAY,
               N2(ADD, N2(NTH, NVAR(a), NDATUM(0.0)),
                       N2(NTH, NVAR(b), NDATUM(0.0))),
               N2(ADD, N2(NTH, NVAR(a), NDATUM(1.0)),
                       N2(NTH, NVAR(b), NDATUM(1.0))));
        } else { unreachable(); }
    }
    static Term *final_wrap(env_t *env, Term *arg,
                            const std::string &dc, UNUSED const Term *dc_arg) {
        if (dc == "COUNT" || dc == "SUM") return arg;

        int val = env->gensym(), obj = env->gensym();
        Term *argout = 0;
        if (dc == "AVG") {
            N2(MAP, argout = arg, arg = pb::set_func(arg, obj);
               OPT2(MAKE_OBJ,
                    "group", N2(GETATTR, NVAR(obj), NDATUM("group")),
                    "reduction",
                    N2(FUNCALL, arg = pb::set_func(arg, val);
                       N2(DIV, N2(NTH, NVAR(val), NDATUM(0.0)),
                               N2(NTH, NVAR(val), NDATUM(1.0))),
                       N2(GETATTR, NVAR(obj), NDATUM("reduction")))));
        }
        return argout;
    }
    virtual const char *name() const { return "groupby"; }
};

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(env_t *env, const Term *term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }
    static void rewrite(env_t *env, const Term *in, Term *out,
                        UNUSED const pb_rcheckable_t *bt_src) {
        const Term *l = &in->args(0);
        const Term *r = &in->args(1);
        const Term *f = &in->args(2);
        int n = env->gensym();
        int m = env->gensym();

        Term *arg = out;
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
    }
    virtual const char *name() const { return "inner_join"; }
};

class outer_join_term_t : public rewrite_term_t {
public:
    outer_join_term_t(env_t *env, const Term *term) :
        rewrite_term_t(env, term, argspec_t(3), rewrite) { }
    static void rewrite(env_t *env, const Term *in, Term *out,
                        UNUSED const pb_rcheckable_t *bt_src) {
        const Term *l = &in->args(0), *r = &in->args(1), *f = &in->args(2);
        int64_t n = env->gensym(), m = env->gensym(), lst = env->gensym();

        Term *arg = out;

        // `l`.concatmap { |n|
        N2(CONCATMAP, *arg = *l, arg = pb::set_func(arg, n);
           // r.funcall(lambda { |lst
           N2(FUNCALL, arg = pb::set_func(arg, lst);
              // r.branch(
              N3(BRANCH,
                 // r.gt(r.count(lst), 0),
                 N2(GT, N1(COUNT, NVAR(lst)), NDATUM(0.0)),
                 // lst,
                 NVAR(lst),
                 // [{:left => n}])},
                 N1(MAKE_ARRAY, OPT1(MAKE_OBJ, "left", NVAR(n)))),
              // r.coerce_to(
              N2(COERCE_TO,
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
    }
    virtual const char *name() const { return "outer_join"; }
};

class eq_join_term_t : public rewrite_term_t {
public:
    eq_join_term_t(env_t *env, const Term *term) :
        rewrite_term_t(env, term, argspec_t(3), rewrite) { }
private:
    static void rewrite(env_t *env, const Term *in, Term *out,
                        UNUSED const pb_rcheckable_t *bt_src) {
        const Term *l = &in->args(0), *lattr = &in->args(1), *r = &in->args(2);
        int row = env->gensym(), v = env->gensym();

        Term *arg = out;
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
    }
    virtual const char *name() const { return "inner_join"; }
};

class delete_term_t : public rewrite_term_t {
public:
    delete_term_t(env_t *env, const Term *term)
        : rewrite_term_t(env, term, argspec_t(1), rewrite) { }
private:
    static void rewrite(env_t *env, const Term *in, Term *out,
                        UNUSED const pb_rcheckable_t *bt_src) {
        int x = env->gensym();

        Term *arg = out;
        N2(REPLACE, *arg = in->args(0), pb::set_null(pb::set_func(arg, x)));
     }
     virtual const char *name() const { return "delete"; }
};

class update_term_t : public rewrite_term_t {
public:
    update_term_t(env_t *env, const Term *term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static void rewrite(env_t *env, const Term *in, Term *out,
                        UNUSED const pb_rcheckable_t *bt_src) {
        // The `false` values below mean that we don't bind the implicit variable.
        int old_row = env->gensym(false);
        int new_row = env->gensym(false);

        Term *arg = out;
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
    }
    virtual const char *name() const { return "update"; }
};

class skip_term_t : public rewrite_term_t {
public:
    skip_term_t(env_t *env, const Term *term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static void rewrite(UNUSED env_t *env, const Term *in, Term *out,
                        UNUSED const pb_rcheckable_t *bt_src) {
        Term *arg = out;
        N3(SLICE, *arg = in->args(0), *arg = in->args(1), NDATUM(-1.0));
     }
     virtual const char *name() const { return "skip"; }
};

} // namespace ql

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic pop
#endif

#endif // RDB_PROTOCOL_TERMS_REWRITES_HPP_
