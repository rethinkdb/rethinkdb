// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

// This file implements terms that are rewritten into other terms.

class rewrite_term_t : public term_t {
public:
    rewrite_term_t(compile_env_t *env, const protob_t<const Term> term,
                   argspec_t argspec,
                   r::reql_t (*rewrite)(protob_t<const Term> in,
                                        protob_t<const Term> optargs_in))
            : term_t(term), in(term), out(make_counted_term()) {
        int args_size = in->args_size();
        rcheck(argspec.contains(args_size),
               base_exc_t::GENERIC,
               strprintf("Expected %s but found %d.",
                         argspec.print().c_str(), args_size));
        out->Swap(&rewrite(in, in).get());
        propagate_backtrace(out.get(), backtrace());

        real = compile_term(env, out);
    }

private:
    virtual void accumulate_captures(var_captures_t *captures) const {
        return real->accumulate_captures(captures);
    }
    virtual bool is_deterministic() const {
        return real->is_deterministic();
    }

    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env, eval_flags_t) const {
        return real->eval(env);
    }

    protob_t<const Term> in;
    protob_t<Term> out;

    counted_t<const term_t> real;
};

class inner_join_term_t : public rewrite_term_t {
public:
    inner_join_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static r::reql_t rewrite(protob_t<const Term> in,
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
    outer_join_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }

    static r::reql_t rewrite(protob_t<const Term> in,
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
    eq_join_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(3), rewrite) { }
private:

    static r::reql_t rewrite(protob_t<const Term> in,
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
        return r::expr(left).concat_map(
            r::fun(row,
                   r::branch(
                       r::null() == row,
                       r::array(),
                       std::move(get_all).default_(r::array()).map(
                           r::fun(v, r::object(r::optarg("left", row),
                                               r::optarg("right", v)))))));

    }
    virtual const char *name() const { return "eq_join"; }
};

class delete_term_t : public rewrite_term_t {
public:
    delete_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : rewrite_term_t(env, term, argspec_t(1), rewrite) { }
private:

    static r::reql_t rewrite(protob_t<const Term> in,
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

counted_t<term_t> make_skip_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<skip_term_t>(env, term);
}
counted_t<term_t> make_inner_join_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<inner_join_term_t>(env, term);
}
counted_t<term_t> make_outer_join_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<outer_join_term_t>(env, term);
}
counted_t<term_t> make_eq_join_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<eq_join_term_t>(env, term);
}
counted_t<term_t> make_update_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<update_term_t>(env, term);
}
counted_t<term_t> make_delete_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<delete_term_t>(env, term);
}
counted_t<term_t> make_difference_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<difference_term_t>(env, term);
}
counted_t<term_t> make_with_fields_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<with_fields_term_t>(env, term);
}

} // namespace ql
