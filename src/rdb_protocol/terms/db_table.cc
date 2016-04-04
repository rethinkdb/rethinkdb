// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <map>
#include <string>

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/name_string.hpp"
#include "rdb_protocol/datum_string.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"
#include "rdb_protocol/terms/writes.hpp"

namespace ql {

name_string_t get_name(bt_rcheckable_t *target, const datum_string_t &raw_name,
                       const char *type_str) {
    name_string_t name;
    bool assignment_successful = name.assign_value(raw_name);
    rcheck_target(target,
                  assignment_successful,
                  base_exc_t::LOGIC,
                  strprintf("%s name `%s` invalid (%s).",
                            type_str,
                            raw_name.to_std().c_str(),
                            name_string_t::valid_char_msg));
    return name;
}

name_string_t get_name(const scoped_ptr_t<val_t> &name, const char *type_str) {
    r_sanity_check(name.has());
    return get_name(name.get(), name->as_str(), type_str);
}

void get_replicas_and_primary(const scoped_ptr_t<val_t> &replicas,
                              const scoped_ptr_t<val_t> &nonvoting_replica_tags,
                              const scoped_ptr_t<val_t> &primary_replica_tag,
                              table_generate_config_params_t *params) {
    if (replicas.has()) {
        params->num_replicas.clear();
        datum_t datum = replicas->as_datum();
        if (datum.get_type() == datum_t::R_OBJECT) {
            rcheck_target(replicas.get(), primary_replica_tag.has(), base_exc_t::LOGIC,
                "`primary_replica_tag` must be specified when `replicas` is an OBJECT.");
            for (size_t i = 0; i < datum.obj_size(); ++i) {
                std::pair<datum_string_t, datum_t> pair = datum.get_pair(i);
                name_string_t name = get_name(replicas.get(), pair.first, "Server tag");
                int64_t count = checked_convert_to_int(replicas.get(),
                                                       pair.second.as_num());
                rcheck_target(replicas.get(), count >= 0,
                    base_exc_t::LOGIC, "Can't have a negative number of replicas");
                size_t size_count = static_cast<size_t>(count);
                rcheck_target(replicas.get(), static_cast<int64_t>(size_count) == count,
                              base_exc_t::LOGIC,
                              strprintf("Integer too large: %" PRIi64, count));
                params->num_replicas.insert(std::make_pair(name, size_count));
            }
        } else if (datum.get_type() == datum_t::R_NUM) {
            rcheck_target(
                replicas.get(), !primary_replica_tag.has(), base_exc_t::LOGIC,
                "`replicas` must be an OBJECT if `primary_replica_tag` is specified.");
            rcheck_target(
                replicas.get(), !nonvoting_replica_tags.has(), base_exc_t::LOGIC,
                "`replicas` must be an OBJECT if `nonvoting_replica_tags` is "
                "specified.");
            size_t count = replicas->as_int<size_t>();
            params->num_replicas.insert(
                std::make_pair(params->primary_replica_tag, count));
        } else {
            rfail_target(replicas, base_exc_t::LOGIC,
                "Expected type OBJECT or NUMBER but found %s:\n%s",
                datum.get_type_name().c_str(), datum.print().c_str());
        }
    }

    if (nonvoting_replica_tags.has()) {
        params->nonvoting_replica_tags.clear();
        datum_t datum = nonvoting_replica_tags->as_datum();
        rcheck_target(nonvoting_replica_tags.get(), datum.get_type() == datum_t::R_ARRAY,
            base_exc_t::LOGIC, strprintf("Expected type ARRAY but found %s:\n%s",
            datum.get_type_name().c_str(), datum.print().c_str()));
        for (size_t i = 0; i < datum.arr_size(); ++i) {
            datum_t tag = datum.get(i);
            rcheck_target(
                nonvoting_replica_tags.get(), tag.get_type() == datum_t::R_STR,
                base_exc_t::LOGIC, strprintf("Expected type STRING but found %s:\n%s",
                tag.get_type_name().c_str(), tag.print().c_str()));
            params->nonvoting_replica_tags.insert(get_name(
                nonvoting_replica_tags.get(), tag.as_str(), "Server tag"));
        }
    }

    if (primary_replica_tag.has()) {
        params->primary_replica_tag = get_name(
            primary_replica_tag.get(), primary_replica_tag->as_str(), "Server tag");
    }
}

