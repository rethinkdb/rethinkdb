// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/op.hpp"

namespace ql {

class datum_term_t : public term_t {
public:
    explicit datum_term_t(const raw_term_t &term)
            : term_t(term),
              datum(term.datum(configured_limits_t::unlimited, reql_version_t::LATEST)) {
        r_sanity_check(datum.has());
    }

    bool is_simple_selector() const {
        if (datum.get_type() == datum_t::type_t::R_STR) {
            return true;
        }
        return false;
    }

private:
    virtual void accumulate_captures(var_captures_t *) const { /* do nothing */ }
    virtual deterministic_t is_deterministic() const { return deterministic_t::always; }
    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *, eval_flags_t) const {
        return new_val(datum);
    }
    virtual const char *name() const { return "datum"; }
    const datum_t datum;
};

class constant_term_t : public op_term_t {
public:
    constant_term_t(compile_env_t *env, const raw_term_t &term,
                    double constant, const char *name)
        : op_term_t(env, term, argspec_t(0)), _constant(constant), _name(name) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *, args_t *, eval_flags_t) const {
        return new_val(datum_t(_constant));
    }
    virtual const char *name() const { return _name; }
    const double _constant;
    const char *const _name;
};

class make_array_term_t : public op_term_t {
public:
    make_array_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_array_builder_t acc(env->env->limits());
        acc.reserve(args->num_args());
        {
            profile::sampler_t sampler("Evaluating elements in make_array.", env->env->trace);
            for (size_t i = 0; i < args->num_args(); ++i) {
                acc.add(args->arg(env, i)->as_datum());
                sampler.new_sample();
            }
        }
        return new_val(std::move(acc).to_datum());
    }
    virtual const char *name() const { return "make_array"; }

    bool is_simple_selector() const {
        return recursive_is_simple_selector();
    }
};

class make_obj_term_t : public term_t {
public:
    make_obj_term_t(compile_env_t *env, const raw_term_t &term)
        : term_t(term) {
        // An F.Y.I. for driver developers.
        rcheck(term.num_args() == 0,
               base_exc_t::LOGIC,
               "MAKE_OBJ term must not have any args.");

        term.each_optarg([&](const raw_term_t &o, const std::string &name) {
                counted_t<const term_t> t = compile_term(env, o);
                auto res = optargs.insert(std::make_pair(name, std::move(t)));
                rcheck(res.second, base_exc_t::LOGIC,
                       strprintf("Duplicate object key: %s.", name.c_str()));
            });
    }

    scoped_ptr_t<val_t> term_eval(scope_env_t *env, eval_flags_t flags) const {
        bool literal_ok = flags & LITERAL_OK;
        eval_flags_t new_flags = literal_ok ? LITERAL_OK : NO_FLAGS;
        datum_object_builder_t acc;
        {
            profile::sampler_t sampler("Evaluating elements in make_obj.", env->env->trace);
            for (const auto &pair : optargs) {
                bool dup = acc.add(datum_string_t(pair.first),
                                   pair.second->eval(env, new_flags)->as_datum());
                rcheck(!dup, base_exc_t::LOGIC,
                       strprintf("Duplicate object key: %s.",
                                 pair.first.c_str()));
                sampler.new_sample();
            }
        }
        return new_val(std::move(acc).to_datum());
    }

    deterministic_t is_deterministic() const {
        deterministic_t worst_so_far = deterministic_t::always;
        for (const auto &arg : optargs) {
            worst_so_far = worst_determinism(worst_so_far, arg.second->is_deterministic());
        }
        return worst_so_far;
    }

    void accumulate_captures(var_captures_t *captures) const {
        accumulate_all_captures(optargs, captures);
    }

    const char *name() const { return "make_obj"; }

private:
    std::map<std::string, counted_t<const term_t> > optargs;
    DISABLE_COPYING(make_obj_term_t);
};

class binary_term_t : public op_term_t {
public:
    binary_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, 1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> arg = args->arg(env, 0);
        datum_t datum_arg = arg->as_datum();

        if (datum_arg.get_type() == datum_t::type_t::R_BINARY) {
            return arg;
        }

        const datum_string_t &datum_str = datum_arg.as_str();
        return new_val(datum_t::binary(datum_string_t(datum_str)));
    }
    virtual const char *name() const { return "binary"; }
};

counted_t<term_t> make_datum_term(
        const raw_term_t &term) {
    return make_counted<datum_term_t>(term);
}
counted_t<term_t> make_constant_term(
        compile_env_t *env, const raw_term_t &term,
        double constant, const char *name) {
    return make_counted<constant_term_t>(env, term, constant, name);
}
counted_t<term_t> make_make_array_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<make_array_term_t>(env, term);
}
counted_t<term_t> make_make_obj_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<make_obj_term_t>(env, term);
}
counted_t<term_t> make_binary_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<binary_term_t>(env, term);
}

} // namespace ql
