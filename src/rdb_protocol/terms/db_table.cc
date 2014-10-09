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

name_string_t get_name(const scoped_ptr_t<val_t> &val, const term_t *caller,
                       const char *type_str) {
    r_sanity_check(val.has());
    const datum_string_t &raw_name = val->as_str();
    name_string_t name;
    bool assignment_successful = name.assign_value(raw_name);
    rcheck_target(caller,
                  assignment_successful,
                  base_exc_t::GENERIC,
                  strprintf("%s name `%s` invalid (%s).",
                            type_str,
                            raw_name.to_std().c_str(),
                            name_string_t::valid_char_msg));
    return name;
}

std::map<name_string_t, size_t> get_replica_counts(scoped_ptr_t<val_t> arg) {
    r_sanity_check(arg.has());
    std::map<name_string_t, size_t> replica_counts;
    datum_t datum = arg->as_datum();
    if (datum.get_type() == datum_t::R_OBJECT) {
        for (size_t i = 0; i < datum.obj_size(); ++i) {
            std::pair<datum_string_t, datum_t> pair = datum.get_pair(i);
            name_string_t name;
            bool assignment_successful = name.assign_value(pair.first);
            rcheck_target(arg.get(), assignment_successful, base_exc_t::GENERIC,
                strprintf("Server tag name `%s` invalid (%s).",
                          pair.first.to_std().c_str(), name_string_t::valid_char_msg));
            int64_t replicas = checked_convert_to_int(arg.get(), pair.second.as_num());
            rcheck_target(arg.get(), replicas >= 0,
                base_exc_t::GENERIC, "Can't have a negative number of replicas");
            size_t replicas2 = static_cast<size_t>(replicas);
            rcheck_target(arg.get(), static_cast<int64_t>(replicas2) == replicas,
                base_exc_t::GENERIC, strprintf("Integer too large: %" PRIi64, replicas));
            replica_counts.insert(std::make_pair(name, replicas2));
        }
    } else if (datum.get_type() == datum_t::R_NUM) {
        size_t replicas = arg->as_int<size_t>();
        replica_counts.insert(std::make_pair(
            name_string_t::guarantee_valid("default"), replicas));
    } else {
        rfail_target(arg.get(), base_exc_t::GENERIC,
            "Expected type OBJECT or NUMBER but found %s:\n%s",
            datum.get_type_name().c_str(), datum.print().c_str());
    }
    return replica_counts;
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
        name_string_t db_name = get_name(args->arg(env, 0), this, "Database");
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
        name_string_t db_name = get_name(args->arg(env, 0), this, "Database");
        std::string error;
        if (!env->env->reql_cluster_interface()->db_create(db_name,
                env->env->interruptor, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }

        return "created";
    }
    virtual const char *name() const { return "db_create"; }
};

bool is_hard(durability_requirement_t requirement) {
    switch (requirement) {
    case DURABILITY_REQUIREMENT_DEFAULT:
    case DURABILITY_REQUIREMENT_HARD:
        return true;
    case DURABILITY_REQUIREMENT_SOFT:
        return false;
    default:
        unreachable();
    }
}