  // Meta operations (BUT NOT TABLE TERMS) should inherit from this.
class meta_op_term_t : public op_term_t {
public:
    meta_op_term_t(compile_env_t *env, const raw_term_t &term,
                   argspec_t argspec, optargspec_t optargspec = optargspec_t({}))
        : op_term_t(env, term, std::move(argspec), std::move(optargspec)) { }

private:
    virtual deterministic_t is_deterministic() const { return deterministic_t::no; }
};

class db_term_t : public meta_op_term_t {
public:
    db_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        name_string_t db_name = get_name(args->arg(env, 0), "Database");
        counted_t<const db_t> db;
        admin_err_t error;
        if (!env->env->reql_cluster_interface()->db_find(
                db_name, env->env->interruptor, &db, &error)) {
            REQL_RETHROW(error);
        }
        return new_val(db);
    }
    virtual const char *name() const { return "db"; }
};

class db_create_term_t : public meta_op_term_t {
public:
    db_create_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        name_string_t db_name = get_name(args->arg(env, 0), "Database");
        admin_err_t error;
        ql::datum_t result;
        if (!env->env->reql_cluster_interface()->db_create(db_name,
                env->env->interruptor, &result, &error)) {
            REQL_RETHROW(error);
        }
        return new_val(result);
    }
    virtual const char *name() const { return "db_create"; }
};

class table_create_term_t : public meta_op_term_t {
public:
    table_create_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1, 2),
            optargspec_t({"primary_key", "shards", "replicas",
                          "nonvoting_replica_tags", "primary_replica_tag",
                          "durability"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        /* Parse arguments */
        table_generate_config_params_t config_params =
            table_generate_config_params_t::make_default();

        // Parse the 'shards' optarg
        if (scoped_ptr_t<val_t> shards_optarg = args->optarg(env, "shards")) {
            rcheck_target(shards_optarg, shards_optarg->as_int() > 0, base_exc_t::LOGIC,
                          "Every table must have at least one shard.");
            config_params.num_shards = shards_optarg->as_int();
        }

        // Parse the 'replicas', 'nonvoting_replica_tags', and
        // 'primary_replica_tag' optargs
        get_replicas_and_primary(args->optarg(env, "replicas"),
                                 args->optarg(env, "nonvoting_replica_tags"),
                                 args->optarg(env, "primary_replica_tag"),
                                 &config_params);

        std::string primary_key = "id";
        if (scoped_ptr_t<val_t> v = args->optarg(env, "primary_key")) {
            primary_key = v->as_str().to_std();
        }

        write_durability_t durability =
            parse_durability_optarg(args->optarg(env, "durability")) ==
                DURABILITY_REQUIREMENT_SOFT ?
                    write_durability_t::SOFT : write_durability_t::HARD;

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
        admin_err_t error;
        ql::datum_t result;
        if (!env->env->reql_cluster_interface()->table_create(tbl_name, db,
                config_params, primary_key, durability,
                env->env->interruptor, &result, &error)) {
            REQL_RETHROW(error);
        }
        return new_val(result);
    }
    virtual const char *name() const { return "table_create"; }
};

class db_drop_term_t : public meta_op_term_t {
public:
    db_drop_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        name_string_t db_name = get_name(args->arg(env, 0), "Database");

        admin_err_t error;
        ql::datum_t result;
        if (!env->env->reql_cluster_interface()->db_drop(db_name,
                env->env->interruptor, &result, &error)) {
            REQL_RETHROW(error);
        }

        return new_val(result);
    }
    virtual const char *name() const { return "db_drop"; }
};

class table_drop_term_t : public meta_op_term_t {
public:
    table_drop_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1, 2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
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

        admin_err_t error;
        ql::datum_t result;
        if (!env->env->reql_cluster_interface()->table_drop(tbl_name, db,
                env->env->interruptor, &result, &error)) {
            REQL_RETHROW(error);
        }

        return new_val(result);
    }
    virtual const char *name() const { return "table_drop"; }
};

