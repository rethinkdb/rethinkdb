// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <map>
#include <string>

#include "containers/name_string.hpp"
#include "rdb_protocol/datum_string.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"
#include "rdb_protocol/terms/writes.hpp"

namespace ql {

name_string_t get_name(const scoped_ptr_t<val_t> &val, const char *type_str) {
    r_sanity_check(val.has());
    const datum_string_t &raw_name = val->as_str();
    name_string_t name;
    bool assignment_successful = name.assign_value(raw_name);
    rcheck_target(val.get(),
                  assignment_successful,
                  base_exc_t::GENERIC,
                  strprintf("%s name `%s` invalid (%s).",
                            type_str,
                            raw_name.to_std().c_str(),
                            name_string_t::valid_char_msg));
    return name;
}

void get_replicas_and_director(const scoped_ptr_t<val_t> &replicas,
                               const scoped_ptr_t<val_t> &director_tag,
                               table_generate_config_params_t *params) {
    if (replicas.has()) {
        params->num_replicas.clear();
        datum_t datum = replicas->as_datum();
        if (datum.get_type() == datum_t::R_OBJECT) {
            rcheck_target(replicas.get(), director_tag.has(), base_exc_t::GENERIC,
                "`director_tag` must be specified when `replicas` is an OBJECT.");
            for (size_t i = 0; i < datum.obj_size(); ++i) {
                std::pair<datum_string_t, datum_t> pair = datum.get_pair(i);
                name_string_t name;
                bool assignment_successful = name.assign_value(pair.first);
                rcheck_target(replicas, assignment_successful, base_exc_t::GENERIC,
                    strprintf("Server tag name `%s` invalid (%s).",
                              pair.first.to_std().c_str(),
                              name_string_t::valid_char_msg));
                int64_t count = checked_convert_to_int(replicas.get(),
                                                       pair.second.as_num());
                rcheck_target(replicas.get(), count >= 0,
                    base_exc_t::GENERIC, "Can't have a negative number of replicas");
                size_t size_count = static_cast<size_t>(count);
                rcheck_target(replicas.get(), static_cast<int64_t>(size_count) == count,
                              base_exc_t::GENERIC,
                              strprintf("Integer too large: %" PRIi64, count));
                params->num_replicas.insert(std::make_pair(name, size_count));
            }
        } else if (datum.get_type() == datum_t::R_NUM) {
            rcheck_target(replicas.get(), !director_tag.has(), base_exc_t::GENERIC,
                "`replicas` must be an OBJECT if `director_tag` is specified.");
            size_t count = replicas->as_int<size_t>();
            params->num_replicas.insert(std::make_pair(params->director_tag, count));
        } else {
            rfail_target(replicas, base_exc_t::GENERIC,
                "Expected type OBJECT or NUMBER but found %s:\n%s",
                datum.get_type_name().c_str(), datum.print().c_str());
        }
    }

    if (director_tag.has()) {
        params->director_tag = get_name(director_tag, "Server tag");
    }
}

// Meta operations (BUT NOT TABLE TERMS) should inherit from this.
class meta_op_term_t : public op_term_t {
public:
    meta_op_term_t(compile_env_t *env, protob_t<const Term> term, argspec_t argspec,
              optargspec_t optargspec = optargspec_t({}))
        : op_term_t(env, std::move(term), std::move(argspec), std::move(optargspec)) { }

private:
    virtual bool is_deterministic() const { return false; }
};

class meta_write_op_t : public meta_op_term_t {
public:
    meta_write_op_t(compile_env_t *env, protob_t<const Term> term, argspec_t argspec,
                    optargspec_t optargspec = optargspec_t({}))
        : meta_op_term_t(env, std::move(term), std::move(argspec), std::move(optargspec)) { }

private:
    virtual std::string write_eval_impl(scope_env_t *env,
                                        args_t *args,
                                        eval_flags_t flags) const = 0;
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t flags) const {
        std::string op = write_eval_impl(env, args, flags);
        datum_object_builder_t res;
        UNUSED bool b = res.add(datum_string_t(op), datum_t(1.0));
        return new_val(std::move(res).to_datum());
    }
};

class db_term_t : public meta_op_term_t {
public:
    db_term_t(compile_env_t *env, const protob_t<const Term> &term) : meta_op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        name_string_t db_name = get_name(args->arg(env, 0), "Database");
        counted_t<const db_t> db;
        std::string error;
        if (!env->env->reql_cluster_interface()->db_find(db_name, env->env->interruptor,
                &db, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }
        return new_val(db);
    }
    virtual const char *name() const { return "db"; }
};

