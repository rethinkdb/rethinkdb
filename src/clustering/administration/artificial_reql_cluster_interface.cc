// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/artificial_reql_cluster_interface.hpp"

#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/real_reql_cluster_interface.hpp"
#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view/field.hpp"

bool artificial_reql_cluster_interface_t::db_create(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` already exists.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->db_create(user_context, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_drop(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't delete it.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->db_drop(user_context, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_list(
        signal_t *interruptor,
        std::set<name_string_t> *names_out, admin_err_t *error_out) {
    if (!m_next->db_list(interruptor, names_out, error_out)) {
        return false;
    }
    guarantee(names_out->count(m_database) == 0);
    names_out->insert(m_database);
    return true;
}

bool artificial_reql_cluster_interface_t::db_find(const name_string_t &name,
        signal_t *interruptor,
        counted_t<const ql::db_t> *db_out, admin_err_t *error_out) {
    if (name == m_database) {
        *db_out = make_counted<const ql::db_t>(nil_uuid(), m_database);
        return true;
    }
    return m_next->db_find(name, interruptor, db_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_config(
        auth::user_context_t const &user_context,
        const counted_t<const ql::db_t> &db,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure it.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->db_config(user_context, db, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_create(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &config_params,
        const std::string &primary_key,
        write_durability_t durability,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't create new tables "
                      "in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_create(
        user_context,
        name,
        db,
        config_params,
        primary_key,
        durability,
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::table_drop(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't drop tables in it.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_drop(user_context, name, db, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_list(counted_t<const ql::db_t> db,
        signal_t *interruptor,
        std::set<name_string_t> *names_out, admin_err_t *error_out) {
    if (db->name == m_database) {
        for (auto it = m_tables.begin(); it != m_tables.end(); ++it) {
            if (it->first.str()[0] == '_') {
                /* If a table's name starts with `_`, don't show it to the user unless
                they explicitly request it. */
                continue;
            }
            names_out->insert(it->first);
        }
        return true;
    }
    return m_next->table_list(db, interruptor, names_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_find(
        const name_string_t &name, counted_t<const ql::db_t> db,
        boost::optional<admin_identifier_format_t> identifier_format,
        signal_t *interruptor,
        counted_t<base_table_t> *table_out, admin_err_t *error_out) {
    if (db->name == m_database) {
        auto it = m_tables.find(name);
        if (it != m_tables.end()) {
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
            *error_out = admin_err_t{
                strprintf("Table `%s.%s` does not exist.",
                          m_database.c_str(), name.c_str()),
                query_state_t::FAILED};
            return false;
        }
    }
    return m_next->table_find(name, db, identifier_format,
        interruptor, table_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_estimate_doc_counts(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::env_t *env,
        std::vector<int64_t> *doc_counts_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        auto it = m_tables.find(name);
        if (it != m_tables.end()) {
            counted_t<ql::datum_stream_t> docs;
            /* We arbitrarily choose to read from the UUID version of the system table
            rather than the name version. */
            if (!it->second.second->read_all_rows_as_stream(
                    ql::backtrace_id_t::empty(),
                    ql::datumspec_t(ql::datum_range_t::universe()),
                    sorting_t::UNORDERED,
                    env->interruptor,
                    &docs,
                    error_out)) {
                error_out->msg = "When estimating doc count: " + error_out->msg;
                return false;
            }
            try {
                scoped_ptr_t<ql::val_t> count =
                    docs->run_terminal(env, ql::count_wire_func_t());
                *doc_counts_out = std::vector<int64_t>({ count->as_int<int64_t>() });
            } catch (const ql::base_exc_t &msg) {
                *error_out = admin_err_t{
                    "When estimating doc count: " + std::string(msg.what()),
                    query_state_t::FAILED};
                return false;
            }
            return true;
        } else {
            *error_out = admin_err_t{
                strprintf("Table `%s.%s` does not exist.",
                          m_database.c_str(), name.c_str()),
                query_state_t::FAILED};
            return false;
        }
    } else {
        return m_next->table_estimate_doc_counts(
            user_context, db, name, env, doc_counts_out, error_out);
    }
}

bool artificial_reql_cluster_interface_t::table_config(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out, admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_config(
        user_context, db, name, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_status(
        counted_t<const ql::db_t> db, const name_string_t &name,
        ql::backtrace_id_t bt, ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out, admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; the system tables in it don't "
                      "have meaningful status information.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_status(db, name, bt, env, selection_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_wait(
        counted_t<const ql::db_t> db, const name_string_t &name,
        table_readiness_t readiness, signal_t *interruptor,
        ql::datum_t *result_out, admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; the system tables in it are "
                      "always available and don't need to be waited on.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_wait(db, name, readiness, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_wait(
        counted_t<const ql::db_t> db, table_readiness_t readiness,
        signal_t *interruptor,
        ql::datum_t *result_out, admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; the system tables in it are "
                      "always available and don't need to be waited on.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->db_wait(db, readiness, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_reconfigure(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_reconfigure(
        user_context, db, name, params, dry_run, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_reconfigure(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->db_reconfigure(
        user_context, db, params, dry_run, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_emergency_repair(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        emergency_repair_mode_t mode,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't configure the "
                      "tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_emergency_repair(
        user_context, db, name, mode, dry_run, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_rebalance(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't rebalance the "
                      "tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->table_rebalance(
        user_context, db, name, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::db_rebalance(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't rebalance the "
                      "tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->db_rebalance(user_context, db, interruptor, result_out, error_out);
}

bool artificial_reql_cluster_interface_t::grant_global(
        auth::user_context_t const &user_context,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    return m_next->grant_global(
        user_context,
        std::move(username),
        std::move(permissions),
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::grant_database(
        auth::user_context_t const &user_context,
        database_id_t const &database_id,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (database_id.is_nil()) {
        *error_out = admin_err_t{
            strprintf("The `%s` database is special, you can't grant permissions on it.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->grant_database(
        user_context,
        database_id,
        std::move(username),
        std::move(permissions),
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::grant_table(
        auth::user_context_t const &user_context,
        database_id_t const &database_id,
        namespace_id_t const &table_id,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (database_id.is_nil() || table_id.is_nil()) {
        *error_out = admin_err_t{
            strprintf("The `%s` database is special, you can't grant permissions on it.",
                      m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->grant_table(
        user_context,
        database_id,
        table_id,
        std::move(username),
        std::move(permissions),
        interruptor,
        result_out,
        error_out);
}

bool artificial_reql_cluster_interface_t::sindex_create(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const sindex_config_t &config,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Database `%s` is special; you can't create secondary "
                      "indexes on the tables in it.", m_database.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->sindex_create(
        user_context, db, table, name, config, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_drop(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Index `%s` does not exist on table `%s.%s`.",
                      name.c_str(), db->name.c_str(), table.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->sindex_drop(user_context, db, table, name, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_rename(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const std::string &new_name,
        bool overwrite,
        signal_t *interruptor,
        admin_err_t *error_out) {
    if (db->name == m_database) {
        *error_out = admin_err_t{
            strprintf("Index `%s` does not exist on table `%s.%s`.",
                      name.c_str(), db->name.c_str(), table.c_str()),
            query_state_t::FAILED};
        return false;
    }
    return m_next->sindex_rename(
        user_context, db, table, name, new_name, overwrite, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::sindex_list(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        signal_t *interruptor,
        admin_err_t *error_out,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *configs_and_statuses_out) {
    if (db->name == m_database) {
        configs_and_statuses_out->clear();
        return true;
    }
    return m_next->sindex_list(
        db, table, interruptor, error_out, configs_and_statuses_out);
}

admin_artificial_tables_t::admin_artificial_tables_t(
        real_reql_cluster_interface_t *_next_reql_cluster_interface,
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
            auth_semilattice_view,
        boost::shared_ptr<semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        boost::shared_ptr<semilattice_readwrite_view_t<
            heartbeat_semilattice_metadata_t> > _heartbeat_view,
        clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory_map_view,
        table_meta_client_t *_table_meta_client,
        server_config_client_t *_server_config_client,
        namespace_repo_t *_namespace_repo,
        mailbox_manager_t *_mailbox_manager) {
    std::map<name_string_t,
        std::pair<artificial_table_backend_t *, artificial_table_backend_t *> > backends;

    for (int i = 0; i < 2; ++i) {
        permissions_backend[i].init(new auth::permissions_artificial_table_backend_t(
            auth_semilattice_view,
            _semilattice_view,
            _table_meta_client,
            static_cast<admin_identifier_format_t>(i)));
    }
    backends[name_string_t::guarantee_valid("permissions")] =
        std::make_pair(permissions_backend[0].get(), permissions_backend[1].get());

    users_backend.init(new auth::users_artificial_table_backend_t(
        auth_semilattice_view,
        _semilattice_view));
    backends[name_string_t::guarantee_valid("users")] =
        std::make_pair(users_backend.get(), users_backend.get());

    cluster_config_backend.init(new cluster_config_artificial_table_backend_t(
        _heartbeat_view));
    backends[name_string_t::guarantee_valid("cluster_config")] =
        std::make_pair(cluster_config_backend.get(), cluster_config_backend.get());

    db_config_backend.init(new db_config_artificial_table_backend_t(
        metadata_field(&cluster_semilattice_metadata_t::databases,
                       _semilattice_view),
        _next_reql_cluster_interface));

    backends[name_string_t::guarantee_valid("db_config")] =
        std::make_pair(db_config_backend.get(), db_config_backend.get());

    for (int i = 0; i < 2; ++i) {
        issues_backend[i].init(new issues_artificial_table_backend_t(
            _mailbox_manager,
            _semilattice_view,
            _directory_map_view,
            _server_config_client,
            _table_meta_client,
            _namespace_repo,
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
        _directory_map_view,
        _server_config_client));
    backends[name_string_t::guarantee_valid("server_config")] =
        std::make_pair(server_config_backend.get(), server_config_backend.get());

    server_status_backend.init(new server_status_artificial_table_backend_t(
        _directory_map_view,
        _server_config_client));
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
            _server_config_client,
            _table_meta_client,
            _namespace_repo,
            static_cast<admin_identifier_format_t>(i)));
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
        _directory_map_view,
        _server_config_client,
        _mailbox_manager));
    backends[name_string_t::guarantee_valid("_debug_stats")] =
        std::make_pair(debug_stats_backend.get(), debug_stats_backend.get());

    debug_table_status_backend.init(new debug_table_status_artificial_table_backend_t(
        _semilattice_view,
        _table_meta_client));
    backends[name_string_t::guarantee_valid("_debug_table_status")] =
        std::make_pair(debug_table_status_backend.get(),
                       debug_table_status_backend.get());

    reql_cluster_interface.init(new artificial_reql_cluster_interface_t(
        name_string_t::guarantee_valid("rethinkdb"),
        backends,
        _next_reql_cluster_interface));
}

