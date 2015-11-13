// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/archive/string_stream.hpp"
#include "rdb_protocol/real_table.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

/* `sindex_config_to_string` produces the string that goes in the `function` field of
`sindex_status()`. `sindex_config_from_string()` parses that string when it's passed to
`sindex_create()`. */

const char *const sindex_blob_prefix = "$reql_index_function$";

datum_string_t sindex_config_to_string(const sindex_config_t &config) {
    sindex_reql_version_info_t version;
    version.original_reql_version = config.func_version;
    version.latest_compatible_reql_version = config.func_version;
    version.latest_checked_reql_version = reql_version_t::LATEST;
    sindex_disk_info_t disk_info(config.func, version, config.multi, config.geo);

    write_message_t wm;
    serialize_sindex_info(&wm, disk_info);
    string_stream_t stream;
    int write_res = send_write_message(&stream, &wm);
    guarantee(write_res == 0);
    return datum_string_t(sindex_blob_prefix + stream.str());
}

sindex_config_t sindex_config_from_string(
        const datum_string_t &string, rcheckable_t *target) {
    sindex_config_t config;
    const char *data = string.data();
    size_t sz = string.size();
    size_t prefix_sz = strlen(sindex_blob_prefix);
    bool bad_prefix = (sz < prefix_sz);
    for (size_t i = 0; !bad_prefix && i < prefix_sz; ++i) {
        bad_prefix |= (data[i] != sindex_blob_prefix[i]);
    }
    rcheck_target(
        target,
        !bad_prefix,
        base_exc_t::LOGIC,
        "Cannot create an sindex except from a reql_index_function returned from "
        "`index_status` in the field `function`.");
    std::vector<char> vec(data + prefix_sz, data + sz);
    sindex_disk_info_t sindex_info;
    try {
        deserialize_sindex_info(vec, &sindex_info,
            [target](obsolete_reql_version_t ver) {
                switch (ver) {
                case obsolete_reql_version_t::v1_13:
                    rfail_target(target, base_exc_t::LOGIC,
                                 "Attempted to import a RethinkDB 1.13 secondary index, "
                                 "which is no longer supported.  This secondary index "
                                 "may be updated by importing into RethinkDB 2.0.");
                    break;
                // v1_15 is equal to v1_14
                case obsolete_reql_version_t::v1_14:
                    rfail_target(target, base_exc_t::LOGIC,
                                 "Attempted to import a secondary index from before "
                                 "RethinkDB 1.16, which is no longer supported.  This "
                                 "secondary index may be updated by importing into "
                                 "RethinkDB 2.1.");
                    break;
                default: unreachable();
                }
            });
    } catch (const archive_exc_t &e) {
        rfail_target(
            target,
            base_exc_t::LOGIC,
            "Binary blob passed to index create could not be interpreted as a "
            "reql_index_function (%s).",
            e.what());
    }
    return sindex_config_t(
        sindex_info.mapping,
        sindex_info.mapping_version_info.original_reql_version,
        sindex_info.multi,
        sindex_info.geo);
}

// Helper for `sindex_status_to_datum()`
std::string format_index_create_query(
        const std::string &name,
        const sindex_config_t &config) {
    // TODO: Theoretically we need to escape quotes and UTF-8 characters inside the name.
    // Maybe use RapidJSON? Does our pretty-printer even do that for strings?
    std::string ret = "indexCreate('" + name + "', ";
    ret += config.func.compile_wire_func()->print_js_function();
    bool first_optarg = true;
    if (config.multi == sindex_multi_bool_t::MULTI) {
        if (first_optarg) {
            ret += ", {";
            first_optarg = false;
        } else {
            ret += ", ";
        }
        ret += "multi: true";
    }
    if (config.geo == sindex_geo_bool_t::GEO) {
        if (first_optarg) {
            ret += ", {";
            first_optarg = false;
        } else {
            ret += ", ";
        }
        ret += "geo: true";
    }
    if (!first_optarg) {
        ret += "}";
    }
    ret += ")";
    return ret;
}

/* `sindex_status_to_datum()` produces the documents that are returned from
`sindex_status()` and `sindex_wait()`. */

ql::datum_t sindex_status_to_datum(
        const std::string &name,
        const sindex_config_t &config,
        const sindex_status_t &status) {
    ql::datum_object_builder_t stat;
    stat.overwrite("index", ql::datum_t(datum_string_t(name)));
    if (!status.ready) {
        stat.overwrite("progress",
            ql::datum_t(status.progress_numerator /
                        std::max<double>(status.progress_denominator, 1.0)));
    }
    stat.overwrite("ready", ql::datum_t::boolean(status.ready));
    stat.overwrite("outdated", ql::datum_t::boolean(status.outdated));
    stat.overwrite("multi",
        ql::datum_t::boolean(config.multi == sindex_multi_bool_t::MULTI));
    stat.overwrite("geo",
        ql::datum_t::boolean(config.geo == sindex_geo_bool_t::GEO));
    stat.overwrite("function",
        ql::datum_t::binary(sindex_config_to_string(config)));
    stat.overwrite("query",
        ql::datum_t(datum_string_t(format_index_create_query(name, config))));
    return std::move(stat).to_datum();
}