class db_create_term_t : public meta_write_op_t {
public:
    db_create_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1)) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        name_string_t db_name = get_name(args->arg(env, 0), "Database");
        std::string error;
        if (!env->env->reql_cluster_interface()->db_create(db_name,
                env->env->interruptor, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return "created";
    }
    virtual const char *name() const { return "db_create"; }
};

class table_create_term_t : public meta_write_op_t {
public:
    table_create_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1, 2),
            optargspec_t({"primary_key", "shards", "replicas", "director_tag"})) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        /* Parse arguments */
        table_generate_config_params_t config_params =
            table_generate_config_params_t::make_default();

        // Parse the 'shards' optarg
        if (scoped_ptr_t<val_t> shards_optarg = args->optarg(env, "shards")) {
            rcheck_target(shards_optarg, shards_optarg->as_int() > 0, base_exc_t::GENERIC,
                          "Every table must have at least one shard.");
            config_params.num_shards = shards_optarg->as_int();
        }

        // Parse the 'replicas' and 'director_tag' optargs
        get_replicas_and_director(args->optarg(env, "replicas"),
                                  args->optarg(env, "director_tag"),
                                  &config_params);

        std::string primary_key = "id";
        if (scoped_ptr_t<val_t> v = args->optarg(env, "primary_key")) {
            primary_key = v->as_str().to_std();
        }

        counted_t<const db_t> db;
        name_string_t tbl_name;
        if (args->num_args() == 1) {
            scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
            r_sanity_check(dbv);
            db = dbv->as_db();
            tbl_name = get_name(args->arg(env, 0), "Table");
        } else {
            db = args->arg(env, 0)->as_db();
            tbl_name = get_name(args->arg(env, 1), "Table");
        }

        /* Create the table */
        std::string error;
        if (!env->env->reql_cluster_interface()->table_create(tbl_name, db,
                config_params, primary_key,
                env->env->interruptor, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return "created";
    }
    virtual const char *name() const { return "table_create"; }
};

class db_drop_term_t : public meta_write_op_t {
public:
    db_drop_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1)) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        name_string_t db_name = get_name(args->arg(env, 0), "Database");

        std::string error;
        if (!env->env->reql_cluster_interface()->db_drop(db_name,
                env->env->interruptor, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return "dropped";
    }
    virtual const char *name() const { return "db_drop"; }
};

class table_drop_term_t : public meta_write_op_t {
public:
    table_drop_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1, 2)) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const db_t> db;
        name_string_t tbl_name;
        if (args->num_args() == 1) {
            scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
            r_sanity_check(dbv);
            db = dbv->as_db();
            tbl_name = get_name(args->arg(env, 0), "Table");
        } else {
            db = args->arg(env, 0)->as_db();
            tbl_name = get_name(args->arg(env, 1), "Table");
        }

        std::string error;
        if (!env->env->reql_cluster_interface()->table_drop(tbl_name, db,
                env->env->interruptor, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return "dropped";
    }
    virtual const char *name() const { return "table_drop"; }
};

