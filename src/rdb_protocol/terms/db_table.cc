// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <map>
#include <string>

#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/suggester.hpp"
#include "rdb_protocol/meta_utils.hpp"
#include "rdb_protocol/op.hpp"
#include "rpc/directory/read_manager.hpp"

namespace ql {

durability_requirement_t parse_durability_optarg(counted_t<val_t> arg,
                                                 pb_rcheckable_t *target);

name_string_t get_name(counted_t<val_t> val, const term_t *caller) {
    r_sanity_check(val.has());
    std::string raw_name = val->as_str();
    name_string_t name;
    bool assignment_successful = name.assign_value(raw_name);
    rcheck_target(caller, base_exc_t::GENERIC, assignment_successful,
                  strprintf("Database name `%s` invalid (%s).",
                            raw_name.c_str(), name_string_t::valid_char_msg));
    return name;
}

// Meta operations (BUT NOT TABLE TERMS) should inherit from this.  It will
// handle a lot of the nasty semilattice initialization stuff for them,
// including the thread switching.
class meta_op_t : public op_term_t {
public:
    meta_op_t(compile_env_t *env, protob_t<const Term> term, argspec_t argspec,
              optargspec_t optargspec = optargspec_t({}))
        : op_term_t(env, std::move(term), std::move(argspec), std::move(optargspec)) { }

private:
    virtual bool is_deterministic_impl() const { return false; }
};

struct rethreading_metadata_accessor_t : public on_thread_t {
    explicit rethreading_metadata_accessor_t(scope_env_t *env)
    : on_thread_t(env->env->cluster_env.semilattice_metadata->home_thread()),
      metadata(env->env->cluster_env.semilattice_metadata->get()),
      ns_change(&metadata.rdb_namespaces),
      ns_searcher(&ns_change.get()->namespaces),
      db_searcher(&metadata.databases.databases),
      dc_searcher(&metadata.datacenters.datacenters)
    { }
    cluster_semilattice_metadata_t metadata;
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t
    ns_change;
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
    ns_searcher;
    metadata_searcher_t<database_semilattice_metadata_t> db_searcher;
    metadata_searcher_t<datacenter_semilattice_metadata_t> dc_searcher;
};


class meta_write_op_t : public meta_op_t {
public:
    meta_write_op_t(compile_env_t *env, protob_t<const Term> term, argspec_t argspec,
                    optargspec_t optargspec = optargspec_t({}))
        : meta_op_t(env, std::move(term), std::move(argspec), std::move(optargspec)) { }

protected:
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >
    directory_metadata(env_t *env) const {
        rcheck(env->cluster_env.directory_read_manager != NULL,
               base_exc_t::GENERIC,
               "Cannot nest meta operations inside queries.");
        r_sanity_check(env->cluster_env.directory_read_manager->home_thread() == get_thread_id());
        return env->cluster_env.directory_read_manager->get_root_view();
    }

private:

    virtual std::string write_eval_impl(scope_env_t *env, eval_flags_t flags) = 0;
    virtual counted_t<val_t> eval_impl(scope_env_t *env, eval_flags_t flags) {
        std::string op = write_eval_impl(env, flags);
        datum_ptr_t res(datum_t::R_OBJECT);
        UNUSED bool b = res.add(op, make_counted<datum_t>(1.0));
        return new_val(res.to_counted());
    }
};

class db_term_t : public meta_op_t {
public:
    db_term_t(compile_env_t *env, const protob_t<const Term> &term) : meta_op_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        name_string_t db_name = get_name(arg(env, 0), this);
        uuid_u uuid;
        {
            rethreading_metadata_accessor_t meta(env);
            uuid = meta_get_uuid(&meta.db_searcher, db_name,
                                 strprintf("Database `%s` does not exist.",
                                           db_name.c_str()), this);
        }
        return new_val(make_counted<const db_t>(uuid, db_name.str()));
    }
    virtual const char *name() const { return "db"; }
};

