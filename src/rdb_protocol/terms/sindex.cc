// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/minidriver.hpp"

namespace ql {

// We need to use inheritance rather than composition for
// `env_t::special_var_shadower_t` because it needs to be initialized before
// `op_term_t`.
class sindex_create_term_t : public op_term_t {
public:
    sindex_create_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2, 3), optargspec_t({"multi"})) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        counted_t<const datum_t> name_datum = args->arg(env, 1)->as_datum();
        std::string name = name_datum->as_str().to_std();
        rcheck(name != table->get_pkey(),
               base_exc_t::GENERIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         name.c_str()));

        /* Check if we're doing a multi index or a normal index. */
        counted_t<val_t> multi_val = args->optarg(env, "multi");
        sindex_multi_bool_t multi =
            (multi_val && multi_val->as_datum()->as_bool()
             ? sindex_multi_bool_t::MULTI
             : sindex_multi_bool_t::SINGLE);

        counted_t<func_t> index_func;
        if (args->num_args() == 3) {
            counted_t<val_t> v = args->arg(env, 2);
            if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
                counted_t<const datum_t> d = v->as_datum();
                if (d->get_type() == datum_t::R_BINARY) {
                    const wire_string_t &s = d->as_binary();
                    std::vector<char> data;
                    for (size_t i = 0; i < s.size(); ++i) {
                        data.push_back(s.data()[i]);
                    }
                    map_wire_func_t mapping;
                    sindex_reql_version_info_t reql_version;
                    deserialize_sindex_info(data, &mapping, &reql_version, &multi);
                    index_func = mapping.compile_wire_func();
                    // We ignore the `reql_version`, but in the future we may
                    // have to do some conversions for compatibility.
                }
            }
            // We do it this way so that if someone passes a string, we produce
            // a type error asking for a function rather than BINARY.
            if (!index_func.has()) {
                index_func = v->as_func();
            }
        } else {

            pb::dummy_var_t x = pb::dummy_var_t::SINDEXCREATE_X;
            protob_t<Term> func_term
                = r::fun(x, r::var(x)[name_datum]).release_counted();

            prop_bt(func_term.get());
            compile_env_t empty_compile_env((var_visibility_t()));
            counted_t<func_term_t> func_term_term = make_counted<func_term_t>(
                &empty_compile_env, func_term);

            index_func = func_term_term->eval_to_func(env->scope);
        }
        r_sanity_check(index_func.has());

        bool success = table->sindex_create(env->env, name, index_func, multi);
        if (success) {
            datum_object_builder_t res;
            UNUSED bool b = res.add("created", make_counted<datum_t>(1.0));
            return new_val(std::move(res).to_counted());
        } else {
            rfail(base_exc_t::GENERIC, "Index `%s` already exists on table `%s`.",
                  name.c_str(), table->display_name().c_str());
        }
    }

    virtual const char *name() const { return "sindex_create"; }
};

class sindex_drop_term_t : public op_term_t {
public:
    sindex_drop_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        std::string name = args->arg(env, 1)->as_datum()->as_str().to_std();
        bool success = table->sindex_drop(env->env, name);
        if (success) {
            datum_object_builder_t res;
            UNUSED bool b = res.add("dropped", make_counted<datum_t>(1.0));
            return new_val(std::move(res).to_counted());
        } else {
            rfail(base_exc_t::GENERIC, "Index `%s` does not exist on table `%s`.",
                  name.c_str(), table->display_name().c_str());
        }
    }

    virtual const char *name() const { return "sindex_drop"; }
};

class sindex_list_term_t : public op_term_t {
public:
    sindex_list_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();

        return new_val(table->sindex_list(env->env));
    }

    virtual const char *name() const { return "sindex_list"; }
};

class sindex_status_term_t : public op_term_t {
public:
    sindex_status_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        std::set<std::string> sindexes;
        for (size_t i = 1; i < args->num_args(); ++i) {
            sindexes.insert(args->arg(env, i)->as_str().to_std());
        }
        return new_val(table->sindex_status(env->env, sindexes));
    }

    virtual const char *name() const { return "sindex_status"; }
};

/* We wait for no more than 10 seconds between polls to the indexes. */
int64_t initial_poll_ms = 50;
int64_t max_poll_ms = 10000;

bool all_ready(counted_t<const datum_t> statuses) {
    for (size_t i = 0; i < statuses->size(); ++i) {
        if (!statuses->get(i)->get("ready", NOTHROW)->as_bool()) {
            return false;
        }
    }
    return true;
}

class sindex_wait_term_t : public op_term_t {
public:
    sindex_wait_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        std::set<std::string> sindexes;
        for (size_t i = 1; i < args->num_args(); ++i) {
            sindexes.insert(args->arg(env, i)->as_str().to_std());
        }
        // Start with initial_poll_ms, then double the waiting period after each
        // attempt up to a maximum of max_poll_ms.
        int64_t current_poll_ms = initial_poll_ms;
        for (;;) {
            counted_t<const datum_t> statuses =
                table->sindex_status(env->env, sindexes);
            if (all_ready(statuses)) {
                return new_val(statuses);
            } else {
                nap(current_poll_ms, env->env->interruptor);
                current_poll_ms = std::min(max_poll_ms, current_poll_ms * 2);
            }
        }
    }

    virtual const char *name() const { return "sindex_wait"; }
};

class sindex_rename_term_t : public op_term_t {
public:
    sindex_rename_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(3, 3), optargspec_t({"overwrite"})) { }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        counted_t<val_t> old_name_val = args->arg(env, 1);
        counted_t<val_t> new_name_val = args->arg(env, 2);
        std::string old_name = old_name_val->as_str().to_std();
        std::string new_name = new_name_val->as_str().to_std();
        rcheck(old_name != table->get_pkey(),
               base_exc_t::GENERIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         old_name.c_str()));
        rcheck(new_name != table->get_pkey(),
               base_exc_t::GENERIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         new_name.c_str()));

        counted_t<val_t> overwrite_val = args->optarg(env, "overwrite");
        bool overwrite = overwrite_val ? overwrite_val->as_bool() : false;

        sindex_rename_result_t result = table->sindex_rename(env->env, old_name,
                                                             new_name, overwrite);

        switch (result) {
        case sindex_rename_result_t::SUCCESS: {
                datum_object_builder_t retval;
                UNUSED bool b = retval.add("renamed",
                                           make_counted<datum_t>(old_name == new_name ?
                                                                 0.0 : 1.0));
                return new_val(std::move(retval).to_counted());
            }
        case sindex_rename_result_t::OLD_NAME_DOESNT_EXIST:
            rfail_target(old_name_val, base_exc_t::GENERIC,
                         "Index `%s` does not exist on table `%s`.",
                         old_name.c_str(), table->display_name().c_str());
        case sindex_rename_result_t::NEW_NAME_EXISTS:
            rfail_target(new_name_val, base_exc_t::GENERIC,
                         "Index `%s` already exists on table `%s`.",
                         new_name.c_str(), table->display_name().c_str());
        default:
            unreachable();
        }
    }

    virtual const char *name() const { return "sindex_rename"; }
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
counted_t<term_t> make_sindex_status_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sindex_status_term_t>(env, term);
}
counted_t<term_t> make_sindex_wait_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sindex_wait_term_t>(env, term);
}
counted_t<term_t> make_sindex_rename_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sindex_rename_term_t>(env, term);
}


} // namespace ql