class db_list_term_t : public meta_op_term_t {
public:
    db_list_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(0)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *, eval_flags_t) const {
        std::set<name_string_t> dbs;
        std::string error;
        if (!env->env->reql_cluster_interface()->db_list(
                env->env->interruptor, &dbs, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        std::vector<datum_t> arr;
        arr.reserve(dbs.size());
        for (auto it = dbs.begin(); it != dbs.end(); ++it) {
            arr.push_back(datum_t(datum_string_t(it->str())));
        }

        return new_val(datum_t(std::move(arr), env->env->limits()));
    }
    virtual const char *name() const { return "db_list"; }
};

class table_list_term_t : public meta_op_term_t {
public:
    table_list_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(0, 1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const ql::db_t> db;
        if (args->num_args() == 0) {
            scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
            r_sanity_check(dbv);
            db = dbv->as_db();
        } else {
            db = args->arg(env, 0)->as_db();
        }

        std::set<name_string_t> tables;
        std::string error;
        if (!env->env->reql_cluster_interface()->table_list(db,
                env->env->interruptor, &tables, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        std::vector<datum_t> arr;
        arr.reserve(tables.size());
        for (auto it = tables.begin(); it != tables.end(); ++it) {
            arr.push_back(datum_t(datum_string_t(it->str())));
        }
        return new_val(datum_t(std::move(arr), env->env->limits()));
    }
    virtual const char *name() const { return "table_list"; }
};

class db_config_term_t : public meta_op_term_t {
public:
    db_config_term_t(compile_env_t *env,
                     const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(0, -1), optargspec_t({})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::vector<name_string_t> db_names;
        if (args->num_args() > 0) {
            for (size_t i = 0; i < args->num_args(); ++i) {
                scoped_ptr_t<val_t> arg = args->arg(env, i);
                db_names.push_back(get_name(arg, "Database"));
            }
        }

        std::string error;
        scoped_ptr_t<val_t> resp;
        if (!env->env->reql_cluster_interface()->db_config(
                db_names, backtrace(), env->env->interruptor, &resp, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }
        return resp;
    }
    virtual const char *name() const { return "db_config"; }
};

class table_meta_read_term_t : public meta_op_term_t {
public:
    table_meta_read_term_t(compile_env_t *env,
                           const protob_t<const Term> &term,
                           const optargspec_t &optargs) :
        meta_op_term_t(env, term, argspec_t(0, -1), optargs) { }
protected:
    virtual bool impl(scope_env_t *env,
                      args_t *args,
                      counted_t<const db_t> db,
                      const std::vector<name_string_t> &tables,
                      scoped_ptr_t<val_t> *resp_out,
                      std::string *error_out) const = 0;
private:
    counted_t<const db_t> get_db_optarg(scope_env_t *env, args_t *args) const {
        scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
        r_sanity_check(dbv);
        return dbv->as_db();
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const db_t> db;
        std::vector<name_string_t> tables;

        if (args->num_args() > 0) {
            for (size_t i = 0; i < args->num_args(); ++i) {
                scoped_ptr_t<val_t> arg = args->arg(env, i);
                if (i == 0 && arg->get_type().is_convertible(val_t::type_t::DB)) {
                    db = arg->as_db();
                } else {
                    tables.push_back(get_name(arg, "Table"));
                }
            }
        }

        if (!db.has()) {
            db = get_db_optarg(env, args);
        }

        std::string error;
        scoped_ptr_t<val_t> resp;
        if (!impl(env, args, db, tables, &resp, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }
        return resp;
    }
};

class table_config_term_t : public table_meta_read_term_t {
public:
    table_config_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        table_meta_read_term_t(env, term, optargspec_t({})) { }
private:
    bool impl(scope_env_t *env,
              UNUSED args_t *args,
              counted_t<const db_t> db,
              const std::vector<name_string_t> &tables,
              scoped_ptr_t<val_t> *resp_out,
              std::string *error_out) const {
        return env->env->reql_cluster_interface()->table_config(
            db, tables, backtrace(), env->env->interruptor, resp_out, error_out);
    }
    virtual const char *name() const { return "table_config"; }
};

class table_status_term_t : public table_meta_read_term_t {
public:
    table_status_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        table_meta_read_term_t(env, term, optargspec_t({})) { }
private:
    bool impl(scope_env_t *env,
              UNUSED args_t *args,
              counted_t<const db_t> db,
              const std::vector<name_string_t> &tables,
              scoped_ptr_t<val_t> *resp_out,
              std::string *error_out) const {
        return env->env->reql_cluster_interface()->table_status(
            db, tables, backtrace(), env->env->interruptor, resp_out, error_out);
    }
    virtual const char *name() const { return "table_status"; }
};

class table_wait_term_t : public table_meta_read_term_t {
public:
    table_wait_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        table_meta_read_term_t(env, term, optargspec_t({})) { }
private:
    bool impl(scope_env_t *env,
              UNUSED args_t *args,
              counted_t<const db_t> db,
              const std::vector<name_string_t> &tables,
              scoped_ptr_t<val_t> *resp_out,
              std::string *error_out) const {
        return env->env->reql_cluster_interface()->table_wait(
            db, tables, table_readiness_t::finished, backtrace(),
            env->env->interruptor, resp_out, error_out);
    }
    virtual const char *name() const { return "table_wait"; }
};

class reconfigure_term_t : public meta_op_term_t {
public:
    reconfigure_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(0, 1),
            optargspec_t({"director_tag", "dry_run", "replicas", "shards"})) { }
private:
    scoped_ptr_t<val_t> required_optarg(scope_env_t *env,
                                        args_t *args,
                                        const char *name) const {
        scoped_ptr_t<val_t> result = args->optarg(env, name);
        rcheck(result.has(), base_exc_t::GENERIC,
               strprintf("Missing required argument `%s`.", name));
        return result;
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t) const {
        // Use the default director_tag, unless the optarg overwrites it
        table_generate_config_params_t config_params =
            table_generate_config_params_t::make_default();

        // Parse the 'shards' optarg
        scoped_ptr_t<val_t> shards_optarg = required_optarg(env, args, "shards");
        rcheck_target(shards_optarg, shards_optarg->as_int() > 0, base_exc_t::GENERIC,
                      "Every table must have at least one shard.");
        config_params.num_shards = shards_optarg->as_int();

        // Parse the 'replicas' and 'director_tag' optargs
        get_replicas_and_director(required_optarg(env, args, "replicas"),
                                  args->optarg(env, "director_tag"),
                                  &config_params);

        // Parse the 'dry_run' optarg
        bool dry_run = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "dry_run")) {
            dry_run = v->as_bool();
        }

        bool success;
        datum_t result;
        std::string error;
        scoped_ptr_t<val_t> target;
        if (args->num_args() == 0) {
            target = args->optarg(env, "db");
            r_sanity_check(target.has());
        } else {
            target = args->arg(env, 0);
        }

        /* Perform the operation */
        if (target->get_type().is_convertible(val_t::type_t::DB)) {
            success = env->env->reql_cluster_interface()->db_reconfigure(
                    target->as_db(), config_params, dry_run, env->env->interruptor,
                    &result, &error);
        } else {
            counted_t<table_t> table = target->as_table();
            name_string_t name = name_string_t::guarantee_valid(table->name.c_str());
            /* RSI(reql_admin): Make sure the user didn't call `.between()` or `.order_by()`
            on this table */
            success = env->env->reql_cluster_interface()->table_reconfigure(
                    table->db, name, config_params, dry_run,
                    env->env->interruptor, &result, &error);
        }

        if (!success) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return new_val(result);
    }
    virtual const char *name() const { return "reconfigure"; }
};

class rebalance_term_t : public meta_op_term_t {
public:
    rebalance_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(0, 1), optargspec_t({})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t) const {
        scoped_ptr_t<val_t> target;
        if (args->num_args() == 0) {
            target = args->optarg(env, "db");
            r_sanity_check(target.has());
        } else {
            target = args->arg(env, 0);
        }

        /* Perform the operation */
        std::string error;
        bool success;
        datum_t result;
        if (target->get_type().is_convertible(val_t::type_t::DB)) {
            success = env->env->reql_cluster_interface()->db_rebalance(
                    target->as_db(), env->env->interruptor, &result, &error);
        } else {
            counted_t<table_t> table = target->as_table();
            name_string_t name = name_string_t::guarantee_valid(table->name.c_str());
            /* RSI(reql_admin): Make sure the user didn't call `.between()` or `.order_by()`
            on this table */
            success = env->env->reql_cluster_interface()->table_rebalance(
                    table->db, name, env->env->interruptor, &result, &error);
        }

        if (!success) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return new_val(result);
    }
    virtual const char *name() const { return "rebalance"; }
};

class sync_term_t : public meta_write_op_t {
public:
    sync_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : meta_write_op_t(env, term, argspec_t(1)) { }

private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> t = args->arg(env, 0)->as_table();
        bool success = t->sync(env->env);
        r_sanity_check(success);
        return "synced";
    }
    virtual const char *name() const { return "sync"; }
};