class db_list_term_t : public meta_op_term_t {
public:
    db_list_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(0)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *, eval_flags_t) const {
        std::set<name_string_t> dbs;
        admin_err_t error;
        if (!env->env->reql_cluster_interface()->db_list(
                env->env->interruptor, &dbs, &error)) {
            REQL_RETHROW(error);
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
    table_list_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(0, 1)) { }
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
        admin_err_t error;
        if (!env->env->reql_cluster_interface()->table_list(db,
                env->env->interruptor, &tables, &error)) {
            REQL_RETHROW(error);
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

class config_term_t : public meta_op_term_t {
public:
    config_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1, 1), optargspec_t({})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> target = args->arg(env, 0);
        scoped_ptr_t<val_t> selection;
        bool success;
        admin_err_t error;
        /* Note that we always require an argument; we never take a default `db`
        argument. So `r.config()` is an error rather than the configuration for the
        current database. This is why we don't subclass from `table_or_db_meta_term_t`.
        */
        if (target->get_type().is_convertible(val_t::type_t::DB)) {
            success = env->env->reql_cluster_interface()->db_config(
                    target->as_db(), backtrace(), env->env, &selection, &error);
        } else {
            counted_t<table_t> table = target->as_table();
            name_string_t name = name_string_t::guarantee_valid(table->name.c_str());
            success = env->env->reql_cluster_interface()->table_config(
                    table->db, name, backtrace(), env->env, &selection, &error);
        }
        if (!success) {
            REQL_RETHROW(error);
        }
        return selection;
    }
    virtual const char *name() const { return "config"; }
};

class status_term_t : public meta_op_term_t {
public:
    status_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1, 1), optargspec_t({})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        name_string_t name = name_string_t::guarantee_valid(table->name.c_str());
        admin_err_t error;
        scoped_ptr_t<val_t> selection;
        if (!env->env->reql_cluster_interface()->table_status(
                table->db, name, backtrace(), env->env, &selection, &error)) {
            REQL_RETHROW(error);
        }
        return selection;
    }
    virtual const char *name() const { return "status"; }
};

/* Common superclass for terms that can operate on either a table or a database: `wait`,
`reconfigure`, and `rebalance`. */
class table_or_db_meta_term_t : public meta_op_term_t {
public:
    table_or_db_meta_term_t(compile_env_t *env, const raw_term_t &term,
                            optargspec_t &&optargs)
        /* None of the subclasses take positional arguments except for the table/db. */
        : meta_op_term_t(env, term, argspec_t(0, 1), std::move(optargs)) { }
protected:
    /* If the term is called on a table, then `db` and `name_if_table` indicate the
    table's database and name. If the term is called on a database, then `db `indicates
    the database and `name_if_table` will be empty. */
    virtual scoped_ptr_t<val_t> eval_impl_on_table_or_db(
            scope_env_t *env, args_t *args, eval_flags_t flags,
            const counted_t<const ql::db_t> &db,
            const boost::optional<name_string_t> &name_if_table) const = 0;
private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t flags) const {
        scoped_ptr_t<val_t> target;
        if (args->num_args() == 0) {
            target = args->optarg(env, "db");
            r_sanity_check(target.has());
        } else {
            target = args->arg(env, 0);
        }
        if (target->get_type().is_convertible(val_t::type_t::DB)) {
            return eval_impl_on_table_or_db(env, args, flags, target->as_db(),
                boost::none);
        } else {
            counted_t<table_t> table = target->as_table();
            name_string_t name = name_string_t::guarantee_valid(table->name.c_str());
            return eval_impl_on_table_or_db(env, args, flags, table->db, name);
        }
    }
};

class wait_term_t : public table_or_db_meta_term_t {
public:
    wait_term_t(compile_env_t *env, const raw_term_t &term)
        : table_or_db_meta_term_t(env, term,
                                  optargspec_t({"timeout", "wait_for"})) { }
private:
    static char const * const wait_outdated_str;
    static char const * const wait_reads_str;
    static char const * const wait_writes_str;
    static char const * const wait_all_str;

