// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <vector>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// ALL and ANY are written strangely because I originally thought that we could
// have non-boolean values that evaluate to true, but then we decided not to do
// that.

class all_term_t : public op_term_t {
public:
    all_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        for (size_t i = 0; i < num_args(); ++i) {
            counted_t<val_t> v = arg(env, i);
            if (!v->as_bool() || i == num_args() - 1) {
                return v;
            }
        }
        unreachable();
    }
    virtual const char *name() const { return "all"; }
};

class any_term_t : public op_term_t {
public:
    any_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        for (size_t i = 0; i < num_args(); ++i) {
            counted_t<val_t> v = arg(env, i);
            if (v->as_bool()) {
                return v;
            }
        }
        return new_val_bool(false);
    }
    virtual const char *name() const { return "any"; }
};

class branch_term_t : public op_term_t {
public:
    branch_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        bool b = arg(env, 0)->as_bool();
        return b ? arg(env, 1) : arg(env, 2);
    }
    virtual const char *name() const { return "branch"; }
};


class funcall_term_t : public op_term_t {
public:
    funcall_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1),
          optargspec_t({"_SHORTCUT_", "_EVAL_FLAGS_"})) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, eval_flags_t) {
        function_shortcut_t shortcut = CONSTANT_SHORTCUT;
        eval_flags_t flags = NO_FLAGS;
        if (counted_t<val_t> v = optarg(env, "_SHORTCUT_")) {
            shortcut = static_cast<function_shortcut_t>(v->as_num());
        }

        if (counted_t<val_t> v = optarg(env, "_EVAL_FLAGS_")) {
            flags = static_cast<eval_flags_t>(v->as_num());
        }

        /* This switch exists just to make sure that we don't get a bogus value
         * for the shortcut. */
        switch (shortcut) {
            case NO_SHORTCUT: // fallthru
            case CONSTANT_SHORTCUT: // fallthru
            case GET_FIELD_SHORTCUT: // fallthru
            case PLUCK_SHORTCUT: // fallthru
                break;
            default:
                rfail(base_exc_t::GENERIC,
                      "Unrecognized value `%d` for _SHORTCUT_ argument.", shortcut);
        }
        counted_t<func_t> f = arg(env, 0, flags)->as_func(shortcut);

        // We need specialized logic for `grouped_data` here because `funcall`
        // needs to be polymorphic on its second argument rather than its first.
        // (We might have wanted to do this anyway, though, because otherwise
        // we'd be compiling shortcut functions `n` times.)
        if (num_args() == 1) {
            return f->call(env->env, flags);
        } else {
            counted_t<val_t> arg1 = arg(env, 1, flags);
            std::vector<counted_t<const datum_t> > args(1);
            args.reserve(num_args() - 1);
            for (size_t i = 2; i < num_args(); ++i) {
                args.push_back(arg(env, i, flags)->as_datum());
            }
            r_sanity_check(!args[0].has());
            counted_t<grouped_data_t> gd
                = arg1->maybe_as_promiscuous_grouped_data(env->env);
            if (gd.has()) {
                counted_t<grouped_data_t> out(new grouped_data_t());
                for (auto kv = gd->begin(); kv != gd->end(); ++kv) {
                    args[0] = kv->second;
                    (*out)[kv->first] = f->call(env->env, args, flags)->as_datum();
                }
                return make_counted<val_t>(out, backtrace());
            } else {
                args[0] = arg1->as_datum();
                return f->call(env->env, args, flags);
            }
        }
    }
    virtual const char *name() const { return "funcall"; }
};


counted_t<term_t> make_all_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<all_term_t>(env, term);
}
counted_t<term_t> make_any_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<any_term_t>(env, term);
}
counted_t<term_t> make_branch_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<branch_term_t>(env, term);
}
counted_t<term_t> make_funcall_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<funcall_term_t>(env, term);
}

}  // namespace ql