class table_term_t : public op_term_t {
public:
    table_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, 2),
          optargspec_t({ "use_outdated", "identifier_format" })) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> t = args->optarg(env, "use_outdated");
        bool use_outdated = t ? t->as_bool() : false;

        boost::optional<admin_identifier_format_t> identifier_format;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "identifier_format")) {
            const datum_string_t &str = v->as_str();
            if (str == "name") {
                identifier_format = admin_identifier_format_t::name;
            } else if (str == "uuid") {
                identifier_format = admin_identifier_format_t::uuid;
            } else {
                rfail(base_exc_t::GENERIC, "Identifier format `%s` unrecognized "
                    "(options are \"name\" and \"uuid\").", str.to_std().c_str());
            }
        }

        counted_t<const db_t> db;
        name_string_t name;
        if (args->num_args() == 1) {
            scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
            r_sanity_check(dbv.has());
            db = dbv->as_db();
            name = get_name(args->arg(env, 0), "Table");
        } else {
            r_sanity_check(args->num_args() == 2);
            db = args->arg(env, 0)->as_db();
            name = get_name(args->arg(env, 1), "Table");
        }

        std::string error;
        counted_t<base_table_t> table;
        if (!env->env->reql_cluster_interface()->table_find(name, db,
                identifier_format, env->env->interruptor, &table, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }
        return new_val(make_counted<table_t>(
            std::move(table), db, name.str(), use_outdated, backtrace()));
    }
    virtual bool is_deterministic() const { return false; }
    virtual const char *name() const { return "table"; }
};

