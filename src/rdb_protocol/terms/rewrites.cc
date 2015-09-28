// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

// This file implements terms that are rewritten into other terms.

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(compile_env_t *env, const raw_term_t &term,
                   argspec_t argspec,
                   minidriver_t::reql_t (*rewrite)(const raw_term_t &))
            : term_t(term), rewrite_src(do_rewrite(term, argspec, rewrite)) {
        real = compile_term(env, rewrite_src);
    }

private:
    raw_term_t do_rewrite(const raw_term_t &term, 
                          argspec_t argspec,
                          minidriver_t::reql_t (*rewrite)(const raw_term_t &)) {
        rcheck(argspec.contains(term.num_args()),
               base_exc_t::LOGIC,
               strprintf("Expected %s but found %zu.",
                         argspec.print().c_str(), term.num_args()));
        return rewrite(term).root_term();
    }

    virtual void accumulate_captures(var_captures_t *captures) const {
        return real->accumulate_captures(captures);
    }
    virtual bool is_deterministic() const {
        return real->is_deterministic();
    }

    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env, eval_flags_t) const {
        return real->eval(env);
    }

    raw_term_t rewrite_src;
    counted_t<const term_t> real;
};

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());

        raw_term_t left = in.arg(0);
        raw_term_t right = in.arg(1);
        raw_term_t func = in.arg(2);
        auto n = minidriver_t::dummy_var_t::INNERJOIN_N;
        auto m = minidriver_t::dummy_var_t::INNERJOIN_M;

        minidriver_t::reql_t term =
            r.expr(left).concat_map(
                r.fun(n,
                    r.expr(right).concat_map(
                        r.fun(m,
                            r.branch(
                                r.expr(func)(r.var(n), r.var(m)),
                                r.array(r.object(r.optarg("left", n),
                                                 r.optarg("right", m))),
                                r.array())))));

        term.copy_optargs_from_term(in);
        return term;
    }

    virtual const char *name() const { return "inner_join"; }
};

class outer_join_term_t : public rewrite_term_t {
public:
    outer_join_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());

        raw_term_t left = in.arg(0);
        raw_term_t right = in.arg(1);
        raw_term_t func = in.arg(2);
        auto n = minidriver_t::dummy_var_t::OUTERJOIN_N;
        auto m = minidriver_t::dummy_var_t::OUTERJOIN_M;
        auto lst = minidriver_t::dummy_var_t::OUTERJOIN_LST;

        minidriver_t::reql_t inner_concat_map =
            r.expr(right).concat_map(
                r.fun(m,
                    r.branch(
                        r.expr(func)(r.var(n), r.var(m)),
                        r.array(r.object(r.optarg("left", n),
                                         r.optarg("right", m))),
                        r.array())));

        minidriver_t::reql_t term =
            r.expr(left).concat_map(
                r.fun(n,
                    inner_concat_map.coerce_to("ARRAY").do_(lst,
                        r.branch(r.var(lst).count() > 0,
                                 r.var(lst),
                                 r.array(r.object(r.optarg("left", n)))))));

        term.copy_optargs_from_term(in);
        return term;
    }

    virtual const char *name() const { return "outer_join"; }
};

class eq_join_term_t : public rewrite_term_t {
public:
    eq_join_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }
private:

    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());

        raw_term_t left = in.arg(0);
        raw_term_t left_attr = in.arg(1);
        raw_term_t right = in.arg(2);
        auto row = minidriver_t::dummy_var_t::EQJOIN_ROW;
        auto v = minidriver_t::dummy_var_t::EQJOIN_V;

        minidriver_t::reql_t get_all =
            r.expr(right).get_all(
                r.expr(left_attr)(r.var(row),
                                  r.optarg("_SHORTCUT_", GET_FIELD_SHORTCUT)));
        get_all.copy_optargs_from_term(in);
        return r.expr(left).concat_map(
            r.fun(row,
                  r.branch(
                       r.null() == r.var(row),
                       r.array(),
                       get_all.default_(r.array()).map(
                           r.fun(v, r.object(r.optarg("left", row),
                                             r.optarg("right", v)))))));

    }
    virtual const char *name() const { return "eq_join"; }
};

