#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pb_utils.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

// This file implements terms that are rewritten into other terms.  See
// pb_utils.hpp for explanations of the macros.

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(env_t *env, protob_t<const Term> term, argspec_t argspec,
                   protob_t<Term> (*rewrite)(env_t *env,
                                             protob_t<const Term> in,
                                             protob_t<Term> out,
                                             const pb_rcheckable_t *bt_src))
        : term_t(env, term), in(term), out(make_counted_term()) {
        int args_size = in->args_size();
        rcheck(argspec.contains(args_size),
               base_exc_t::GENERIC,
               strprintf("Expected %s but found %d.",
                         argspec.print().c_str(), args_size));
        protob_t<Term> optarg_inheritor = rewrite(env, in, out, this);
        propagate(out.get()); // duplicates `in` backtrace (see `pb_rcheckable_t`)
        for (int i = 0; i < in->optargs_size(); ++i) {
            *optarg_inheritor->add_optargs() = in->optargs(i);
        }
        real = compile_term(env, out);
    }

private:
    virtual bool is_deterministic_impl() const { return real->is_deterministic(); }
    virtual counted_t<val_t> eval_impl(UNUSED eval_flags_t flags) { return real->eval(); }
    protob_t<const Term> in;
    protob_t<Term> out;

    counted_t<term_t> real;
};

class groupby_term_t : public rewrite_term_t {
public:
    groupby_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  const pb_rcheckable_t *bt_src) {
        std::string dc;
        Term dc_arg;
        parse_dc(&in->args(2), &dc, &dc_arg, bt_src);
        Term *arg = out.get();
        arg = final_wrap(env, arg, dc, &dc_arg);
        N4(GROUPED_MAP_REDUCE,
           *arg = in->args(0),
           *arg = in->args(1),
           map_fn(env, arg, dc, &dc_arg),
           reduce_fn(env, arg, dc, &dc_arg));
        return out;
    }

private:

    // This logic is ugly because we need to handle both MAKE_OBJ and R_OBJECT
    // as syntax rather than just parsing them both into an object (since we're
    // doing this at compile-time rather than runtime).
    static void parse_dc(const Term *t, std::string *dc_out,
                         Term *dc_arg_out, const pb_rcheckable_t *bt_src) {
        std::string errmsg = "Invalid aggregator for GROUPBY.";
        if (t->type() == Term::MAKE_OBJ) {
            rcheck_target(bt_src, base_exc_t::GENERIC,
                          t->optargs_size() == 1, errmsg);
            const Term_AssocPair *ap = &t->optargs(0);
            *dc_out = ap->key();
            rcheck_target(
                bt_src, base_exc_t::GENERIC,
                *dc_out == "SUM" || *dc_out == "AVG" || *dc_out == "COUNT",
                strprintf("Unrecognized GROUPBY aggregator `%s`.", dc_out->c_str()));
            *dc_arg_out = ap->val();
        } else if (t->type() == Term::DATUM) {
            rcheck_target(bt_src, base_exc_t::GENERIC, t->has_datum(), errmsg);
            const Datum *d = &t->datum();
            rcheck_target(bt_src, base_exc_t::GENERIC,
                          d->type() == Datum::R_OBJECT, errmsg);
            rcheck_target(bt_src, base_exc_t::GENERIC,
                          d->r_object_size() == 1, errmsg);
            const Datum_AssocPair *ap = &d->r_object(0);
            *dc_out = ap->key();
            rcheck_target(
                bt_src, base_exc_t::GENERIC,
                *dc_out == "SUM" || *dc_out == "AVG" || *dc_out == "COUNT",
                strprintf("Unrecognized GROUPBY aggregator `%s`.", dc_out->c_str()));
            dc_arg_out->set_type(Term::DATUM);
            *dc_arg_out->mutable_datum() = ap->val();
        } else {
            rcheck_target(bt_src, base_exc_t::GENERIC,
                          t->type() == Term::MAKE_OBJ, errmsg);
            unreachable();
        }
    }

    static void map_fn(env_t *env, Term *arg,
                       const std::string &dc, const Term *dc_arg) {
        const sym_t obj = env->gensym();
        const sym_t attr = env->gensym();
        arg = pb::set_func(arg, obj);
        if (dc == "COUNT") {
            NDATUM(1.0);
        } else if (dc == "SUM") {
            N2(FUNCALL, arg = pb::set_func(arg, attr);
               N3(BRANCH,
                  N2(HAS_FIELDS, NVAR(obj), NVAR(attr)),
                  N2(GET_FIELD, NVAR(obj), NVAR(attr)),
                  NDATUM(0.0)),
               *arg = *dc_arg);
        } else if (dc == "AVG") {
            N2(FUNCALL, arg = pb::set_func(arg, attr);
               N3(BRANCH,
                  N2(HAS_FIELDS, NVAR(obj), NVAR(attr)),
                  N2(MAKE_ARRAY, N2(GET_FIELD, NVAR(obj), NVAR(attr)), NDATUM(1.0)),
                  N2(MAKE_ARRAY, NDATUM(0.0), NDATUM(0.0))),
               *arg = *dc_arg);
        } else if (dc == "AVG") {
        } else { unreachable(); }
    }
    static void reduce_fn(env_t *env, Term *arg,
                          const std::string &dc, UNUSED const Term *dc_arg) {
        const sym_t a = env->gensym();
        const sym_t b = env->gensym();
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
        if (dc == "COUNT" || dc == "SUM") {
            return arg;
        }

        const sym_t val = env->gensym();
        const sym_t obj = env->gensym();
        Term *argout = 0;
        if (dc == "AVG") {
            N2(MAP, argout = arg, arg = pb::set_func(arg, obj);
               OPT2(MAKE_OBJ,
                    "group", N2(GET_FIELD, NVAR(obj), NDATUM("group")),
                    "reduction",
                    N2(FUNCALL, arg = pb::set_func(arg, val);
                       N2(DIV, N2(NTH, NVAR(val), NDATUM(0.0)),
                               N2(NTH, NVAR(val), NDATUM(1.0))),
                       N2(GET_FIELD, NVAR(obj), NDATUM("reduction")))));
        }
        return argout;
    }
    virtual const char *name() const { return "groupby"; }
};

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        const Term *const left = &in->args(0);
        const Term *const right = &in->args(1);
        const Term *const func = &in->args(2);
        const sym_t n = env->gensym();
        const sym_t m = env->gensym();

        Term *arg = out.get();
        // `left`.concatmap { |n|
        N2(CONCATMAP, *arg = *left, arg = pb::set_func(arg, n);
           // `right`.concatmap { |m|
           N2(CONCATMAP, *arg = *right, arg = pb::set_func(arg, m);
              // r.branch(
              N3(BRANCH,
                 // r.funcall(`func`, n, m),
                 N3(FUNCALL, *arg = *func, NVAR(n), NVAR(m)),
                 // [{:left => n, :right => m}],
                 N1(MAKE_ARRAY, OPT2(MAKE_OBJ, "left", NVAR(n), "right", NVAR(m))),
                 // [])}}
                 N0(MAKE_ARRAY))));
        return out;
    }

    virtual const char *name() const { return "inner_join"; }
};

