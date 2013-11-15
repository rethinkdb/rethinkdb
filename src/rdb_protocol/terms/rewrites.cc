// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/minidriver.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

// This file implements terms that are rewritten into other terms.

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(compile_env_t *env, protob_t<const Term> term, argspec_t argspec,
                   r::reql_t (*rewrite)(protob_t<const Term> in,
                                        const pb_rcheckable_t *bt_src,
                                        protob_t<const Term> optargs_in))
        : term_t(term), in(term), out(make_counted_term()) {
        int args_size = in->args_size();
        rcheck(argspec.contains(args_size),
               base_exc_t::GENERIC,
               strprintf("Expected %s but found %d.",
                         argspec.print().c_str(), args_size));
        out->Swap(&rewrite(in, this, in).get());
        propagate(out.get()); // duplicates `in` backtrace (see `pb_rcheckable_t`)
        real = compile_term(env, out);
    }

private:
    virtual void accumulate_captures(var_captures_t *captures) const {
        return real->accumulate_captures(captures);
    }
    virtual bool is_deterministic() const {
        return real->is_deterministic();
    }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return real->eval(env);
    }

    protob_t<const Term> in;
    protob_t<Term> out;

    counted_t<term_t> real;
};

class groupby_term_t : public rewrite_term_t {
public:
    groupby_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static r::reql_t rewrite(protob_t<const Term> in,
                             const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        std::string dc;
        r::reql_t dc_arg = parse_dc(&in->args(2), &dc, bt_src);
        r::reql_t gmr =
            r::expr(in->args(0)).grouped_map_reduce(
                in->args(1),
                map_fn(dc, &dc_arg),
                reduce_fn(dc, &dc_arg));
        r::reql_t term = final_wrap(std::move(gmr), dc, &dc_arg);
        term.copy_optargs_from_term(*optargs_in);
        return term;
    }

private:

    // This logic is ugly because we need to handle both MAKE_OBJ and R_OBJECT
    // as syntax rather than just parsing them both into an object (since we're
    // doing this at compile-time rather than runtime).
    static r::reql_t parse_dc(const Term *t, std::string *dc_out,
                              const pb_rcheckable_t *bt_src) {
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
            return r::expr(ap->val());
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
            return r::expr(ap->val());
        } else {
            rcheck_target(bt_src, base_exc_t::GENERIC,
                          t->type() == Term::MAKE_OBJ, errmsg);
            unreachable();
        }
    }

    static r::reql_t map_fn(const std::string &dc, const r::reql_t *dc_arg) {
        auto obj = pb::dummy_var_t::GROUPBY_MAP_OBJ;
        if (dc == "COUNT") {
            return r::fun(obj, r::expr(1.0));
        } else if (dc == "SUM") {
            auto attr = pb::dummy_var_t::GROUPBY_MAP_ATTR;
            return
                r::fun(obj,
                    dc_arg->copy().do_(attr,
                        r::branch(
                            r::var(obj).has_fields(r::var(attr)),
                            r::var(obj)[r::var(attr)],
                            0.0)));
        } else if (dc == "AVG") {
            auto attr = pb::dummy_var_t::GROUPBY_MAP_ATTR;
            return
                r::fun(obj,
                    dc_arg->copy().do_(attr,
                        r::branch(
                            r::var(obj).has_fields(r::var(attr)),
                            r::array(r::var(obj)[r::var(attr)], 1.0),
                            r::array(0.0, 0.0))));
        } else { unreachable(); }
    }
    static r::reql_t reduce_fn(const std::string &dc, UNUSED const r::reql_t *dc_arg) {
        auto a = pb::dummy_var_t::GROUPBY_REDUCE_A;
        auto b = pb::dummy_var_t::GROUPBY_REDUCE_B;
        if (dc == "COUNT" || dc == "SUM") {
            return r::fun(a, b, r::var(a) + r::var(b));
        } else if (dc == "AVG") {
            return
                r::fun(a, b,
                    r::array(r::var(a).nth(0) + r::var(b).nth(0),
                        r::var(a).nth(1) + r::var(b).nth(1)));
        } else { unreachable(); }
    }
    static r::reql_t final_wrap(r::reql_t arg,
                                const std::string &dc, UNUSED const r::reql_t *dc_arg) {
        if (dc == "COUNT" || dc == "SUM") {
            return arg;
        }

        if (dc == "AVG") {
            auto obj = pb::dummy_var_t::GROUPBY_FINAL_OBJ;
            auto val = pb::dummy_var_t::GROUPBY_FINAL_VAL;
            return
                std::move(arg).map(
                    r::fun(obj,
                        r::object(
                            r::optarg("group", r::var(obj)["group"]),
                            r::optarg("reduction",
                                r::var(obj)["reduction"].do_(val,
                                    r::var(val).nth(0) / r::var(val).nth(1))))));
        } else {
            unreachable();
        }
    }
    virtual const char *name() const { return "groupby"; }
};

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        const Term &left = in->args(0);
        const Term &right = in->args(1);
        const Term &func = in->args(2);
        auto n = pb::dummy_var_t::INNERJOIN_N;
        auto m = pb::dummy_var_t::INNERJOIN_M;

        r::reql_t term =
            r::expr(left).concat_map(
                r::fun(n,
                    r::expr(right).concat_map(
                        r::fun(m,
                            r::branch(
                                r::expr(func)(r::var(n), r::var(m)),
                                r::array(r::object(
                                        r::optarg("left", n), r::optarg("right", m))),
                                r::array())))));

        term.copy_optargs_from_term(*optargs_in);
        return term;
    }

    virtual const char *name() const { return "inner_join"; }
};