class delete_term_t : public rewrite_term_t {
public:
    delete_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(1), rewrite) { }
private:

    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());

        raw_term_t val = in.arg(0);
        auto x = minidriver_t::dummy_var_t::IGNORED;

        minidriver_t::reql_t term = r.expr(val).replace(r.fun(x, r.null()));
        term.copy_optargs_from_term(in);
        return term;
     }
     virtual const char *name() const { return "delete"; }
};

class update_term_t : public rewrite_term_t {
public:
    update_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());
        raw_term_t arg0 = in.arg(0);
        raw_term_t arg1 = in.arg(1);
        auto old_row = minidriver_t::dummy_var_t::UPDATE_OLDROW;
        auto new_row = minidriver_t::dummy_var_t::UPDATE_NEWROW;

        minidriver_t::reql_t term =
            r.expr(arg0).replace(
                r.fun(old_row,
                    r.branch(
                        r.null() == r.var(old_row),
                        r.null(),
                        r.fun(new_row,
                            r.branch(
                                r.null() == r.var(new_row),
                                old_row,
                                r.var(old_row).merge(r.var(new_row))))(
                                    r.expr(arg1)(r.var(old_row),
                                        r.optarg("_EVAL_FLAGS_", LITERAL_OK)),
                                    r.optarg("_EVAL_FLAGS_", LITERAL_OK)))));

        term.copy_optargs_from_term(in);
        return term;
    }
    virtual const char *name() const { return "update"; }
};

class skip_term_t : public rewrite_term_t {
public:
    skip_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());
        raw_term_t arg0 = in.arg(0);
        raw_term_t arg1 = in.arg(1);

        minidriver_t::reql_t term =
            r.expr(arg0).slice(arg1, -1, r.optarg("right_bound", "closed"));

        term.copy_optargs_from_term(in);
        return term;
     }
     virtual const char *name() const { return "skip"; }
};

class difference_term_t : public rewrite_term_t {
public:
    difference_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(2), rewrite) { }
private:
    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());
        raw_term_t arg0 = in.arg(0);
        raw_term_t arg1 = in.arg(1);
        auto row = minidriver_t::dummy_var_t::DIFFERENCE_ROW;

        minidriver_t::reql_t term =
            r.expr(arg0).filter(r.fun(row, !(r.expr(arg1).contains(r.var(row)))));

        term.copy_optargs_from_term(in);
        return term;
    }

     virtual const char *name() const { return "difference"; }
};

class with_fields_term_t : public rewrite_term_t {
public:
    with_fields_term_t(compile_env_t *env, const raw_term_t &term)
        : rewrite_term_t(env, term, argspec_t(1, -1), rewrite) { }
private:
    static minidriver_t::reql_t rewrite(const raw_term_t &in) {
        minidriver_t r(in.bt());
        raw_term_t arg0 = in.arg(0);

        minidriver_t::reql_t has_fields = r.expr(arg0).has_fields();
        has_fields.copy_args_from_term(in, 1);
        has_fields.copy_optargs_from_term(in);
        minidriver_t::reql_t pluck = has_fields.pluck();
        pluck.copy_args_from_term(in, 1);
        return pluck;
    }
     virtual const char *name() const { return "with_fields"; }
};

counted_t<term_t> make_skip_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<skip_term_t>(env, term);
}
counted_t<term_t> make_inner_join_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<inner_join_term_t>(env, term);
}
counted_t<term_t> make_outer_join_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<outer_join_term_t>(env, term);
}
counted_t<term_t> make_eq_join_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<eq_join_term_t>(env, term);
}
counted_t<term_t> make_update_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<update_term_t>(env, term);
}
counted_t<term_t> make_delete_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<delete_term_t>(env, term);
}
counted_t<term_t> make_difference_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<difference_term_t>(env, term);
}
counted_t<term_t> make_with_fields_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<with_fields_term_t>(env, term);
}

} // namespace ql
