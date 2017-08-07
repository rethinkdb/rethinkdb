// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/write_hook.hpp"

#include <string>

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/archive/string_stream.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/real_table.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/terms/terms.hpp"

std::string format_write_hook_query(const write_hook_config_t &config) {
    std::string ret = "setWriteHook(";
    ret += config.func.compile_wire_func()->print_js_function();
    ret += ")";
    return ret;
}

namespace ql {

class set_write_hook_term_t : public op_term_t {
public:
    set_write_hook_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }

    deterministic_t is_deterministic() const final {
        return deterministic_t::no();
    }

    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();

        bool existed = false;
        admin_err_t error;
        datum_t write_hook;
        if (env->env->reql_cluster_interface()->get_write_hook(
                env->env->get_user_context(),
                table->db,
                name_string_t::guarantee_valid(table->name.c_str()),
                env->env->interruptor,
                &write_hook,
                &error)) {
            if (write_hook.has() &&
                write_hook.get_type() != datum_t::type_t::R_NULL) {
                existed = true;
            }
        }
        /* Parse the write_hook configuration */
        optional<write_hook_config_t> config;
        datum_string_t message("deleted");
        scoped_ptr_t<val_t> v = args->arg(env, 1);
        // RSI: Old reql versions hanging around, being unused, is pretty bad.
        // RSI: Something about write hooks not being specified vs. being specified as "null" in certain API's was weird.

        // We ignore the write_hook's old `reql_version` and make the new version
        // just be `reql_version_t::LATEST`; but in the future we may have
        // to do some conversions for compatibility.
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            datum_t d = v->as_datum();
            if (d.get_type() == datum_t::R_BINARY) {
                ql::wire_func_t func;

                datum_string_t str = d.as_binary();
                size_t sz = str.size();
                size_t prefix_sz = strlen(write_hook_blob_prefix);
                const char *data = str.data();
                bool bad_prefix = (sz < prefix_sz);
                for (size_t i = 0; !bad_prefix && i < prefix_sz; ++i) {
                    bad_prefix |= (data[i] != write_hook_blob_prefix[i]);
                }
                rcheck(!bad_prefix,
                       base_exc_t::LOGIC,
                       "Cannot create a write hook except from a reql_write_hook_function"
                       " returned from `get_write_hook`.");

                string_read_stream_t rs(str.to_std(), prefix_sz);
                deserialize<cluster_version_t::LATEST_DISK>(&rs, &func);

                config.set(write_hook_config_t(func, reql_version_t::LATEST));
                goto config_specified_with_value;
            } else if (d.get_type() == datum_t::R_NULL) {
                goto config_specified_without_value;
            }
        }

        // This way it will complain about it not being a function.
        config.set(write_hook_config_t(ql::wire_func_t(v->as_func()),
                                       reql_version_t::LATEST));

    config_specified_with_value:

        config->func.compile_wire_func()->assert_deterministic(
                constant_now_t::no,
                "Write hook functions must be deterministic.");

        {
            optional<size_t> arity = config->func.compile_wire_func()->arity();

            rcheck(static_cast<bool>(arity) && arity.get() == 3,
                   base_exc_t::LOGIC,
                   strprintf("Write hook functions must expect 3 arguments."));
        }

        message =
            existed ?
            datum_string_t("replaced") :
            datum_string_t("created");

    config_specified_without_value:

        try {
            admin_err_t err;
            if (!env->env->reql_cluster_interface()->set_write_hook(
                    env->env->get_user_context(),
                    table->db,
                    name_string_t::guarantee_valid(table->name.c_str()),
                    config,
                    env->env->interruptor,
                    &err)) {
                REQL_RETHROW(err);
            }
        } catch (auth::permission_error_t const &permission_error) {
            rfail(ql::base_exc_t::PERMISSION_ERROR, "%s", permission_error.what());
        }

        ql::datum_object_builder_t res;
        res.overwrite(message, datum_t(1.0));
        return new_val(std::move(res).to_datum());
    }

    virtual const char *name() const { return "set_write_hook"; }
};

class get_write_hook_term_t : public op_term_t {
public:
    get_write_hook_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }

    deterministic_t is_deterministic() const final {
        return deterministic_t::no();
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();

        datum_t write_hook;
        try {
            admin_err_t error;
            if (!env->env->reql_cluster_interface()->get_write_hook(
                    env->env->get_user_context(),
                    table->db,
                    name_string_t::guarantee_valid(table->name.c_str()),
                    env->env->interruptor,
                    &write_hook,
                    &error)) {
                REQL_RETHROW(error);
            }
        } catch (auth::permission_error_t const &permission_error) {
            rfail(ql::base_exc_t::PERMISSION_ERROR, "%s", permission_error.what());
        }
        return new_val(std::move(write_hook));
    }

    virtual const char *name() const { return "get_write_hook"; }
};

counted_t<term_t> make_set_write_hook_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<set_write_hook_term_t>(env, term);
}
counted_t<term_t> make_get_write_hook_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<get_write_hook_term_t>(env, term);
}

} // namespace ql