    virtual scoped_ptr_t<val_t> eval_impl_on_table_or_db(
            scope_env_t *env, args_t *args, eval_flags_t,
	    const counted_t<const ql::db_t> &db,
            const boost::optional<name_string_t> &name_if_table) const {
      // Don't allow a wait call without explicit database
      if (args->num_args() == 0) {
	rfail(base_exc_t::LOGIC, "`wait` can only be called on a table or database.");
      }

      // Handle 'wait_for' optarg
        table_readiness_t readiness = table_readiness_t::finished;
        if (scoped_ptr_t<val_t> wait_for = args->optarg(env, "wait_for")) {
            if (wait_for->as_str() == wait_outdated_str) {
                readiness = table_readiness_t::outdated_reads;
            } else if (wait_for->as_str() == wait_reads_str) {
                readiness = table_readiness_t::reads;
            } else if (wait_for->as_str() == wait_writes_str) {
                readiness = table_readiness_t::writes;
            } else if (wait_for->as_str() == wait_all_str) {
                readiness = table_readiness_t::finished;
            } else {
                rfail_target(wait_for, base_exc_t::LOGIC,
                             "Unknown table readiness state: '%s', must be one of "
                             "'%s', '%s', '%s', or '%s'",
                             wait_for->as_str().to_std().c_str(),
                             wait_outdated_str, wait_reads_str,
                             wait_writes_str, wait_all_str);
            }
        }

        // Handle 'timeout' optarg
        signal_timer_t timeout_timer;
        wait_any_t combined_interruptor(env->env->interruptor);
        if (scoped_ptr_t<val_t> timeout = args->optarg(env, "timeout")) {
            timeout_timer.start(timeout->as_int<uint64_t>() * 1000);
            combined_interruptor.add(&timeout_timer);
        }

        // Perform db or table wait
        ql::datum_t result;
        bool success;
        admin_err_t error;
        try {
            if (static_cast<bool>(name_if_table)) {
                success = env->env->reql_cluster_interface()->table_wait(db,
                    *name_if_table, readiness, &combined_interruptor, &result, &error);
            } else {
                success = env->env->reql_cluster_interface()->db_wait(
                    db, readiness, &combined_interruptor, &result, &error);
            }
        } catch (const interrupted_exc_t &ex) {
            if (!timeout_timer.is_pulsed()) {
                throw;
            }
            rfail(base_exc_t::OP_FAILED, "Timed out while waiting for tables.");
        }

        if (!success) {
            REQL_RETHROW(error);
        }
        return new_val(result);
    }
    virtual const char *name() const { return "wait"; }
};

char const * const wait_term_t::wait_outdated_str = "ready_for_outdated_reads";
char const * const wait_term_t::wait_reads_str = "ready_for_reads";
char const * const wait_term_t::wait_writes_str = "ready_for_writes";
char const * const wait_term_t::wait_all_str = "all_replicas_ready";

class reconfigure_term_t : public table_or_db_meta_term_t {
public:
    reconfigure_term_t(compile_env_t *env, const raw_term_t &term)
        : table_or_db_meta_term_t(env, term,
            optargspec_t({"dry_run", "emergency_repair", "nonvoting_replica_tags",
                "primary_replica_tag", "replicas", "shards"})) { }
private:
    scoped_ptr_t<val_t> required_optarg(scope_env_t *env,
                                        args_t *args,
                                        const char *name) const {
        scoped_ptr_t<val_t> result = args->optarg(env, name);
        rcheck(result.has(), base_exc_t::LOGIC,
               strprintf("Missing required argument `%s`.", name));
        return result;
    }