class outer_join_term_t : public rewrite_term_t {
public:
    outer_join_term_t(env_t *env, const protob_t<const Term> &term) :
        rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        const Term *const left = &in->args(0);
        const Term *const right = &in->args(1);
        const Term *const func = &in->args(2);
        const sym_t n = env->gensym();
        const sym_t m = env->gensym();
        const sym_t lst = env->gensym();

        Term *arg = out.get();

        // `left`.concatmap { |n|
        N2(CONCATMAP, *arg = *left, arg = pb::set_func(arg, n);
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
                 // `right`.concatmap { |m|
                 N2(CONCATMAP, *arg = *right, arg = pb::set_func(arg, m);
                    // r.branch(
                    N3(BRANCH,
                       // r.funcall(`func`, n, m),
                       N3(FUNCALL, *arg = *func, NVAR(n), NVAR(m)),
                       // [{:left => n, :right => m}],
                       N1(MAKE_ARRAY, OPT2(MAKE_OBJ, "left", NVAR(n), "right", NVAR(m))),
                       // [])},
                       N0(MAKE_ARRAY))),
                 // "ARRAY"))}
                 NDATUM("ARRAY"))));
        return out;
    }

    virtual const char *name() const { return "outer_join"; }
};

class eq_join_term_t : public rewrite_term_t {
public:
    eq_join_term_t(env_t *env, const protob_t<const Term> &term) :
        rewrite_term_t(env, term, argspec_t(3), rewrite) { }
private:

    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        const Term *const left = &in->args(0);
        const Term *const left_attr = &in->args(1);
        const Term *const right = &in->args(2);
        const sym_t row = env->gensym();
        const sym_t v = env->gensym();

