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
        counted_t<const datum_t> d = args->arg(env, 0)->as_datum();
        const std::map<std::string, counted_t<const datum_t> > &obj = d->as_object();

        std::vector<counted_t<const datum_t> > arr;
        arr.reserve(obj.size());
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            arr.push_back(make_counted<const datum_t>(std::string(it->first)));
        }

        return new_val(make_counted<const datum_t>(std::move(arr)));
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
        datum_ptr_t obj(datum_t::R_OBJECT);
        for (size_t i = 0; i < args->num_args(); i+=2) {
            std::string key = args->arg(env, i)->as_str().to_std();
            counted_t<const datum_t> keyval = args->arg(env, i+1)->as_datum();
            bool b = obj.add(key, keyval);
            rcheck(!b, base_exc_t::GENERIC,
                   strprintf("Duplicate key `%s` in object.  "
                             "(got `%s` and `%s` as values)",
                             key.c_str(),
                             obj->get(key)->trunc_print().c_str(),
                             keyval->trunc_print().c_str()));
        }
        return new_val(obj.to_counted());
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