class outer_join_term_t : public rewrite_term_t {
public:
    outer_join_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        const Term &left = in->args(0);
        const Term &right = in->args(1);
        const Term &func = in->args(2);
        auto n = pb::dummy_var_t::OUTERJOIN_N;
        auto m = pb::dummy_var_t::OUTERJOIN_M;
        auto lst = pb::dummy_var_t::OUTERJOIN_LST;

        r::reql_t inner_concat_map =
            r::expr(right).concat_map(
                r::fun(m,
                    r::branch(
                        r::expr(func)(n, m),
                        r::array(r::object(r::optarg("left", n), r::optarg("right", m))),
                        r::array())));

        r::reql_t term =
            r::expr(left).concat_map(
                r::fun(n,
                    std::move(inner_concat_map).coerce_to("ARRAY").do_(lst,
                        r::branch(
                            r::expr(lst).count() > 0,
                            lst,
                            r::array(r::object(r::optarg("left", n)))))));

        term.copy_optargs_from_term(*optargs_in);
        return term;
    }

    virtual const char *name() const { return "outer_join"; }
};

class eq_join_term_t : public rewrite_term_t {
public:
    eq_join_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        rewrite_term_t(env, term, argspec_t(3), rewrite) { }
private:

    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        const Term &left = in->args(0);
        const Term &left_attr = in->args(1);
        const Term &right = in->args(2);

        auto row = pb::dummy_var_t::EQJOIN_ROW;
        auto v = pb::dummy_var_t::EQJOIN_V;

        r::reql_t get_all =
            r::expr(right).get_all(
                r::expr(left_attr)(row, r::optarg("_SHORTCUT_", GET_FIELD_SHORTCUT)));

        get_all.copy_optargs_from_term(*optargs_in);

        return
            r::expr(left).concat_map(
                r::fun(row,
                    std::move(get_all).map(
                        r::fun(v,
                            r::object(r::optarg("left", row), r::optarg("right", v))))));
    }
    virtual const char *name() const { return "inner_join"; }
};

class delete_term_t : public rewrite_term_t {
public:
    delete_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(1), rewrite) { }
private:

    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        auto x = pb::dummy_var_t::IGNORED;

        r::reql_t term = r::expr(in->args(0)).replace(r::fun(x, r::null()));

        term.copy_optargs_from_term(*optargs_in);
        return term;
     }
     virtual const char *name() const { return "delete"; }
};

class update_term_t : public rewrite_term_t {
public:
    update_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        auto old_row = pb::dummy_var_t::UPDATE_OLDROW;
        auto new_row = pb::dummy_var_t::UPDATE_NEWROW;

        r::reql_t term =
            r::expr(in->args(0)).replace(
                r::fun(old_row,
                    r::branch(
                        r::null() == old_row,
                        r::null(),
                        r::fun(new_row,
                            r::branch(
                                r::null() == new_row,
                                old_row,
                                r::expr(old_row).merge(new_row)))(
                                    r::expr(in->args(1))(old_row,
                                        r::optarg("_EVAL_FLAGS_", LITERAL_OK)),
                                    r::optarg("_EVAL_FLAGS_", LITERAL_OK)))));

        term.copy_optargs_from_term(*optargs_in);
        return term;
    }
    virtual const char *name() const { return "update"; }
};

class skip_term_t : public rewrite_term_t {
public:
    skip_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        r::reql_t term =
            r::expr(in->args(0)).slice(in->args(1), -1,
                r::optarg("right_bound", "closed"));

        term.copy_optargs_from_term(*optargs_in);
        return term;
     }
     virtual const char *name() const { return "skip"; }
};

class difference_term_t : public rewrite_term_t {
public:
    difference_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {
        auto row = pb::dummy_var_t::DIFFERENCE_ROW;

        r::reql_t term =
            r::expr(in->args(0)).filter(
                r::fun(row,
                    !r::expr(in->args(1)).contains(row)));

        term.copy_optargs_from_term(*optargs_in);
        return term;
    }

     virtual const char *name() const { return "difference"; }
};

class with_fields_term_t : public rewrite_term_t {
public:
    with_fields_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(1, -1), rewrite) { }
private:
    static r::reql_t rewrite(protob_t<const Term> in,
                             UNUSED const pb_rcheckable_t *bt_src,
                             protob_t<const Term> optargs_in) {

        r::reql_t has_fields = r::expr(in->args(0)).has_fields();
        has_fields.copy_args_from_term(*in, 1);
        has_fields.copy_optargs_from_term(*optargs_in);
        r::reql_t pluck = std::move(has_fields).pluck();
        pluck.copy_args_from_term(*in, 1);

        return pluck;
    }
     virtual const char *name() const { return "with_fields"; }
};

counted_t<term_t> make_skip_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<skip_term_t>(env, term);
}
counted_t<term_t> make_groupby_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<groupby_term_t>(env, term);
}
counted_t<term_t> make_inner_join_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<inner_join_term_t>(env, term);
}
counted_t<term_t> make_outer_join_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<outer_join_term_t>(env, term);
}
counted_t<term_t> make_eq_join_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<eq_join_term_t>(env, term);
}
counted_t<term_t> make_update_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<update_term_t>(env, term);
}
counted_t<term_t> make_delete_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<delete_term_t>(env, term);
}
counted_t<term_t> make_difference_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<difference_term_t>(env, term);
}
counted_t<term_t> make_with_fields_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<with_fields_term_t>(env, term);
}

} // namespace ql
