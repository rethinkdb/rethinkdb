// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

// We need to use inheritance rather than composition for
// `env_t::special_var_shadower_t` because it needs to be initialized before
// `op_term_t`.
class sindex_create_term_t : public op_term_t {
public:
    sindex_create_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2, 3), optargspec_t({"multi"})) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> table = arg(env, 0)->as_table();
        counted_t<const datum_t> name_datum = arg(env, 1)->as_datum();
        std::string name = name_datum->as_str();
        rcheck(name != table->get_pkey(),
               base_exc_t::GENERIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         name.c_str()));
        counted_t<func_t> index_func;
        if (num_args() == 3) {
            index_func = arg(env, 2)->as_func();
        } else {
            protob_t<Term> func_term = make_counted_term();
            {
                sym_t x;
                Term *arg = pb::set_func(func_term.get(), pb::dummy_var_t::SINDEXCREATE_X, &x);
                N2(GET_FIELD, NVAR(x), NDATUM(name_datum));
            }
            prop_bt(func_term.get());
            compile_env_t empty_compile_env((var_visibility_t()));
            counted_t<func_term_t> func_term_term = make_counted<func_term_t>(&empty_compile_env,
                                                                              func_term);

            index_func = func_term_term->eval_to_func(env->scope);
        }
        r_sanity_check(index_func.has());

        /* Check if we're doing a multi index or a normal index. */
        counted_t<val_t> multi_val = optarg(env, "multi");
        sindex_multi_bool_t multi =
            (multi_val && multi_val->as_datum()->as_bool() ?  MULTI : SINGLE);

        bool success = table->sindex_create(env->env, name, index_func, multi);
        if (success) {
            datum_ptr_t res(datum_t::R_OBJECT);
            UNUSED bool b = res.add("created", make_counted<datum_t>(1.0));
            return new_val(res.to_counted());
        } else {
            rfail(base_exc_t::GENERIC, "Index `%s` already exists.", name.c_str());
        }
    }

    virtual const char *name() const { return "sindex_create"; }
};

class sindex_drop_term_t : public op_term_t {
public:
    sindex_drop_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> table = arg(env, 0)->as_table();
        std::string name = arg(env, 1)->as_datum()->as_str();
        bool success = table->sindex_drop(env->env, name);
        if (success) {
            datum_ptr_t res(datum_t::R_OBJECT);
            UNUSED bool b = res.add("dropped", make_counted<datum_t>(1.0));
            return new_val(res.to_counted());
        } else {
            rfail(base_exc_t::GENERIC, "Index `%s` does not exist.", name.c_str());
        }
    }

    virtual const char *name() const { return "sindex_drop"; }
};

class sindex_list_term_t : public op_term_t {
public:
    sindex_list_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> table = arg(env, 0)->as_table();

        return new_val(table->sindex_list(env->env));
    }

    virtual const char *name() const { return "sindex_list"; }
};

counted_t<term_t> make_sindex_create_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sindex_create_term_t>(env, term);
}
counted_t<term_t> make_sindex_drop_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sindex_drop_term_t>(env, term);
}
counted_t<term_t> make_sindex_list_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sindex_list_term_t>(env, term);
}


} // namespace ql

