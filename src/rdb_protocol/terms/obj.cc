// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

class keys_term_t : public op_term_t {
public:
    keys_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_t d = args->arg(env, 0)->as_datum();

        std::vector<datum_t> arr;
        arr.reserve(d.obj_size());
        for (size_t i = 0; i < d.obj_size(); ++i) {
            arr.push_back(datum_t(d.get_pair(i).first));
        }

        return new_val(datum_t(std::move(arr), env->env->limits()));
    }
    virtual const char *name() const { return "keys"; }
};

class object_term_t : public op_term_t {
public:
    object_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        rcheck(args->num_args() % 2 == 0,
               base_exc_t::GENERIC,
               strprintf("OBJECT expects an even number of arguments (but found %zu).",
                         args->num_args()));
        datum_object_builder_t obj;
        for (size_t i = 0; i < args->num_args(); i+=2) {
            const datum_string_t &key = args->arg(env, i)->as_str();
            datum_t keyval = args->arg(env, i + 1)->as_datum();
            bool b = obj.add(key, keyval);
            rcheck(!b, base_exc_t::GENERIC,
                   strprintf("Duplicate key `%s` in object.  "
                             "(got `%s` and `%s` as values)",
                             key.to_std().c_str(),
                             obj.at(key)->trunc_print().c_str(),
                             keyval->trunc_print().c_str()));
        }
        return new_val(std::move(obj).to_datum());
    }

    virtual const char *name() const { return "object"; }
};

counted_t<term_t> make_keys_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<keys_term_t>(env, term);
}

counted_t<term_t> make_object_term(compile_env_t *env, const protob_t<const Term> &term){
    return make_counted<object_term_t>(env, term);
}


} // namespace ql
