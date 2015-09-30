// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class keys_term_t : public op_term_t {
public:
    keys_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        datum_t d = v->as_datum();
        switch (env->env->reql_version()) {
        case reql_version_t::v1_14: // v1_15 is the same as v1_14
            break;
        case reql_version_t::v1_16:
        case reql_version_t::v2_0:
        case reql_version_t::v2_1_is_latest:
            rcheck_target(v,
                          d.has() && d.get_type() == datum_t::R_OBJECT && !d.is_ptype(),
                          base_exc_t::LOGIC,
                          strprintf("Cannot call `%s` on objects of type `%s`.",
                                    name(),
                                    d.get_type_name().c_str()));
            break;
        default:
            unreachable();
        }

        std::vector<datum_t> arr;
        arr.reserve(d.obj_size());
        for (size_t i = 0; i < d.obj_size(); ++i) {
            arr.push_back(datum_t(d.get_pair(i).first));
        }

        return new_val(datum_t(std::move(arr), env->env->limits()));
    }
    virtual const char *name() const { return "keys"; }
};

class values_term_t : public op_term_t {
public:
    values_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        datum_t d = v->as_datum();
        rcheck_target(v,
                      d.has() && d.get_type() == datum_t::R_OBJECT && !d.is_ptype(),
                      base_exc_t::LOGIC,
                      strprintf("Cannot call `%s` on objects of type `%s`.",
                                name(),
                                d.get_type_name().c_str()));
        std::vector<datum_t> arr;
        arr.reserve(d.obj_size());
        for (size_t i = 0; i < d.obj_size(); ++i) {
            arr.push_back(datum_t(d.get_pair(i).second));
        }

        return new_val(datum_t(std::move(arr), env->env->limits()));
    }
    virtual const char *name() const { return "values"; }
};

class object_term_t : public op_term_t {
public:
    object_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        rcheck(args->num_args() % 2 == 0,
               base_exc_t::LOGIC,
               strprintf("OBJECT expects an even number of arguments (but found %zu).",
                         args->num_args()));
        datum_object_builder_t obj;
        for (size_t i = 0; i < args->num_args(); i+=2) {
            const datum_string_t &key = args->arg(env, i)->as_str();
            datum_t keyval = args->arg(env, i + 1)->as_datum();
            bool b = obj.add(key, keyval);
            rcheck(!b, base_exc_t::LOGIC,
                   strprintf("Duplicate key `%s` in object.  "
                             "(got `%s` and `%s` as values)",
                             key.to_std().c_str(),
                             obj.at(key).trunc_print().c_str(),
                             keyval.trunc_print().c_str()));
        }
        return new_val(std::move(obj).to_datum());
    }

    virtual const char *name() const { return "object"; }
};

counted_t<term_t> make_keys_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<keys_term_t>(env, term);
}

counted_t<term_t> make_values_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<values_term_t>(env, term);
}

counted_t<term_t> make_object_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<object_term_t>(env, term);
}


} // namespace ql