class get_term_t : public op_term_t {
public:
    get_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t>
    eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return new_val(single_selection_t::from_key(
                           env->env,
                           backtrace(),
                           args->arg(env, 0)->as_table(),
                           args->arg(env, 1)->as_datum()));
    }
    virtual const char *name() const { return "get"; }
};

class get_all_term_t : public op_term_t {
public:
    get_all_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2, -1), optargspec_t({ "index" })) { }
private:
    datum_t get_key_arg(const scoped_ptr_t<val_t> &arg) const {
        datum_t datum_arg = arg->as_datum();

        rcheck_target(arg,
                      !datum_arg.is_ptype(pseudo::geometry_string),
                      base_exc_t::GENERIC,
                      "Cannot use a geospatial index with `get_all`. "
                      "Use `get_intersecting` instead.");
        return datum_arg;
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        scoped_ptr_t<val_t> index = args->optarg(env, "index");
        std::string index_str = index ? index->as_str().to_std() : "";
        if (index && index_str != table->get_pkey()) {
            std::vector<counted_t<datum_stream_t> > streams;
            for (size_t i = 1; i < args->num_args(); ++i) {
                datum_t key = get_key_arg(args->arg(env, i));
                counted_t<datum_stream_t> seq =
                    table->get_all(env->env, key, index_str, backtrace());
                streams.push_back(seq);
            }
            counted_t<datum_stream_t> stream
                = make_counted<union_datum_stream_t>(std::move(streams), backtrace());
            return new_val(make_counted<selection_t>(table, stream));
        } else {
            datum_array_builder_t arr(env->env->limits());
            for (size_t i = 1; i < args->num_args(); ++i) {
                datum_t key = get_key_arg(args->arg(env, i));
                datum_t row = table->get_row(env->env, key);
                if (row.get_type() != datum_t::R_NULL) {
                    arr.add(row);
                }
            }
            counted_t<datum_stream_t> stream
                = make_counted<array_datum_stream_t>(std::move(arr).to_datum(),
                                                     backtrace());
            return new_val(make_counted<selection_t>(table, stream));
        }
    }
    virtual const char *name() const { return "get_all"; }
};

counted_t<term_t> make_db_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<db_term_t>(env, term);
}

counted_t<term_t> make_table_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_term_t>(env, term);
}

counted_t<term_t> make_get_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_term_t>(env, term);
}

counted_t<term_t> make_get_all_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_all_term_t>(env, term);
}

counted_t<term_t> make_db_create_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<db_create_term_t>(env, term);
}

counted_t<term_t> make_db_drop_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<db_drop_term_t>(env, term);
}

counted_t<term_t> make_db_list_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<db_list_term_t>(env, term);
}

counted_t<term_t> make_table_create_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_create_term_t>(env, term);
}

counted_t<term_t> make_table_drop_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_drop_term_t>(env, term);
}

counted_t<term_t> make_table_list_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_list_term_t>(env, term);
}

counted_t<term_t> make_db_config_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<db_config_term_t>(env, term);
}

counted_t<term_t> make_table_config_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_config_term_t>(env, term);
}

counted_t<term_t> make_table_status_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_status_term_t>(env, term);
}

counted_t<term_t> make_table_wait_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_wait_term_t>(env, term);
}

counted_t<term_t> make_reconfigure_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<reconfigure_term_t>(env, term);
}

counted_t<term_t> make_rebalance_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<rebalance_term_t>(env, term);
}

counted_t<term_t> make_sync_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sync_term_t>(env, term);
}



} // namespace ql