class sindex_create_term_t : public op_term_t {
public:
    sindex_create_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2, 3), optargspec_t({"multi", "geo"})) { }

    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        datum_t name_datum = args->arg(env, 1)->as_datum();
        std::string name = name_datum.as_str().to_std();
        rcheck(name != table->get_pkey(),
               base_exc_t::LOGIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         name.c_str()));

        /* Parse the sindex configuration */
        sindex_config_t config;
        config.multi = sindex_multi_bool_t::SINGLE;
        config.geo = sindex_geo_bool_t::REGULAR;
        if (args->num_args() == 3) {
            scoped_ptr_t<val_t> v = args->arg(env, 2);
            bool got_func = false;
            if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
                datum_t d = v->as_datum();
                if (d.get_type() == datum_t::R_BINARY) {
                    config = sindex_config_from_string(d.as_binary(), v.get());
                    // We ignore the sindex's old `reql_version` and make the new version
                    // just be `reql_version_t::LATEST`; but in the future we may have
                    // to do some conversions for compatibility.
                    config.func_version = reql_version_t::LATEST;
                    got_func = true;
                }
            }
            // We do it this way so that if someone passes a string, we produce
            // a type error asking for a function rather than BINARY.
            if (!got_func) {
                config.func = ql::map_wire_func_t(v->as_func());
                config.func_version = reql_version_t::LATEST;
            }
        } else {
            minidriver_t r(backtrace());
            auto x = minidriver_t::dummy_var_t::SINDEXCREATE_X;

            compile_env_t empty_compile_env((var_visibility_t()));
            counted_t<func_term_t> func_term_term =
                make_counted<func_term_t>(&empty_compile_env,
                                          r.fun(x, r.var(x)[name_datum]).root_term());

            config.func = ql::map_wire_func_t(func_term_term->eval_to_func(env->scope));
            config.func_version = reql_version_t::LATEST;
        }

        config.func.compile_wire_func()->assert_deterministic(
            "Index functions must be deterministic.");

        /* Check if we're doing a multi index or a normal index. */
        if (scoped_ptr_t<val_t> multi_val = args->optarg(env, "multi")) {
            config.multi = multi_val->as_bool()
                ? sindex_multi_bool_t::MULTI
                : sindex_multi_bool_t::SINGLE;
        }
        /* Do we want to create a geo index? */
        if (scoped_ptr_t<val_t> geo_val = args->optarg(env, "geo")) {
            config.geo = geo_val->as_bool()
                ? sindex_geo_bool_t::GEO
                : sindex_geo_bool_t::REGULAR;
        }

        admin_err_t error;
        if (!env->env->reql_cluster_interface()->sindex_create(
                table->db, name_string_t::guarantee_valid(table->name.c_str()),
                name, config, env->env->interruptor, &error)) {
            REQL_RETHROW(error);
        }

        ql::datum_object_builder_t res;
        res.overwrite("created", datum_t(1.0));
        return new_val(std::move(res).to_datum());
    }

    virtual const char *name() const { return "sindex_create"; }
};

class sindex_drop_term_t : public op_term_t {
public:
    sindex_drop_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        std::string name = args->arg(env, 1)->as_datum().as_str().to_std();

        admin_err_t error;
        if (!env->env->reql_cluster_interface()->sindex_drop(
                table->db, name_string_t::guarantee_valid(table->name.c_str()),
                name, env->env->interruptor, &error)) {
            REQL_RETHROW(error);
        }

        ql::datum_object_builder_t res;
        res.overwrite("dropped", datum_t(1.0));
        return new_val(std::move(res).to_datum());
    }

    virtual const char *name() const { return "sindex_drop"; }
};

class sindex_list_term_t : public op_term_t {
public:
    sindex_list_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();

        /* Fetch a list of all sindexes and their configs and statuses */
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            configs_and_statuses;
        admin_err_t error;
        if (!env->env->reql_cluster_interface()->sindex_list(
                table->db, name_string_t::guarantee_valid(table->name.c_str()),
                env->env->interruptor, &error, &configs_and_statuses)) {
            REQL_RETHROW(error);
        }

        /* Convert into an array and return it */
        ql::datum_array_builder_t res(ql::configured_limits_t::unlimited);
        for (const auto &pair : configs_and_statuses) {
            res.add(ql::datum_t(datum_string_t(pair.first)));
        }
        return new_val(std::move(res).to_datum());
    }

    virtual const char *name() const { return "sindex_list"; }
};

class sindex_status_term_t : public op_term_t {
public:
    sindex_status_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        /* Parse the arguments */
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        std::set<std::string> sindexes;
        for (size_t i = 1; i < args->num_args(); ++i) {
            sindexes.insert(args->arg(env, i)->as_str().to_std());
        }