    virtual scoped_ptr_t<val_t> eval_impl_on_table_or_db(
            scope_env_t *env, args_t *args, eval_flags_t,
            const counted_t<const ql::db_t> &db,
            const boost::optional<name_string_t> &name_if_table) const {
        // Don't allow a reconfigure call without explicit database
        if (args->num_args() == 0) {
	  rfail(base_exc_t::LOGIC, "`reconfigure` can only be called on a table or database.");
        }

        // Parse the 'dry_run' optarg
        bool dry_run = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "dry_run")) {
            dry_run = v->as_bool();
        }

        /* Figure out whether we're doing a regular reconfiguration or an emergency
        repair. */
        scoped_ptr_t<val_t> emergency_repair = args->optarg(env, "emergency_repair");
        if (!emergency_repair.has() ||
                emergency_repair->as_datum() == ql::datum_t::null()) {
            /* We're doing a regular reconfiguration. */

            // Use the default primary_replica_tag, unless the optarg overwrites it
            table_generate_config_params_t config_params =
                table_generate_config_params_t::make_default();

            // Parse the 'shards' optarg
            scoped_ptr_t<val_t> shards_optarg = required_optarg(env, args, "shards");
            rcheck_target(shards_optarg, shards_optarg->as_int() > 0,
                          base_exc_t::LOGIC,
                          "Every table must have at least one shard.");
            config_params.num_shards = shards_optarg->as_int();

            // Parse the 'replicas', 'nonvoting_replica_tags', and
            // 'primary_replica_tag' optargs
            get_replicas_and_primary(required_optarg(env, args, "replicas"),
                                     args->optarg(env, "nonvoting_replica_tags"),
                                     args->optarg(env, "primary_replica_tag"),
                                     &config_params);

            bool success;
            datum_t result;
            admin_err_t error;
            /* Perform the operation */
            if (static_cast<bool>(name_if_table)) {
                success = env->env->reql_cluster_interface()->table_reconfigure(
                        db, *name_if_table, config_params, dry_run,
                        env->env->interruptor, &result, &error);
            } else {
                success = env->env->reql_cluster_interface()->db_reconfigure(
                        db, config_params, dry_run, env->env->interruptor,
                        &result, &error);
            }
            if (!success) {
                REQL_RETHROW(error);
            }
            return new_val(result);

        } else {
            /* We're doing an emergency repair */

            /* Parse `emergency_repair` to figure out which kind we're doing. */
            datum_string_t emergency_repair_str = emergency_repair->as_str();
            emergency_repair_mode_t mode;
            if (emergency_repair_str == "_debug_recommit") {
                mode = emergency_repair_mode_t::DEBUG_RECOMMIT;
            } else if (emergency_repair_str == "unsafe_rollback") {
                mode = emergency_repair_mode_t::UNSAFE_ROLLBACK;
            } else if (emergency_repair_str == "unsafe_rollback_or_erase") {
                mode = emergency_repair_mode_t::UNSAFE_ROLLBACK_OR_ERASE;
            } else {
                rfail_target(emergency_repair.get(), base_exc_t::LOGIC,
                    "`emergency_repair` should be \"unsafe_rollback\" or "
                    "\"unsafe_rollback_or_erase\"");
            }

            /* Make sure none of the optargs that are used with regular reconfigurations
            are present, to avoid user confusion. */
            if (args->optarg(env, "nonvoting_replica_tags").has() ||
                    args->optarg(env, "primary_replica_tag").has() ||
                    args->optarg(env, "replicas").has() ||
                    args->optarg(env, "shards").has()) {
                rfail(base_exc_t::LOGIC, "In emergency repair mode, you can't "
                    "specify shards, replicas, etc.");
            }

            if (!static_cast<bool>(name_if_table)) {
                rfail(base_exc_t::LOGIC, "Can't emergency repair an entire database "
                    "at once; instead you should run `reconfigure()` on each table "
                    "individually.");
            }

            datum_t result;
            admin_err_t error;
            bool success = env->env->reql_cluster_interface()->table_emergency_repair(
                db, *name_if_table, mode, dry_run,
                env->env->interruptor, &result, &error);
            if (!success) {
                REQL_RETHROW(error);
            }
            return new_val(result);
        }
    }
    virtual const char *name() const { return "reconfigure"; }
};

class rebalance_term_t : public table_or_db_meta_term_t {
public:
    rebalance_term_t(compile_env_t *env, const raw_term_t &term)
        : table_or_db_meta_term_t(env, term, optargspec_t({})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl_on_table_or_db(
            scope_env_t *env, args_t *args, eval_flags_t,
            const counted_t<const ql::db_t> &db,
            const boost::optional<name_string_t> &name_if_table) const {
        // Don't allow a rebalance call without explicit database
        if (args->num_args() == 0) {
	  rfail(base_exc_t::LOGIC, "`rebalance` can only be called on a table or database.");
        }

        ql::datum_t result;
        bool success;
        admin_err_t error;
        if (static_cast<bool>(name_if_table)) {
            success = env->env->reql_cluster_interface()->table_rebalance(
                db, *name_if_table, env->env->interruptor, &result, &error);
        } else {
            success = env->env->reql_cluster_interface()->db_rebalance(
                db, env->env->interruptor, &result, &error);
        }
        if (!success) {
            REQL_RETHROW(error);
        }
        return new_val(result);
    }
    virtual const char *name() const { return "rebalance"; }
};

class sync_term_t : public meta_op_term_t {
public:
    sync_term_t(compile_env_t *env, const raw_term_t &term)
        : meta_op_term_t(env, term, argspec_t(1)) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> t = args->arg(env, 0)->as_table();
        bool success = t->sync(env->env);
        r_sanity_check(success);
        ql::datum_object_builder_t result;
        result.overwrite("synced", ql::datum_t(1.0));
        return new_val(std::move(result).to_datum());
    }
    virtual const char *name() const { return "sync"; }
};