class db_create_term_t : public meta_write_op_t {
public:
    db_create_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1)) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        name_string_t db_name = get_name(arg(env, 0), this);

        rethreading_metadata_accessor_t meta(env);

        // Ensure database doesn't already exist.
        metadata_search_status_t status;
        meta.db_searcher.find_uniq(db_name, &status);
        rcheck(status == METADATA_ERR_NONE,
               base_exc_t::GENERIC,
               strprintf("Database `%s` already exists.", db_name.c_str()));

        // Create database, insert into metadata, then join into real metadata.
        database_semilattice_metadata_t db;
        db.name = vclock_t<name_string_t>(db_name, env->env->this_machine);
        meta.metadata.databases.databases.insert(
            std::make_pair(generate_uuid(), make_deletable(db)));
        try {
            fill_in_blueprints(&meta.metadata, directory_metadata(env->env)->get(),
                               env->env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
        env->env->cluster_env.join_and_wait_to_propagate(meta.metadata, env->env->interruptor);

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
                        optargspec_t({"datacenter", "primary_key",
                                    "cache_size", "durability"})) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        uuid_u dc_id = nil_uuid();
        if (counted_t<val_t> v = optarg(env, "datacenter")) {
            name_string_t name = get_name(v, this);
            {
                rethreading_metadata_accessor_t meta(env);
                dc_id = meta_get_uuid(&meta.dc_searcher, name,
                                      strprintf("Datacenter `%s` does not exist.",
                                                name.str().c_str()),
                                      this);
            }
        }

        const bool hard_durability
            = is_hard(parse_durability_optarg(optarg(env, "durability"), this));

        std::string primary_key = "id";
        if (counted_t<val_t> v = optarg(env, "primary_key")) {
            primary_key = v->as_str();
        }

        int64_t cache_size = 1073741824;
        if (counted_t<val_t> v = optarg(env, "cache_size")) {
            cache_size = v->as_int<int64_t>();
        }

        uuid_u db_id;
        name_string_t tbl_name;
        if (num_args() == 1) {
            counted_t<val_t> dbv = optarg(env, "db");
            r_sanity_check(dbv);
            db_id = dbv->as_db()->id;
            tbl_name = get_name(arg(env, 0), this);
        } else {
            db_id = arg(env, 0)->as_db()->id;
            tbl_name = get_name(arg(env, 1), this);
        }
        // Ensure table doesn't already exist.
        metadata_search_status_t status;
        namespace_predicate_t pred(&tbl_name, &db_id);

        const uuid_u namespace_id = generate_uuid();

        {
            rethreading_metadata_accessor_t meta(env);
            meta.ns_searcher.find_uniq(pred, &status);
            rcheck(status == METADATA_ERR_NONE,
                   base_exc_t::GENERIC,
                   strprintf("Table `%s` already exists.", tbl_name.c_str()));

            // Create namespace (DB + table pair) and insert into metadata.
            // The port here is a legacy from the day when memcached ran on a different port.
            namespace_semilattice_metadata_t<rdb_protocol_t> ns =
                new_namespace<rdb_protocol_t>(env->env->this_machine, db_id, dc_id, tbl_name,
                                              primary_key, port_defaults::reql_port,
                                              cache_size);

            // Set Durability
            std::map<datacenter_id_t, ack_expectation_t> *ack_map =
                &ns.ack_expectations.get_mutable();
            for (auto it = ack_map->begin(); it != ack_map->end(); ++it) {
                it->second = ack_expectation_t(it->second.expectation(), hard_durability);
            }
            ns.ack_expectations.upgrade_version(env->env->this_machine);

            meta.ns_change.get()->namespaces.insert(
                                                    std::make_pair(namespace_id, make_deletable(ns)));
            try {
                fill_in_blueprints(&meta.metadata, directory_metadata(env->env)->get(),
                                   env->env->this_machine, false);
            } catch (const missing_machine_exc_t &e) {
                rfail(base_exc_t::GENERIC, "%s", e.what());
            }
            env->env->cluster_env.join_and_wait_to_propagate(meta.metadata, env->env->interruptor);
        }

        // UGLY HACK BELOW (see wait_for_rdb_table_readiness)

        try {
            wait_for_rdb_table_readiness(env->env->cluster_env.ns_repo, namespace_id,
                                         env->env->interruptor,
                                         env->env->cluster_env.semilattice_metadata);
        } catch (const interrupted_exc_t &e) {
            rfail(base_exc_t::GENERIC, "Query interrupted, probably by user.");
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
    virtual std::string write_eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        name_string_t db_name = get_name(arg(env, 0), this);

        rethreading_metadata_accessor_t meta(env);

        // Get database metadata.
        metadata_search_status_t status;
        metadata_searcher_t<database_semilattice_metadata_t>::iterator
            db_metadata = meta.db_searcher.find_uniq(db_name, &status);
        rcheck(status == METADATA_SUCCESS, base_exc_t::GENERIC,
               strprintf("Database `%s` does not exist.", db_name.c_str()));
        guarantee(!db_metadata->second.is_deleted());
        uuid_u db_id = db_metadata->first;

        // Delete all tables in database.
        namespace_predicate_t pred(&db_id);
        for (auto it = meta.ns_searcher.find_next(meta.ns_searcher.begin(), pred);
             it != meta.ns_searcher.end();
             it = meta.ns_searcher.find_next(++it, pred)) {
            guarantee(!it->second.is_deleted());
            it->second.mark_deleted();
        }

        // Delete database
        db_metadata->second.mark_deleted();

        // Join
        try {
            fill_in_blueprints(&meta.metadata, directory_metadata(env->env)->get(),
                               env->env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
        env->env->cluster_env.join_and_wait_to_propagate(meta.metadata, env->env->interruptor);

        return "dropped";
    }
    virtual const char *name() const { return "db_drop"; }
};

class table_drop_term_t : public meta_write_op_t {
public:
    table_drop_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_write_op_t(env, term, argspec_t(1, 2)) { }
private:
    virtual std::string write_eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        uuid_u db_id;
        name_string_t tbl_name;
        if (num_args() == 1) {
            counted_t<val_t> dbv = optarg(env, "db");
            r_sanity_check(dbv);
            db_id = dbv->as_db()->id;
            tbl_name = get_name(arg(env, 0), this);
        } else {
            db_id = arg(env, 0)->as_db()->id;
            tbl_name = get_name(arg(env, 1), this);
        }

        rethreading_metadata_accessor_t meta(env);

        // Get table metadata.
        metadata_search_status_t status;
        namespace_predicate_t pred(&tbl_name, &db_id);
        metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
            ns_metadata = meta.ns_searcher.find_uniq(pred, &status);
        rcheck(status == METADATA_SUCCESS, base_exc_t::GENERIC,
               strprintf("Table `%s` does not exist.", tbl_name.c_str()));
        guarantee(!ns_metadata->second.is_deleted());

        // Delete table and join.
        ns_metadata->second.mark_deleted();
        try {
            fill_in_blueprints(&meta.metadata, directory_metadata(env->env)->get(),
                               env->env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
        env->env->cluster_env.join_and_wait_to_propagate(meta.metadata, env->env->interruptor);

        return "dropped";
    }
    virtual const char *name() const { return "table_drop"; }
};

class db_list_term_t : public meta_op_t {
public:
    db_list_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_t(env, term, argspec_t(0)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        std::vector<std::string> dbs;
        {
            rethreading_metadata_accessor_t meta(env);
            for (auto it = meta.db_searcher.find_next(meta.db_searcher.begin());
                 it != meta.db_searcher.end();
                 it = meta.db_searcher.find_next(++it)) {
                guarantee(!it->second.is_deleted());
                if (it->second.get_ref().name.in_conflict()) continue;
                dbs.push_back(it->second.get_ref().name.get().c_str());
            }
        }

        std::vector<counted_t<const datum_t> > arr;
        arr.reserve(dbs.size());
        for (auto it = dbs.begin(); it != dbs.end(); ++it) {
            arr.push_back(make_counted<datum_t>(std::move(*it)));
        }

        return new_val(make_counted<const datum_t>(std::move(arr)));
    }
    virtual const char *name() const { return "db_list"; }
};

class table_list_term_t : public meta_op_t {
public:
    table_list_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        meta_op_t(env, term, argspec_t(0, 1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        uuid_u db_id;
        if (num_args() == 0) {
            counted_t<val_t> dbv = optarg(env, "db");
            r_sanity_check(dbv);
            db_id = dbv->as_db()->id;
        } else {
            db_id = arg(env, 0)->as_db()->id;
        }
        std::vector<std::string> tables;
        namespace_predicate_t pred(&db_id);
        {
            rethreading_metadata_accessor_t meta(env);
            for (auto it = meta.ns_searcher.find_next(meta.ns_searcher.begin(), pred);
                 it != meta.ns_searcher.end();
                 it = meta.ns_searcher.find_next(++it, pred)) {
                guarantee(!it->second.is_deleted());
                if (it->second.get_ref().name.in_conflict()) continue;
                tables.push_back(it->second.get_ref().name.get().c_str());
            }
        }

        std::vector<counted_t<const datum_t> > arr;
        arr.reserve(tables.size());
        for (auto it = tables.begin(); it != tables.end(); ++it) {
            arr.push_back(make_counted<datum_t>(std::move(*it)));
        }
        return new_val(make_counted<const datum_t>(std::move(arr)));
    }
    virtual const char *name() const { return "table_list"; }
};

class table_term_t : public op_term_t {
public:
    table_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, 2), optargspec_t({ "use_outdated" })) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> t = optarg(env, "use_outdated");
        bool use_outdated = t ? t->as_bool() : false;
        counted_t<const db_t> db;
        std::string name;
        if (num_args() == 1) {
            counted_t<val_t> dbv = optarg(env, "db");
            r_sanity_check(dbv.has());
            db = dbv->as_db();
            name = arg(env, 0)->as_str();
        } else {
            r_sanity_check(num_args() == 2);
            db = arg(env, 0)->as_db();
            name = arg(env, 1)->as_str();
        }
        return new_val(make_counted<table_t>(env->env, db, name, use_outdated, backtrace()));
    }
    virtual bool is_deterministic_impl() const { return false; }
    virtual const char *name() const { return "table"; }
};

class get_term_t : public op_term_t {
public:
    get_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> table = arg(env, 0)->as_table();
        counted_t<const datum_t> pkey = arg(env, 1)->as_datum();
        counted_t<const datum_t> row = table->get_row(env->env, pkey);
        return new_val(row, table);
    }
    virtual const char *name() const { return "get"; }
};

class get_all_term_t : public op_term_t {
public:
    get_all_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2, -1), optargspec_t({ "index" })) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> table = arg(env, 0)->as_table();
        counted_t<val_t> index = optarg(env, "index");
        if (index && index->as_str() != table->get_pkey()) {
            std::vector<counted_t<datum_stream_t> > streams;
            for (size_t i = 1; i < num_args(); ++i) {
                counted_t<const datum_t> key = arg(env, i)->as_datum();
                counted_t<datum_stream_t> seq =
                    table->get_all(env->env, key, index->as_str(), backtrace());
                streams.push_back(seq);
            }
            counted_t<datum_stream_t> stream
                = make_counted<union_datum_stream_t>(streams, backtrace());
            return new_val(stream, table);
        } else {
            datum_ptr_t arr(datum_t::R_ARRAY);
            for (size_t i = 1; i < num_args(); ++i) {
                counted_t<const datum_t> key = arg(env, i)->as_datum();
                counted_t<const datum_t> row = table->get_row(env->env, key);
                if (row->get_type() != datum_t::R_NULL) {
                    arr.add(row);
                }
            }
            counted_t<datum_stream_t> stream
                = make_counted<array_datum_stream_t>(arr.to_counted(), backtrace());
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



} // namespace ql