class table_create_term_t : public meta_write_op_t {
public:
    table_create_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1, 2),
                        optargspec_t({"datacenter", "primary_key", "durability"})) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {

        /* Parse arguments */
        boost::optional<name_string_t> primary_dc;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "datacenter")) {
            primary_dc.reset(get_name(v, this, "Table"));
        }

        const bool hard_durability
            = is_hard(parse_durability_optarg(args->optarg(env, "durability"), this));

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
            tbl_name = get_name(args->arg(env, 0), this, "Table");
        } else {
            db = args->arg(env, 0)->as_db();
            tbl_name = get_name(args->arg(env, 1), this, "Table");
        }

        /* Create the table */
        std::string error;
        if (!env->env->reql_cluster_interface()->table_create(tbl_name, db,
                primary_dc, hard_durability, primary_key,
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
        name_string_t db_name = get_name(args->arg(env, 0), this, "Database");

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
            tbl_name = get_name(args->arg(env, 0), this, "Table");
        } else {
            db = args->arg(env, 0)->as_db();
            tbl_name = get_name(args->arg(env, 1), this, "Table");
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

class table_config_or_status_term_t : public meta_op_term_t {
public:
    table_config_or_status_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(0, 2)) { }
protected:
    virtual bool impl(scope_env_t *env,
                      const boost::optional<name_string_t> name,
                      counted_t<const db_t> db,
                      scoped_ptr_t<val_t> *resp_out,
                      std::string *error_out) const = 0;
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v0, v1;
        if (args->num_args() > 0) {
            v0 = args->arg(env, 0);
        }
        if (args->num_args() > 1) {
            v1 = args->arg(env, 1);
        }

        counted_t<const db_t> db;
        bool db_arg_present = args->num_args() == 2 ||
            (args->num_args() == 1 && v0->get_type().is_convertible(val_t::type_t::DB));
        if (db_arg_present) {
            db = v0->as_db();
        } else {
            scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
            r_sanity_check(dbv);
            db = dbv->as_db();
        }

        boost::optional<name_string_t> name;
        if (args->num_args() > (db_arg_present ? 1 : 0)) {
            name = boost::optional<name_string_t>(
                get_name((db_arg_present ? v1 : v0), this, "Table"));
        }

        std::string error;
        scoped_ptr_t<val_t> resp;
        if (!impl(env, name, db, &resp, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }
        return resp;
    }
};

class table_config_term_t : public table_config_or_status_term_t {
public:
    table_config_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        table_config_or_status_term_t(env, term) { }
private:
    bool impl(scope_env_t *env, const boost::optional<name_string_t> name,
            counted_t<const db_t> db, scoped_ptr_t<val_t> *resp_out,
            std::string *error_out) const {
        return env->env->reql_cluster_interface()->table_config(name, db, backtrace(),
            env->env->interruptor, resp_out, error_out);
    }
    virtual const char *name() const { return "table_config"; }
};

class table_status_term_t : public table_config_or_status_term_t {
public:
    table_status_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        table_config_or_status_term_t(env, term) { }
private:
    bool impl(scope_env_t *env, const boost::optional<name_string_t> name,
            counted_t<const db_t> db, scoped_ptr_t<val_t> *resp_out,
            std::string *error_out) const {
        return env->env->reql_cluster_interface()->table_status(name, db, backtrace(),
            env->env->interruptor, resp_out, error_out);
    }
    virtual const char *name() const { return "table_status"; }
};

class reconfigure_term_t : public meta_op_term_t {
public:
    reconfigure_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_term_t(env, term, argspec_t(3),
            optargspec_t({"director_tag", "dry_run"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t)
            const {
        /* Parse parameters */
        /* RSI(reql_admin): Make sure the user didn't call `.between()` or `.order_by()`
        on this table */
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        table_generate_config_params_t config_params;
        config_params.num_shards = args->arg(env, 1)->as_int<int>();
        config_params.num_replicas = get_replica_counts(args->arg(env, 2));
        if (scoped_ptr_t<val_t> v = args->optarg(env, "director_tag")) {
            config_params.director_tag = get_name(v, this, "Server tag");
        } else {
            config_params.director_tag = name_string_t::guarantee_valid("default");
        }
        bool dry_run = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "dry_run")) {
            dry_run = v->as_bool();
        }
        /* Perform the operation */
        name_string_t name;
        bool ok = name.assign_value(table->name);
        guarantee(ok, "table->name should have been a valid name");
        datum_t new_config;
        std::string error;
        if (!env->env->reql_cluster_interface()->table_reconfigure(
                table->db, name, config_params, dry_run, env->env->interruptor,
                &new_config, &error)) {
            rfail(base_exc_t::GENERIC, "%s", error.c_str());
        }
        return new_val(new_config);
    }
    virtual const char *name() const { return "reconfigure"; }
};

class sync_term_t : public meta_write_op_t {
public:
    sync_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : meta_write_op_t(env, term, argspec_t(1)) { }

private:
    virtual std::string write_eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> t = args->arg(env, 0)->as_table();
        bool success = t->sync(env->env, this);
        r_sanity_check(success);
        return "synced";
    }
    virtual const char *name() const { return "sync"; }
};

class table_term_t : public op_term_t {
public:
    table_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, 2), optargspec_t({ "use_outdated" })) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> t = args->optarg(env, "use_outdated");
        bool use_outdated = t ? t->as_bool() : false;
        counted_t<const db_t> db;
        name_string_t name;
        if (args->num_args() == 1) {
            scoped_ptr_t<val_t> dbv = args->optarg(env, "db");
            r_sanity_check(dbv.has());
            db = dbv->as_db();
            name = get_name(args->arg(env, 0), this, "Table");
        } else {
            r_sanity_check(args->num_args() == 2);
            db = args->arg(env, 0)->as_db();
            name = get_name(args->arg(env, 1), this, "Table");
        }
        std::string error;
        scoped_ptr_t<base_table_t> table;
        if (!env->env->reql_cluster_interface()->table_find(name, db,
                env->env->interruptor, &table, &error)) {
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
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        datum_t pkey = args->arg(env, 1)->as_datum();
        datum_t row = table->get_row(env->env, pkey);
        return new_val(row, pkey, table);
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
            return new_val(stream, table);
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
            return new_val(stream, table);
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

counted_t<term_t> make_table_config_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_config_term_t>(env, term);
}

counted_t<term_t> make_table_status_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<table_status_term_t>(env, term);
}

counted_t<term_t> make_reconfigure_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<reconfigure_term_t>(env, term);
}

counted_t<term_t> make_sync_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sync_term_t>(env, term);
}



} // namespace ql