class table_term_t : public op_term_t {
public:
    table_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, 2),
                    optargspec_t({"read_mode", "use_outdated", "identifier_format"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        read_mode_t read_mode = read_mode_t::SINGLE;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "use_outdated")) {
            rfail(base_exc_t::LOGIC, "%s",
                  "The `use_outdated` optarg is no longer supported.  "
                  "Use the `read_mode` optarg instead.");
        }
        if (scoped_ptr_t<val_t> v = args->optarg(env, "read_mode")) {
            const datum_string_t &str = v->as_str();
            if (str == "majority") {
                read_mode = read_mode_t::MAJORITY;
            } else if (str == "single") {
                read_mode = read_mode_t::SINGLE;
            } else if (str == "outdated") {
                read_mode = read_mode_t::OUTDATED;
            } else if (str == "_debug_direct") {
                read_mode = read_mode_t::DEBUG_DIRECT;
            } else {
                rfail(base_exc_t::LOGIC, "Read mode `%s` unrecognized (options "
                      "are \"majority\", \"single\", and \"outdated\").",
                      str.to_std().c_str());
            }
        }

        auto identifier_format =
            boost::make_optional<admin_identifier_format_t>(
                false, admin_identifier_format_t());
        if (scoped_ptr_t<val_t> v = args->optarg(env, "identifier_format")) {
            const datum_string_t &str = v->as_str();
            if (str == "name") {
                identifier_format = admin_identifier_format_t::name;
            } else if (str == "uuid") {
                identifier_format = admin_identifier_format_t::uuid;
            } else {
                rfail(base_exc_t::LOGIC, "Identifier format `%s` unrecognized "
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

        admin_err_t error;
        counted_t<base_table_t> table;
        if (!env->env->reql_cluster_interface()->table_find(name, db,
                identifier_format, env->env->interruptor, &table, &error)) {
            REQL_RETHROW(error);
        }
        return new_val(make_counted<table_t>(
            std::move(table), db, name.str(), read_mode, backtrace()));
    }
    virtual deterministic_t is_deterministic() const { return deterministic_t::no; }
    virtual const char *name() const { return "table"; }
};

class get_term_t : public op_term_t {
public:
    get_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
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
    get_all_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, -1), optargspec_t({ "index" })) { }
private:
    datum_t get_key_arg(const scoped_ptr_t<val_t> &arg) const {
        datum_t datum_arg = arg->as_datum();

        rcheck_target(arg,
                      !datum_arg.is_ptype(pseudo::geometry_string),
                      base_exc_t::LOGIC,
                      "Cannot use a geospatial index with `get_all`. "
                      "Use `get_intersecting` instead.");
        rcheck_target(arg, datum_arg.get_type() != datum_t::R_NULL,
                      base_exc_t::NON_EXISTENCE,
                      "Keys cannot be NULL.");
        return datum_arg;
    }

    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        scoped_ptr_t<val_t> index = args->optarg(env, "index");
        std::string index_str = index ? index->as_str().to_std() : table->get_pkey();

        std::map<datum_t, uint64_t> keys;
        for (size_t i = 1; i < args->num_args(); ++i) {
            auto key = get_key_arg(args->arg(env, i));
            keys.insert(std::make_pair(std::move(key), 0)).first->second += 1;
        }

        return new_val(
            make_counted<selection_t>(
                table,
                table->get_all(env->env,
                               datumspec_t(std::move(keys)),
                               index_str,
                               backtrace())));
    }
    virtual const char *name() const { return "get_all"; }
};

counted_t<term_t> make_db_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<db_term_t>(env, term);
}

counted_t<term_t> make_table_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<table_term_t>(env, term);
}

counted_t<term_t> make_get_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<get_term_t>(env, term);
}

counted_t<term_t> make_get_all_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<get_all_term_t>(env, term);
}

counted_t<term_t> make_db_create_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<db_create_term_t>(env, term);
}

counted_t<term_t> make_db_drop_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<db_drop_term_t>(env, term);
}

counted_t<term_t> make_db_list_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<db_list_term_t>(env, term);
}

counted_t<term_t> make_table_create_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<table_create_term_t>(env, term);
}

counted_t<term_t> make_table_drop_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<table_drop_term_t>(env, term);
}

counted_t<term_t> make_table_list_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<table_list_term_t>(env, term);
}

counted_t<term_t> make_config_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<config_term_t>(env, term);
}

counted_t<term_t> make_status_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<status_term_t>(env, term);
}

counted_t<term_t> make_wait_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<wait_term_t>(env, term);
}

counted_t<term_t> make_reconfigure_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<reconfigure_term_t>(env, term);
}

counted_t<term_t> make_rebalance_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<rebalance_term_t>(env, term);
}

counted_t<term_t> make_sync_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sync_term_t>(env, term);
}

} // namespace ql
