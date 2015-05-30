// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/artificial_reql_cluster_interface.hpp"

#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/real_reql_cluster_interface.hpp"
#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view/field.hpp"

bool artificial_reql_cluster_interface_t::db_create(const name_string_t &name,
            signal_t *interruptor, ql::datum_t *result_out, std::string *error_out) {
    if (name == database) {
        *error_out = strprintf("Database `%s` already exists.", database.c_str());
        return false;
    }
    return next->db_create(name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_drop(const name_string_t &name,
        signal_t *interruptor, ql::datum_t *result_out, std::string *error_out) {
    if (name == database) {
        *error_out = strprintf("Database `%s` is special; you can't delete it.",
            database.c_str());
        return false;
    }
    return next->db_drop(name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_list(
        signal_t *interruptor,
        std::set<name_string_t> *names_out, std::string *error_out) {
    if (!next->db_list(interruptor, names_out, error_out)) {
        return false;
    }
    guarantee(names_out->count(database) == 0);
    names_out->insert(database);
    return true;
}

bool artificial_reql_cluster_interface_t::db_find(const name_string_t &name,
        signal_t *interruptor,
        counted_t<const ql::db_t> *db_out, std::string *error_out) {
    if (name == database) {
        *db_out = make_counted<const ql::db_t>(nil_uuid(), database);
        return true;
    }
    return next->db_find(name, interruptor, db_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_config(
        const counted_t<const ql::db_t> &db, ql::backtrace_id_t bt,
        ql::env_t *env, scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't configure it.",
            database.c_str());
        return false;
    }
    return next->db_config(db, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_create(
        const name_string_t &name, counted_t<const ql::db_t> db,
        const table_generate_config_params_t &config_params,
        const std::string &primary_key, write_durability_t durability,
        signal_t *interruptor, ql::datum_t *result_out, std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't create new tables "
            "in it.", database.c_str());
        return false;
    }
    return next->table_create(name, db, config_params, primary_key,
        durability, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_drop(const name_string_t &name,
        counted_t<const ql::db_t> db, signal_t *interruptor,
        ql::datum_t *result_out, std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't drop tables in it.",
            database.c_str());
        return false;
    }
    return next->table_drop(name, db, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_list(counted_t<const ql::db_t> db,
        signal_t *interruptor,
        std::set<name_string_t> *names_out, std::string *error_out) {
    if (db->name == database) {
        for (auto it = tables.begin(); it != tables.end(); ++it) {
            if (it->first.str()[0] == '_') {
                /* If a table's name starts with `_`, don't show it to the user unless
                they explicitly request it. */
                continue;
            }
            names_out->insert(it->first);
        }
        return true;
    }
    return next->table_list(db, interruptor, names_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_find(
        const name_string_t &name, counted_t<const ql::db_t> db,
        boost::optional<admin_identifier_format_t> identifier_format,
        signal_t *interruptor,
        counted_t<base_table_t> *table_out, std::string *error_out) {
    if (db->name == database) {
        auto it = tables.find(name);
        if (it != tables.end()) {
            artificial_table_backend_t *b;
            if (!static_cast<bool>(identifier_format) ||
                    *identifier_format == admin_identifier_format_t::name) {
                b = it->second.first;
            } else {
                b = it->second.second;
            }
            table_out->reset(new artificial_table_t(b));
            return true;
        } else {
            *error_out = strprintf("Table `%s.%s` does not exist.",
                database.c_str(), name.c_str());
            return false;
        }
    }
    return next->table_find(name, db, identifier_format,
        interruptor, table_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_estimate_doc_counts(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::env_t *env,
        std::vector<int64_t> *doc_counts_out,
        std::string *error_out) {
    if (db->name == database) {
        auto it = tables.find(name);
        if (it != tables.end()) {
            counted_t<ql::datum_stream_t> docs;
            /* We arbitrarily choose to read from the UUID version of the system table
            rather than the name version. */
            if (!it->second.second->read_all_rows_as_stream(
                    ql::backtrace_id_t::empty(),
                    ql::datum_range_t::universe(),
                    sorting_t::UNORDERED,
                    env->interruptor,
                    &docs,
                    error_out)) {
                *error_out = "When estimating doc count: " + *error_out;
                return false;
            }
            try {
                scoped_ptr_t<ql::val_t> count =
                    docs->run_terminal(env, ql::count_wire_func_t());
                *doc_counts_out = std::vector<int64_t>({ count->as_int<int64_t>() });
            } catch (const ql::base_exc_t &msg) {
                *error_out = "When estimating doc count: " + std::string(msg.what());
                return false;
            }
            return true;
        } else {
            *error_out = strprintf("Table `%s.%s` does not exist.",
                database.c_str(), name.c_str());
            return false;
        }
    } else {
        return next->table_estimate_doc_counts(db, name, env, doc_counts_out, error_out);
    }
}

bool artificial_reql_cluster_interface_t::table_config(
        counted_t<const ql::db_t> db, const name_string_t &name,
        ql::backtrace_id_t bt, ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out, std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't configure the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->table_config(db, name, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_status(
        counted_t<const ql::db_t> db, const name_string_t &name,
        ql::backtrace_id_t bt, ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out, std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; the system tables in it don't "
            "have meaningful status information.", database.c_str());
        return false;
    }
    return next->table_status(db, name, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_wait(
        counted_t<const ql::db_t> db, const name_string_t &name,
        table_readiness_t readiness, signal_t *interruptor,
        ql::datum_t *result_out, std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; the system tables in it are "
            "always available and don't need to be waited on.", database.c_str());
        return false;
    }
    return next->table_wait(db, name, readiness, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_wait(
        counted_t<const ql::db_t> db, table_readiness_t readiness,
        signal_t *interruptor,
        ql::datum_t *result_out, std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; the system tables in it are "
            "always available and don't need to be waited on.", database.c_str());
        return false;
    }
    return next->db_wait(db, readiness, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_reconfigure(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't configure the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->table_reconfigure(db, name, params, dry_run, interruptor,
        result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_reconfigure(
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't configure the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->db_reconfigure(db, params, dry_run, interruptor,
        result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_emergency_repair(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        bool allow_data_loss,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't configure the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->table_emergency_repair(db, name, allow_data_loss, dry_run, interruptor,
        result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_rebalance(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't rebalance the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->table_rebalance(db, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_rebalance(
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't rebalance the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->db_rebalance(db, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_create(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const sindex_config_t &config,
        signal_t *interruptor,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Database `%s` is special; you can't create secondary "
            "indexes on the tables in it.", database.c_str());
        return false;
    }
    return next->sindex_create(db, table, name, config, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_drop(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        signal_t *interruptor,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Index `%s` does not exist on table `%s.%s`.",
            name.c_str(), db->name.c_str(), table.c_str());
        return false;
    }
    return next->sindex_drop(db, table, name, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_rename(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const std::string &new_name,
        bool overwrite,
        signal_t *interruptor,
        std::string *error_out) {
    if (db->name == database) {
        *error_out = strprintf("Index `%s` does not exist on table `%s.%s`.",
            name.c_str(), db->name.c_str(), table.c_str());
        return false;
    }
    return next->sindex_rename(
        db, table, name, new_name, overwrite, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_list(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        signal_t *interruptor,
        std::string *error_out,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *configs_and_statuses_out) {
    if (db->name == database) {
        configs_and_statuses_out->clear();
        return true;
    }
    return next->sindex_list(
        db, table, interruptor, error_out, configs_and_statuses_out);
}

admin_artificial_tables_t::admin_artificial_tables_t(
        real_reql_cluster_interface_t *_next_reql_cluster_interface,
        boost::shared_ptr< semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        boost::shared_ptr< semilattice_readwrite_view_t<
            auth_semilattice_metadata_t> > _auth_view,
        clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory_map_view,
        table_meta_client_t *_table_meta_client,
        server_config_client_t *_server_config_client,
        mailbox_manager_t *_mailbox_manager) {
    std::map<name_string_t,
        std::pair<artificial_table_backend_t *, artificial_table_backend_t *> > backends;

    cluster_config_backend.init(new cluster_config_artificial_table_backend_t(
        _auth_view));
    backends[name_string_t::guarantee_valid("cluster_config")] =
        std::make_pair(cluster_config_backend.get(), cluster_config_backend.get());

    db_config_backend.init(new db_config_artificial_table_backend_t(
        metadata_field(&cluster_semilattice_metadata_t::databases,
            _semilattice_view)));
    backends[name_string_t::guarantee_valid("db_config")] =
        std::make_pair(db_config_backend.get(), db_config_backend.get());

    for (int i = 0; i < 2; ++i) {
        issues_backend[i].init(new issues_artificial_table_backend_t(
            _semilattice_view,
            _directory_view,
            _server_config_client,
            _table_meta_client,
            static_cast<admin_identifier_format_t>(i)));
    }
    backends[name_string_t::guarantee_valid("current_issues")] =
        std::make_pair(issues_backend[0].get(), issues_backend[1].get());

    for (int i = 0; i < 2; ++i) {
        logs_backend[i].init(new logs_artificial_table_backend_t(
            _mailbox_manager,
            _directory_map_view,
            _server_config_client,
            static_cast<admin_identifier_format_t>(i)));
    }
    backends[name_string_t::guarantee_valid("logs")] =
        std::make_pair(logs_backend[0].get(), logs_backend[1].get());

    server_config_backend.init(new server_config_artificial_table_backend_t(
        metadata_field(&cluster_semilattice_metadata_t::servers,
            _semilattice_view),
        _server_config_client));
    backends[name_string_t::guarantee_valid("server_config")] =
        std::make_pair(server_config_backend.get(), server_config_backend.get());

    server_status_backend.init(new server_status_artificial_table_backend_t(
        metadata_field(&cluster_semilattice_metadata_t::servers,
            _semilattice_view),
        _server_config_client,
        _directory_map_view));
    backends[name_string_t::guarantee_valid("server_status")] =
        std::make_pair(server_status_backend.get(), server_status_backend.get());

    for (int i = 0; i < 2; ++i) {
        stats_backend[i].init(new stats_artificial_table_backend_t(
            _directory_view, _semilattice_view, _server_config_client,
            _table_meta_client, _mailbox_manager,
            static_cast<admin_identifier_format_t>(i)));
    }
    backends[name_string_t::guarantee_valid("stats")] =
        std::make_pair(stats_backend[0].get(), stats_backend[1].get());

    for (int i = 0; i < 2; ++i) {
        table_config_backend[i].init(new table_config_artificial_table_backend_t(
            _semilattice_view,
            _next_reql_cluster_interface,
            static_cast<admin_identifier_format_t>(i),
            _server_config_client,
            _table_meta_client));
    }
    backends[name_string_t::guarantee_valid("table_config")] =
        std::make_pair(table_config_backend[0].get(), table_config_backend[1].get());

    for (int i = 0; i < 2; ++i) {
        table_status_backend[i].init(new table_status_artificial_table_backend_t(
            _semilattice_view,
            _table_meta_client,
            static_cast<admin_identifier_format_t>(i),
            _server_config_client));
    }
    backends[name_string_t::guarantee_valid("table_status")] =
        std::make_pair(table_status_backend[0].get(), table_status_backend[1].get());

    for (int i = 0; i < 2; ++i) {
        jobs_backend[i].init(new jobs_artificial_table_backend_t(
            _mailbox_manager,
            _semilattice_view,
            _directory_view,
            _server_config_client,
            _table_meta_client,
            static_cast<admin_identifier_format_t>(i)));
    }
    backends[name_string_t::guarantee_valid("jobs")] =
        std::make_pair(jobs_backend[0].get(), jobs_backend[1].get());

    debug_scratch_backend.init(new in_memory_artificial_table_backend_t);
    backends[name_string_t::guarantee_valid("_debug_scratch")] =
        std::make_pair(debug_scratch_backend.get(), debug_scratch_backend.get());

    debug_stats_backend.init(new debug_stats_artificial_table_backend_t(
        metadata_field(&cluster_semilattice_metadata_t::servers,
            _semilattice_view),
        _server_config_client,
        _directory_map_view,
        _mailbox_manager));
    backends[name_string_t::guarantee_valid("_debug_stats")] =
        std::make_pair(debug_stats_backend.get(), debug_stats_backend.get());

    debug_table_status_backend.init(new debug_table_status_artificial_table_backend_t(
        _semilattice_view,
        _table_meta_client,
        _server_config_client));
    backends[name_string_t::guarantee_valid("_debug_table_status")] =
        std::make_pair(debug_table_status_backend.get(),
                       debug_table_status_backend.get());

    reql_cluster_interface.init(new artificial_reql_cluster_interface_t(
        name_string_t::guarantee_valid("rethinkdb"),
        backends,
        _next_reql_cluster_interface));
}