        /* Fetch a list of all sindexes and their configs and statuses */
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            configs_and_statuses;
        admin_err_t error;
        if (!env->env->reql_cluster_interface()->sindex_list(
                table->db, name_string_t::guarantee_valid(table->name.c_str()),
                env->env->interruptor, &error, &configs_and_statuses)) {
            REQL_RETHROW(error);
        }

        /* Convert it into an array and return it */
        ql::datum_array_builder_t res(ql::configured_limits_t::unlimited);
        std::set<std::string> remaining_sindexes = sindexes;
        for (const auto &pair : configs_and_statuses) {
            if (!sindexes.empty()) {
                if (sindexes.count(pair.first) == 0) {
                    continue;
                } else {
                    remaining_sindexes.erase(pair.first);
                }
            }
            res.add(sindex_status_to_datum(
                pair.first, pair.second.first, pair.second.second));
        }

        /* Make sure we found all the requested sindexes. */
        rcheck(remaining_sindexes.empty(), base_exc_t::OP_FAILED,
            strprintf("Index `%s` was not found on table `%s`.",
                      remaining_sindexes.begin()->c_str(),
                      table->display_name().c_str()));

        return new_val(std::move(res).to_datum());
    }

    virtual const char *name() const { return "sindex_status"; }
};

/* We wait for no more than 10 seconds between polls to the indexes. */
int64_t initial_poll_ms = 50;
int64_t max_poll_ms = 10000;

class sindex_wait_term_t : public op_term_t {
public:
    sindex_wait_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        std::set<std::string> sindexes;
        for (size_t i = 1; i < args->num_args(); ++i) {
            sindexes.insert(args->arg(env, i)->as_str().to_std());
        }
        // Start with initial_poll_ms, then double the waiting period after each
        // attempt up to a maximum of max_poll_ms.
        int64_t current_poll_ms = initial_poll_ms;
        for (;;) {
            std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
                configs_and_statuses;
            admin_err_t error;
            if (!env->env->reql_cluster_interface()->sindex_list(
                    table->db, name_string_t::guarantee_valid(table->name.c_str()),
                    env->env->interruptor, &error, &configs_and_statuses)) {
                REQL_RETHROW(error);
            }

            // Verify all requested sindexes exist.
            for (const auto &sindex : sindexes) {
                rcheck(configs_and_statuses.count(sindex) == 1, base_exc_t::OP_FAILED,
                    strprintf("Index `%s` was not found on table `%s`.",
                              sindex.c_str(),
                              table->display_name().c_str()));
            }

            ql::datum_array_builder_t statuses(ql::configured_limits_t::unlimited);
            bool all_ready = true;
            for (const auto &pair : configs_and_statuses) {
                if (!sindexes.empty() && sindexes.count(pair.first) == 0) {
                    continue;
                }
                if (pair.second.second.ready) {
                    statuses.add(sindex_status_to_datum(
                        pair.first, pair.second.first, pair.second.second));
                } else {
                    all_ready = false;
                }
            }
            if (all_ready) {
                return new_val(std::move(statuses).to_datum());
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
    sindex_rename_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(3, 3), optargspec_t({"overwrite"})) { }

    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        scoped_ptr_t<val_t> old_name_val = args->arg(env, 1);
        scoped_ptr_t<val_t> new_name_val = args->arg(env, 2);
        std::string old_name = old_name_val->as_str().to_std();
        std::string new_name = new_name_val->as_str().to_std();
        rcheck(old_name != table->get_pkey(),
               base_exc_t::LOGIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         old_name.c_str()));
        rcheck(new_name != table->get_pkey(),
               base_exc_t::LOGIC,
               strprintf("Index name conflict: `%s` is the name of the primary key.",
                         new_name.c_str()));

        scoped_ptr_t<val_t> overwrite_val = args->optarg(env, "overwrite");
        bool overwrite = overwrite_val ? overwrite_val->as_bool() : false;

        admin_err_t error;
        if (!env->env->reql_cluster_interface()->sindex_rename(
                table->db, name_string_t::guarantee_valid(table->name.c_str()),
                old_name, new_name, overwrite, env->env->interruptor, &error)) {
            REQL_RETHROW(error);
        }

        datum_object_builder_t retval;
        UNUSED bool b = retval.add("renamed",
                                   datum_t(old_name == new_name ?
                                                         0.0 : 1.0));
        return new_val(std::move(retval).to_datum());
    }

    virtual const char *name() const { return "sindex_rename"; }
};

counted_t<term_t> make_sindex_create_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sindex_create_term_t>(env, term);
}
counted_t<term_t> make_sindex_drop_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sindex_drop_term_t>(env, term);
}
counted_t<term_t> make_sindex_list_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sindex_list_term_t>(env, term);
}
counted_t<term_t> make_sindex_status_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sindex_status_term_t>(env, term);
}
counted_t<term_t> make_sindex_wait_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sindex_wait_term_t>(env, term);
}
counted_t<term_t> make_sindex_rename_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sindex_rename_term_t>(env, term);
}


} // namespace ql