        Term *arg = out.get();
        Term *optarg_inheritor = NULL;
        N2(CONCATMAP, *arg = *left, arg = pb::set_func(arg, row);
           N2(MAP,
              optarg_inheritor = arg;
              N2(GET_ALL, *arg = *right, N2(FUNCALL, *arg = *left_attr, NVAR(row));
                  OPT1(FUNCALL, "_SHORTCUT_", NDATUM(static_cast<double>(GET_FIELD_SHORTCUT)))),

              arg = pb::set_func(arg, v);
              OPT2(MAKE_OBJ, "left", NVAR(row), "right", NVAR(v))));
        r_sanity_check(optarg_inheritor != NULL);
        return out.make_child(optarg_inheritor);
    }
    virtual const char *name() const { return "inner_join"; }
};

class delete_term_t : public rewrite_term_t {
public:
    delete_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(1), rewrite) { }
private:

    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        const sym_t x = env->gensym();

        Term *arg = out.get();
        N2(REPLACE, *arg = in->args(0), pb::set_null(pb::set_func(arg, x)));
        return out;
     }
     virtual const char *name() const { return "delete"; }
};

class update_term_t : public rewrite_term_t {
public:
    update_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        // The `false` values below mean that we don't bind the implicit variable.
        const sym_t old_row = env->symgen.gensym(false);
        const sym_t new_row = env->symgen.gensym(false);

        Term *arg = out.get();
        N2(REPLACE, *arg = in->args(0), arg = pb::set_func(arg, old_row);
           N3(BRANCH,
              N2(EQ, NVAR(old_row), NDATUM(datum_t::R_NULL)),
              NDATUM(datum_t::R_NULL),
              N2(FUNCALL, arg = pb::set_func(arg, new_row);
                 N3(BRANCH,
                    N2(EQ, NVAR(new_row), NDATUM(datum_t::R_NULL)),
                    NVAR(old_row),
                    N2(MERGE, NVAR(old_row), NVAR(new_row))),
                 N2(FUNCALL, *arg = in->args(1), NVAR(old_row));
                 OPT1(FUNCALL, "_EVAL_FLAGS_",
                      NDATUM(static_cast<double>(LITERAL_OK))))
              OPT1(FUNCALL, "_EVAL_FLAGS_",
                NDATUM(static_cast<double>(LITERAL_OK)))));
        return out;
    }
    virtual const char *name() const { return "update"; }
};

class skip_term_t : public rewrite_term_t {
public:
    skip_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static protob_t<Term> rewrite(UNUSED env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        Term *arg = out.get();
        N3(SLICE, *arg = in->args(0), *arg = in->args(1), NDATUM(-1.0));
        OPT1(SLICE, "right_bound", NDATUM("closed"));
        return out;
     }
     virtual const char *name() const { return "skip"; }
};

class difference_term_t : public rewrite_term_t {
public:
    difference_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static protob_t<Term> rewrite(env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        const sym_t row = env->symgen.gensym(false);

        Term *arg = out.get();
        N2(FILTER, *arg = in->args(0), arg = pb::set_func(arg, row);
           N1(NOT, N2(CONTAINS, *arg = in->args(1), NVAR(row))));

        return out;
    }

     virtual const char *name() const { return "difference"; }
};

class with_fields_term_t : public rewrite_term_t {
public:
    with_fields_term_t(env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(1, -1), rewrite) { }
private:
    static protob_t<Term> rewrite(UNUSED env_t *env, protob_t<const Term> in,
                                  const protob_t<Term> out,
                                  UNUSED const pb_rcheckable_t *bt_src) {
        Term *arg = out.get();
        Term *pluck = arg;
        Term *has_fields = NULL;
        N1(PLUCK,
           has_fields = arg;
           N1(HAS_FIELDS, *arg = in->args(0)));
        r_sanity_check(has_fields != NULL);
        for (int i = 1; i < in->args_size(); ++i) {
            *pluck->add_args() = in->args(i);
            *has_fields->add_args() = in->args(i);
        }
        return out.make_child(has_fields);

    }
     virtual const char *name() const { return "with_fields"; }
};

counted_t<term_t> make_skip_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<skip_term_t>(env, term);
}
counted_t<term_t> make_groupby_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<groupby_term_t>(env, term);
}
counted_t<term_t> make_inner_join_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<inner_join_term_t>(env, term);
}
counted_t<term_t> make_outer_join_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<outer_join_term_t>(env, term);
}
counted_t<term_t> make_eq_join_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<eq_join_term_t>(env, term);
}
counted_t<term_t> make_update_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<update_term_t>(env, term);
}
counted_t<term_t> make_delete_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<delete_term_t>(env, term);
}
counted_t<term_t> make_difference_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<difference_term_t>(env, term);
}
counted_t<term_t> make_with_fields_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<with_fields_term_t>(env, term);
}

} // namespace ql
