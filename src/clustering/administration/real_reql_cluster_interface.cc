// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/real_reql_cluster_interface.hpp"

#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/administration/tables/calculate_status.hpp"
#include "clustering/administration/tables/generate_config.hpp"
#include "clustering/administration/tables/split_points.hpp"
#include "clustering/administration/tables/table_config.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/table_common.hpp"
#include "rdb_protocol/val.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "rpc/semilattice/view/field.hpp"

#define NAMESPACE_INTERFACE_EXPIRATION_MS (60 * 1000)

real_reql_cluster_interface_t::real_reql_cluster_interface_t(
        mailbox_manager_t *mailbox_manager,
        boost::shared_ptr<semilattice_readwrite_view_t<
            auth_semilattice_metadata_t> > auth_semilattice_view,
        boost::shared_ptr<semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > cluster_semilattice_view,
        rdb_context_t *rdb_context,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        multi_table_manager_t *multi_table_manager,
        watchable_map_t<
            std::pair<peer_id_t, std::pair<namespace_id_t, branch_id_t> >,
            table_query_bcard_t> *table_query_directory
        ) :
    m_mailbox_manager(mailbox_manager),
    m_auth_semilattice_view(auth_semilattice_view),
    m_cluster_semilattice_view(cluster_semilattice_view),
    m_table_meta_client(table_meta_client),
    m_cross_thread_database_watchables(get_num_threads()),
    m_rdb_context(rdb_context),
    m_namespace_repo(
        m_mailbox_manager,
        table_query_directory,
        multi_table_manager,
        m_rdb_context,
        m_table_meta_client),
    m_changefeed_client(
        m_mailbox_manager,
        [this](const namespace_id_t &id, signal_t *interruptor) {
            return this->m_namespace_repo.get_namespace_interface(id, interruptor);
        }),
    m_server_config_client(server_config_client)
{
    guarantee(m_auth_semilattice_view->home_thread() == home_thread());
    guarantee(m_cluster_semilattice_view->home_thread() == home_thread());
    guarantee(m_table_meta_client->home_thread() == home_thread());
    guarantee(m_server_config_client->home_thread() == home_thread());
    for (int thr = 0; thr < get_num_threads(); ++thr) {
        m_cross_thread_database_watchables[thr].init(
            new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                    (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                        metadata_field(&cluster_semilattice_metadata_t::databases, m_cluster_semilattice_view))), threadnum_t(thr)));
    }
}

bool real_reql_cluster_interface_t::db_create(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    user_context.require_config_permission(m_rdb_context);

    cluster_semilattice_metadata_t metadata;
    ql::datum_t new_config;
    {
        on_thread_t thread_switcher(home_thread());
        metadata = m_cluster_semilattice_view->get();

        /* Make sure there isn't an existing database with the same name. */
        for (const auto &pair : metadata.databases.databases) {
            if (!pair.second.is_deleted() &&
                    pair.second.get_ref().name.get_ref() == name) {
                *error_out = admin_err_t{
                    strprintf("Database `%s` already exists.", name.c_str()),
                    query_state_t::FAILED};
                return false;
            }
        }

        database_id_t db_id = generate_uuid();
        database_semilattice_metadata_t db;
        db.name = versioned_t<name_string_t>(name);
        metadata.databases.databases.insert(std::make_pair(db_id, make_deletable(db)));

        m_cluster_semilattice_view->join(metadata);
        metadata = m_cluster_semilattice_view->get();

        new_config = convert_db_config_and_name_to_datum(name, db_id);
    }
    wait_for_cluster_metadata_to_propagate(metadata, interruptor_on_caller);

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("dbs_created", ql::datum_t(1.0));
    result_builder.overwrite("config_changes",
        make_replacement_pair(ql::datum_t::null(), new_config));
    *result_out = std::move(result_builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::db_drop_uuid(
            auth::user_context_t const &user_context,
            database_id_t database_id,
            const name_string_t &name,
            signal_t *interruptor_on_home,
            ql::datum_t *result_out,
            admin_err_t *error_out) {
    assert_thread();
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    std::map<namespace_id_t, table_basic_config_t> table_names;
    m_table_meta_client->list_names(&table_names);
    std::set<namespace_id_t> table_ids;
    for (auto const &table_name : table_names) {
        if (table_name.second.database == database_id) {
            table_ids.insert(table_name.first);
        }
    }

    user_context.require_config_permission(m_rdb_context, database_id, table_ids);

    // Here we actually delete the tables
    size_t tables_dropped = 0;
    for (const auto &table_id : table_ids) {
        try {
            m_table_meta_client->drop(table_id, interruptor_on_home);
            ++tables_dropped;
        } catch (const no_such_table_exc_t &) {
             /* The table was dropped by something else between the time when we called
             `list_names()` and when we went to actually delete it. This is OK. */
        } CATCH_OP_ERRORS(name, table_names.at(table_id).name, error_out,
            "The database was not dropped, but some of the tables in it may or may not "
                "have been dropped.",
            "The database was not dropped, but some of the tables in it may or may not "
                "have been dropped.")
    }

    cluster_semilattice_metadata_t metadata = m_cluster_semilattice_view->get();
    auto iter = metadata.databases.databases.find(database_id);
    if (iter != metadata.databases.databases.end() && !iter->second.is_deleted()) {
        iter->second.mark_deleted();
        m_cluster_semilattice_view->join(metadata);
        metadata = m_cluster_semilattice_view->get();
    } else {
        *error_out = admin_err_t{
            "The database was already deleted.", query_state_t::FAILED};
        return false;
    }
    wait_for_cluster_metadata_to_propagate(metadata, interruptor_on_home);

    if (result_out != nullptr) {
        ql::datum_t old_config =
            convert_db_config_and_name_to_datum(name, database_id);

        ql::datum_object_builder_t result_builder;
        result_builder.overwrite("dbs_dropped", ql::datum_t(1.0));
        result_builder.overwrite(
            "tables_dropped", ql::datum_t(static_cast<double>(tables_dropped)));
        result_builder.overwrite(
            "config_changes",
            make_replacement_pair(std::move(old_config), ql::datum_t::null()));
        *result_out = std::move(result_builder).to_datum();
    }

    return true;
}

bool real_reql_cluster_interface_t::db_drop(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    {
        cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
        on_thread_t thread_switcher(home_thread());

        cluster_semilattice_metadata_t metadata = m_cluster_semilattice_view->get();
        database_id_t database_id;
        if (!search_db_metadata_by_name(
                metadata.databases,
                name,
                &database_id,
                error_out)) {
            return false;
        }

        if (!db_drop_uuid(
                user_context,
                database_id,
                name,
                &interruptor_on_home,
                result_out,
                error_out)) {
            return false;
        }
    }
    return true;
}

bool real_reql_cluster_interface_t::db_list(
        UNUSED signal_t *interruptor_on_caller,
        std::set<name_string_t> *names_out,
        UNUSED admin_err_t *error_out) {
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    for (const auto &pair : db_metadata.databases) {
        if (!pair.second.is_deleted()) {
            names_out->insert(pair.second.get_ref().name.get_ref());
        }
    }
    return true;
}

bool real_reql_cluster_interface_t::db_find(
        const name_string_t &name,
        UNUSED signal_t *interruptor_on_caller,
        counted_t<const ql::db_t> *db_out,
        admin_err_t *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    /* Find the specified database */
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    database_id_t db_id;
    if (!search_db_metadata_by_name(db_metadata, name, &db_id, error_out)) {
        return false;
    }
    *db_out = make_counted<const ql::db_t>(db_id, name);
    return true;
}

bool real_reql_cluster_interface_t::db_config(
        const counted_t<const ql::db_t> &db,
        ql::backtrace_id_t backtrace_id,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        admin_err_t *error_out) {
    try {
        // FIXME permissions
        make_single_selection(
            admin_tables->db_config_backend.get(),
            name_string_t::guarantee_valid("db_config"),
            db->id,
            backtrace_id,
            env,
            selection_out);
        return true;
    } catch (const no_such_table_exc_t &) {
        *error_out = admin_err_t{
            strprintf("Database `%s` does not exist.", db->name.c_str()),
            query_state_t::FAILED};
        return false;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    }
}

bool real_reql_cluster_interface_t::table_create(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &config_params,
        const std::string &primary_key,
        write_durability_t durability,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    user_context.require_config_permission(m_rdb_context, db->id);

    namespace_id_t table_id;
    cluster_semilattice_metadata_t metadata;
    ql::datum_t new_config;
    try {
        cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
        on_thread_t thread_switcher(home_thread());

        /* Make sure there isn't an existing table with the same name */
        if (m_table_meta_client->exists(db->id, name)) {
            *error_out = admin_err_t{
                strprintf("Table `%s.%s` already exists.",
                          db->name.c_str(), name.c_str()),
                query_state_t::FAILED};
            return false;
        }

        table_config_and_shards_t config;

        config.config.basic.name = name;
        config.config.basic.database = db->id;
        config.config.basic.primary_key = primary_key;

        /* We don't have any data to generate split points based on, so assume UUIDs */
        calculate_split_points_for_uuids(
            config_params.num_shards, &config.shard_scheme);

        /* Pick which servers to host the data */
        table_generate_config(
            m_server_config_client, nil_uuid(), m_table_meta_client,
            config_params, config.shard_scheme, &interruptor_on_home,
            &config.config.shards, &config.server_names);

        config.config.write_ack_config = write_ack_config_t::MAJORITY;
        config.config.durability = durability;

        table_id = generate_uuid();
        m_table_meta_client->create(table_id, config, &interruptor_on_home);

        new_config = convert_table_config_to_datum(table_id,
            convert_name_to_datum(db->name), config.config,
            admin_identifier_format_t::name, config.server_names);

    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        "The table was not created.",
        "The table may or may not have been created.")

    /* Wait for the table to set itself up. Various things could interfere with this
    process, causing it to wait indefinitely; for example, the table might be deleted, or
    a server might go down. So we set a 10-second timeout. */
    signal_timer_t timer;
    timer.start(10000);
    wait_any_t combined_interruptor(&timer, interruptor_on_caller);
    try {
        wait_for_table_readiness(
            table_id,
            table_readiness_t::finished,
            &m_namespace_repo,
            m_table_meta_client,
            &combined_interruptor);
    } catch (const no_such_table_exc_t &) {
        /* The table was deleted after being created. Just return normally. */
    } catch (const interrupted_exc_t &) {
        if (interruptor_on_caller->is_pulsed()) {
            throw interrupted_exc_t();
        }
        /* Ten seconds elapsed. Just return normally. */
    }

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("tables_created", ql::datum_t(1.0));
    result_builder.overwrite("config_changes",
        make_replacement_pair(ql::datum_t::null(), new_config));
    *result_out = std::move(result_builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::table_drop(
        auth::user_context_t const &user_context,
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    cluster_semilattice_metadata_t metadata;
    try {
        cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
        on_thread_t thread_switcher(home_thread());
        metadata = m_cluster_semilattice_view->get();

        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        /* Fetch the old config via the `table_config_backend` rather than via
        `table_meta_client_t` because this will return an error document instead of
        crashing if the table is not reachable. */
        ql::datum_t old_config;
        artificial_table_backend_t *config_backend = admin_tables->table_config_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();
        if (!config_backend->read_row(convert_uuid_to_datum(table_id),
                &interruptor_on_home, &old_config, error_out)) {
            return false;
        } else if (!old_config.has()) {
            throw no_such_table_exc_t();
        }

        m_table_meta_client->drop(table_id, &interruptor_on_home);

        ql::datum_object_builder_t result_builder;
        result_builder.overwrite("tables_dropped", ql::datum_t(1.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config, ql::datum_t::null()));
        *result_out = std::move(result_builder).to_datum();

        return true;

    } CATCH_NAME_ERRORS(db->name, name, error_out)
}

bool real_reql_cluster_interface_t::table_list(
        counted_t<const ql::db_t> db,
        UNUSED signal_t *interruptor_on_caller,
        std::set<name_string_t> *names_out,
        UNUSED admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    std::map<namespace_id_t, table_basic_config_t> tables;
    m_table_meta_client->list_names(&tables);
    for (const auto &pair : tables) {
        if (pair.second.database == db->id) {
            names_out->insert(pair.second.name);
        }
    }
    return true;
}

bool real_reql_cluster_interface_t::table_find(
        const name_string_t &name,
        counted_t<const ql::db_t> db,
        UNUSED boost::optional<admin_identifier_format_t> identifier_format,
        signal_t *interruptor_on_caller,
        counted_t<base_table_t> *table_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    namespace_id_t table_id;
    std::string primary_key;
    try {
        m_table_meta_client->find(db->id, name, &table_id, &primary_key);

        /* Note that we completely ignore `identifier_format`. `identifier_format` is
        meaningless for real tables, so it might seem like we should produce an error.
        The reason we don't is that the user might write a query that access both a
        system table and a real table, and they might specify `identifier_format` as a
        global optarg. So then they would get a spurious error for the real table. This
        behavior is also consistent with that of system tables that aren't affected by
        `identifier_format`. */
        table_out->reset(new real_table_t(
            table_id,
            m_namespace_repo.get_namespace_interface(table_id, interruptor_on_caller),
            primary_key,
            &m_changefeed_client));

        return true;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
}

bool real_reql_cluster_interface_t::table_estimate_doc_counts(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::env_t *env,
        std::vector<int64_t> *doc_counts_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    cross_thread_signal_t interruptor_on_home(env->interruptor, home_thread());
    try {
        on_thread_t thread_switcher(home_thread());

        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);

        user_context.require_read_permission(m_rdb_context, db->id, table_id);

        table_config_and_shards_t config;
        m_table_meta_client->get_config(table_id, &interruptor_on_home, &config);

        /* Perform a distribution query against the database */
        std::map<store_key_t, int64_t> counts;
        fetch_distribution(table_id, this, &interruptor_on_home, &counts);

        /* Match the results of the distribution query against the table's shard
        boundaries */
        *doc_counts_out = std::vector<int64_t>(config.shard_scheme.num_shards(), 0);
        for (auto it = counts.begin(); it != counts.end(); ++it) {
            /* Calculate the range of shards that this key-range overlaps with */
            size_t left_shard = config.shard_scheme.find_shard_for_key(it->first);
            auto jt = it;
            ++jt;
            size_t right_shard;
            if (jt == counts.end()) {
                right_shard = config.shard_scheme.num_shards() - 1;
            } else {
                store_key_t right_key = jt->first;
                bool ok = right_key.decrement();
                guarantee(ok, "jt->first cannot be the leftmost key");
                right_shard = config.shard_scheme.find_shard_for_key(right_key);
            }
            /* We assume that every shard that this key-range overlaps with has an equal
            share of the keys in the key-range. This is shitty but oh well. */
            for (size_t shard = left_shard; shard <= right_shard; ++shard) {
                doc_counts_out->at(shard) += it->second / (right_shard - left_shard + 1);
            }
        }
        return true;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out, "", "")
}

bool real_reql_cluster_interface_t::table_config(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        admin_err_t *error_out) {
    try {
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);

        // FIXME permissions
        make_single_selection(
            admin_tables->table_config_backend[
                static_cast<int>(admin_identifier_format_t::name)].get(),
            name_string_t::guarantee_valid("table_config"), table_id, bt,
            env, selection_out);
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
}

bool real_reql_cluster_interface_t::table_status(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        admin_err_t *error_out) {
    try {
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);
        make_single_selection(
            admin_tables->table_status_backend[
                static_cast<int>(admin_identifier_format_t::name)].get(),
            name_string_t::guarantee_valid("table_status"), table_id, bt,
            env, selection_out);
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
}

bool real_reql_cluster_interface_t::table_wait(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        table_readiness_t readiness,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    try {
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);
        wait_for_table_readiness(table_id, readiness, &m_namespace_repo,
            m_table_meta_client, interruptor_on_caller);
        ql::datum_object_builder_t builder;
        builder.overwrite("ready", ql::datum_t(1.0));
        *result_out = std::move(builder).to_datum();
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
}

bool real_reql_cluster_interface_t::db_wait(
        counted_t<const ql::db_t> db,
        table_readiness_t readiness,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    std::set<namespace_id_t> table_ids;
    std::map<namespace_id_t, table_basic_config_t> tables;
    m_table_meta_client->list_names(&tables);
    for (const auto &table : tables) {
        if (table.second.database == db->id) {
            table_ids.insert(table.first);
        }
    }

    try {
        size_t count = wait_for_many_tables_readiness(table_ids, readiness,
            &m_namespace_repo, m_table_meta_client, interruptor_on_caller);
        ql::datum_object_builder_t builder;
        builder.overwrite("ready", ql::datum_t(static_cast<double>(count)));
        *result_out = std::move(builder).to_datum();
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    }
}

void real_reql_cluster_interface_t::reconfigure_internal(
        const counted_t<const ql::db_t> &db,
        const namespace_id_t &table_id,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor_on_home,
        ql::datum_t *result_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, admin_op_exc_t,
            failed_table_op_exc_t, maybe_failed_table_op_exc_t) {
    assert_thread();

    /* Fetch the table's current configuration */
    table_config_and_shards_t old_config;
    m_table_meta_client->get_config(table_id, interruptor_on_home, &old_config);

    // Store the old value of the config and status
    ql::datum_t old_config_datum = convert_table_config_to_datum(
        table_id, convert_name_to_datum(db->name), old_config.config,
        admin_identifier_format_t::name, old_config.server_names);

    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();
    ql::datum_t old_status;
    admin_err_t error;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor_on_home,
            &old_status, &error)) {
        throw admin_op_exc_t(error);
    } else if (!old_status.has()) {
        throw no_such_table_exc_t();
    }

    table_config_and_shards_t new_config;
    new_config.config.basic = old_config.config.basic;
    new_config.config.sindexes = old_config.config.sindexes;
    new_config.config.write_ack_config = old_config.config.write_ack_config;
    new_config.config.durability = old_config.config.durability;

    calculate_split_points_intelligently(
        table_id,
        this,
        params.num_shards,
        old_config.shard_scheme,
        interruptor_on_home,
        &new_config.shard_scheme);

    /* `table_generate_config()` just generates the config; it doesn't apply it */
    table_generate_config(
        m_server_config_client, table_id, m_table_meta_client,
        params, new_config.shard_scheme, interruptor_on_home, &new_config.config.shards,
        &new_config.server_names);

    if (!dry_run) {
        table_config_and_shards_change_t table_config_and_shards_change(
            table_config_and_shards_change_t::set_table_config_and_shards_t{ new_config });
        m_table_meta_client->set_config(
            table_id, table_config_and_shards_change, interruptor_on_home);
    }

    // Compute the new value of the config and status
    ql::datum_t new_config_datum = convert_table_config_to_datum(
        table_id, convert_name_to_datum(db->name), new_config.config,
        admin_identifier_format_t::name, new_config.server_names);
    ql::datum_t new_status;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor_on_home,
            &new_status, &error)) {
        throw admin_op_exc_t(error);
    } else if (!new_status.has()) {
        throw no_such_table_exc_t();
    }

    ql::datum_object_builder_t result_builder;
    if (!dry_run) {
        result_builder.overwrite("reconfigured", ql::datum_t(1.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config_datum, new_config_datum));
        result_builder.overwrite("status_changes",
            make_replacement_pair(old_status, new_status));
    } else {
        result_builder.overwrite("reconfigured", ql::datum_t(0.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config_datum, new_config_datum));
    }
    *result_out = std::move(result_builder).to_datum();
}

bool real_reql_cluster_interface_t::table_reconfigure(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    try {
        on_thread_t thread_switcher(home_thread());
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        reconfigure_internal(db, table_id, params, dry_run, &interruptor_on_home,
            result_out);
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        "The table was not reconfigured.",
        "The table may or may not have been reconfigured.")
}

bool real_reql_cluster_interface_t::db_reconfigure(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    std::map<namespace_id_t, table_basic_config_t> table_names;
    m_table_meta_client->list_names(&table_names);
    std::set<namespace_id_t> table_ids;
    for (auto const &table_name : table_names) {
        if (table_name.second.database == db->id) {
            table_ids.insert(table_name.first);
        }
    }

    user_context.require_config_permission(m_rdb_context, db->id, table_ids);

    ql::datum_t combined_stats = ql::datum_t::empty_object();
    for (const auto &table_id : table_ids) {
        ql::datum_t stats;
        try {
            reconfigure_internal(
                db, table_id, params, dry_run, &interruptor_on_home, &stats);
        } catch (const no_such_table_exc_t &) {
            /* The table got deleted during the reconfiguration. It would be weird if
            `r.db('foo').reconfigure()` produced an error complaining that some table
            `foo.bar` did not exist. So we just skip the table, as though it were
            deleted before the operation even began. */
            continue;
        } catch (const admin_op_exc_t &admin_op_exc) {
            *error_out = std::move(admin_op_exc.to_admin_err());
            return false;
        } CATCH_OP_ERRORS(db->name, table_names.at(table_id).name, error_out,
            "The tables may or may not have been reconfigured.",
            "The tables may or may not have been reconfigured.")
        std::set<std::string> dummy_conditions;
        combined_stats = combined_stats.merge(stats, &ql::stats_merge,
            ql::configured_limits_t::unlimited, &dummy_conditions);
        guarantee(dummy_conditions.empty());
    }
    *result_out = combined_stats;
    return true;
}

void real_reql_cluster_interface_t::emergency_repair_internal(
        const counted_t<const ql::db_t> &db,
        const namespace_id_t &table_id,
        emergency_repair_mode_t mode,
        bool dry_run,
        signal_t *interruptor_on_home,
        ql::datum_t *result_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t,
            failed_table_op_exc_t, maybe_failed_table_op_exc_t, admin_op_exc_t) {
    assert_thread();

    /* Fetch the table's current configuration */
    table_config_and_shards_t old_config;
    m_table_meta_client->get_config(table_id, interruptor_on_home, &old_config);

    // Store the old value of the config and status
    ql::datum_t old_config_datum = convert_table_config_to_datum(
        table_id, convert_name_to_datum(db->name), old_config.config,
        admin_identifier_format_t::name, old_config.server_names);

    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();
    ql::datum_t old_status;
    admin_err_t error;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor_on_home,
            &old_status, &error)) {
        throw admin_op_exc_t(error);
    } else if (!old_status.has()) {
        throw no_such_table_exc_t();
    }

    table_config_and_shards_t new_config;
    bool rollback_found;
    bool erase_found;
    m_table_meta_client->emergency_repair(
        table_id,
        mode,
        dry_run,
        interruptor_on_home,
        &new_config,
        &rollback_found,
        &erase_found);

    if (!rollback_found && mode != emergency_repair_mode_t::DEBUG_RECOMMIT) {
        if (!erase_found) {
            throw admin_op_exc_t("This table doesn't need to be repaired.",
                                 query_state_t::FAILED);
        } else if (erase_found &&
                mode != emergency_repair_mode_t::UNSAFE_ROLLBACK_OR_ERASE) {
            throw admin_op_exc_t(
                "One or more shards of this table have no available "
                "replicas. Since there are no available copies of the data that was "
                "stored in those shards, those shards cannot be repaired. If you run "
                "the command again with `emergency_repair` set to "
                "`unsafe_rollback_or_erase`, those shards will be reset to an empty, "
                "but writeable state; but if the missing replicas later reconnect, the "
                "original data that was stored on them will be lost permanently.",
                query_state_t::FAILED);
        }
    }

    // Compute the new value of the config and status
    ql::datum_t new_config_datum = convert_table_config_to_datum(
        table_id, convert_name_to_datum(db->name), new_config.config,
        admin_identifier_format_t::name, new_config.server_names);
    ql::datum_t new_status;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor_on_home,
            &new_status, &error)) {
        throw admin_op_exc_t(error);
    } else if (!new_status.has()) {
        throw no_such_table_exc_t();
    }

    ql::datum_object_builder_t result_builder;
    if (!dry_run) {
        result_builder.overwrite("repaired", ql::datum_t(1.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config_datum, new_config_datum));
        result_builder.overwrite("status_changes",
            make_replacement_pair(old_status, new_status));
    } else {
        result_builder.overwrite("repaired", ql::datum_t(0.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config_datum, new_config_datum));
    }
    *result_out = std::move(result_builder).to_datum();
}

bool real_reql_cluster_interface_t::table_emergency_repair(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        emergency_repair_mode_t mode,
        bool dry_run,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    try {
        on_thread_t thread_switcher(home_thread());
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        emergency_repair_internal(db, table_id, mode, dry_run,
            &interruptor_on_home, result_out);
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        /* Note the non-standard error message here, to clarify if the user tries to
        repair a table that's completely inaccessible. */
        "The table was not repaired. At least one of a table's replicas must be "
            "accessible in order to repair it.",
        "The table may or may not have been repaired.")
}

void real_reql_cluster_interface_t::rebalance_internal(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_home,
        ql::datum_t *results_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t,
            failed_table_op_exc_t, maybe_failed_table_op_exc_t, admin_op_exc_t) {
    assert_thread();

    /* Fetch the table's current configuration */
    table_config_and_shards_t config;
    m_table_meta_client->get_config(table_id, interruptor_on_home, &config);

    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();
    ql::datum_t old_status;
    admin_err_t error;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor_on_home,
            &old_status, &error)) {
        throw admin_op_exc_t(error);
    } else if (!old_status.has()) {
        throw no_such_table_exc_t();
    }

    std::map<store_key_t, int64_t> counts;
    fetch_distribution(table_id, this, interruptor_on_home, &counts);

    /* If there's not enough data to rebalance, return `rebalanced: 0` but don't report
    an error */
    bool actually_rebalanced = calculate_split_points_with_distribution(
        counts, config.config.shards.size(), &config.shard_scheme);
    if (actually_rebalanced) {
        table_config_and_shards_change_t table_config_and_shards_change(
            table_config_and_shards_change_t::set_table_config_and_shards_t{ config });
        m_table_meta_client->set_config(
            table_id, table_config_and_shards_change, interruptor_on_home);
    }

    ql::datum_t new_status;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor_on_home,
            &new_status, &error)) {
        throw admin_op_exc_t(error);
    } else if (!new_status.has()) {
        throw no_such_table_exc_t();
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("rebalanced", ql::datum_t(actually_rebalanced ? 1.0 : 0.0));
    builder.overwrite("status_changes", make_replacement_pair(old_status, new_status));
    *results_out = std::move(builder).to_datum();
}

bool real_reql_cluster_interface_t::table_rebalance(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    try {
        on_thread_t thread_switcher(home_thread());
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, name, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        rebalance_internal(table_id, &interruptor_on_home, result_out);
        return true;
    } catch (const admin_op_exc_t &admin_op_exc) {
        *error_out = std::move(admin_op_exc.to_admin_err());
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        "The table was not rebalanced.",
        "The table may or may not have been rebalanced.")
}

bool real_reql_cluster_interface_t::db_rebalance(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        signal_t *interruptor_on_caller,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    std::map<namespace_id_t, table_basic_config_t> table_names;
    m_table_meta_client->list_names(&table_names);
    std::set<namespace_id_t> table_ids;
    for (auto const &table_name : table_names) {
        if (table_name.second.database == db->id) {
            table_ids.insert(table_name.first);
        }
    }

    user_context.require_config_permission(m_rdb_context, db->id, table_ids);

    ql::datum_t combined_stats = ql::datum_t::empty_object();
    for (const auto &table_id : table_ids) {
        ql::datum_t stats;
        try {
            rebalance_internal(table_id, &interruptor_on_home, &stats);
        } catch (const no_such_table_exc_t &) {
            /* This table was deleted while we were iterating over the tables list. So
            just ignore it to avoid making a confusing error message. */
            continue;
        } catch (const admin_op_exc_t &admin_op_exc) {
            *error_out = std::move(admin_op_exc.to_admin_err());
            return false;
        } CATCH_OP_ERRORS(db->name, table_names.at(table_id).name, error_out,
            "The tables may or may not have been rebalanced.",
            "The tables may or may not have been rebalanced.")
        std::set<std::string> dummy_conditions;
        combined_stats = combined_stats.merge(stats, &ql::stats_merge,
            ql::configured_limits_t::unlimited, &dummy_conditions);
        guarantee(dummy_conditions.empty());
    }
    *result_out = combined_stats;
    return true;
}

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}

template <typename T>
bool grant_internal(
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        rdb_context_t *rdb_context,
        auth::user_context_t const &user_context,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        T permission_selector_function,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    if (!user_context.is_admin()) {
        *error_out = admin_err_t{
            "Only administrators can grant permissions.",
            query_state_t::FAILED};
        return false;
    }

    if (username.is_admin()) {
        *error_out = admin_err_t{
            "The permissions of the user `" + username.to_string() +
                "` can't be modified.",
            query_state_t::FAILED};
        return false;
    }

    auth_semilattice_metadata_t auth_metadata = auth_semilattice_view->get();
    auto grantee = auth_metadata.m_users.find(username);
    if (grantee == auth_metadata.m_users.end() ||
            !static_cast<bool>(grantee->second.get_ref())) {
        *error_out = admin_err_t{
            "User `" + username.to_string() + "` not found.", query_state_t::FAILED};
        return false;
    }

    ql::datum_t old_permissions;
    ql::datum_t new_permissions;
    try {
        grantee->second.apply_write([&](boost::optional<auth::user_t> *user) {
            auto &permissions_ref = permission_selector_function(user->get());
            old_permissions = permissions_ref.to_datum();
            permissions_ref.merge(permissions);
            new_permissions = permissions_ref.to_datum();
        });
    } catch (admin_op_exc_t const &admin_op_exc) {
        *error_out = admin_op_exc.to_admin_err();
        return false;
    }

    auth_semilattice_view->join(auth_metadata);

    // Wait for the metadata to propegate
    rdb_context->get_auth_watchable()->run_until_satisfied(
        [&](auth_semilattice_metadata_t const &metadata) -> bool {
            return is_joined(auth_metadata, metadata);
        },
        interruptor);

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("granted", ql::datum_t(1.0));
    result_builder.overwrite(
        "permissions_changes",
        make_replacement_pair(old_permissions, new_permissions));
    *result_out = std::move(result_builder).to_datum();
    return true;
}

bool real_reql_cluster_interface_t::grant_global(
        auth::user_context_t const &user_context,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    on_thread_t on_thread(home_thread());

    return grant_internal(
        m_auth_semilattice_view,
        m_rdb_context,
        user_context,
        std::move(username),
        std::move(permissions),
        interruptor,
        [](auth::user_t &user) -> auth::permissions_t & {
            return user.get_global_permissions();
        },
        result_out,
        error_out);
}

bool real_reql_cluster_interface_t::grant_database(
        auth::user_context_t const &user_context,
        database_id_t const &database_id,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(
        !database_id.is_nil(),
        "`real_reql_cluster_interface_t` should never get queries for system tables");
    on_thread_t on_thread(home_thread());

    return grant_internal(
        m_auth_semilattice_view,
        m_rdb_context,
        user_context,
        std::move(username),
        std::move(permissions),
        interruptor,
        [&](auth::user_t &user) -> auth::permissions_t & {
            return user.get_database_permissions(database_id);
        },
        result_out,
        error_out);
}

bool real_reql_cluster_interface_t::grant_table(
        auth::user_context_t const &user_context,
        database_id_t const &database_id,
        namespace_id_t const &table_id,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        ql::datum_t *result_out,
        admin_err_t *error_out) {
    guarantee(
        !database_id.is_nil() && !table_id.is_nil(),
        "`real_reql_cluster_interface_t` should never get queries for system tables");
    on_thread_t on_thread(home_thread());

    return grant_internal(
        m_auth_semilattice_view,
        m_rdb_context,
        user_context,
        std::move(username),
        std::move(permissions),
        interruptor,
        [&](auth::user_t &user) -> auth::permissions_t & {
            return user.get_table_permissions(table_id);
        },
        result_out,
        error_out);
}

bool real_reql_cluster_interface_t::sindex_create(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const sindex_config_t &config,
        signal_t *interruptor_on_caller,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    try {
        cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
        on_thread_t thread_switcher(home_thread());

        namespace_id_t table_id;
        m_table_meta_client->find(db->id, table, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        table_config_and_shards_change_t table_config_and_shards_change(
            table_config_and_shards_change_t::sindex_create_t{name, config});
        m_table_meta_client->set_config(
            table_id, table_config_and_shards_change, &interruptor_on_home);

        return true;
    } catch (const config_change_exc_t &) {
        *error_out = admin_err_t{
            strprintf("Index `%s` already exists on table `%s.%s`.",
                      name.c_str(), db->name.c_str(), table.c_str()),
            query_state_t::FAILED};
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        "The secondary index was not created.",
        "The secondary index may or may not have been created.")
}

bool real_reql_cluster_interface_t::sindex_drop(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        signal_t *interruptor_on_caller,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    try {
        cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
        on_thread_t thread_switcher(home_thread());

        namespace_id_t table_id;
        m_table_meta_client->find(db->id, table, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        table_config_and_shards_change_t table_config_and_shards_change(
            table_config_and_shards_change_t::sindex_drop_t{name});
        m_table_meta_client->set_config(
            table_id, table_config_and_shards_change, &interruptor_on_home);

        return true;
    } catch (const config_change_exc_t &) {
        *error_out = admin_err_t{
            strprintf("Index `%s` does not exist on table `%s.%s`.",
                      name.c_str(), db->name.c_str(), table.c_str()),
            query_state_t::FAILED};
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        "The secondary index was not dropped.",
        "The secondary index may or may not have been dropped.")
}

bool real_reql_cluster_interface_t::sindex_rename(
        auth::user_context_t const &user_context,
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const std::string &new_name,
        bool overwrite,
        signal_t *interruptor_on_caller,
        admin_err_t *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    try {
        cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
        on_thread_t thread_switcher(home_thread());

        namespace_id_t table_id;
        m_table_meta_client->find(db->id, table, &table_id);

        user_context.require_config_permission(m_rdb_context, db->id, table_id);

        table_config_and_shards_change_t table_config_and_shards_change(
            table_config_and_shards_change_t::sindex_rename_t{
                name, new_name, overwrite});
        m_table_meta_client->set_config(
            table_id, table_config_and_shards_change, &interruptor_on_home);

        return true;
    } catch (const config_change_exc_t &) {
        if (overwrite) {
            *error_out = admin_err_t{
                strprintf("Index `%s` does not exist on table `%s.%s`.",
                          name.c_str(), db->name.c_str(), table.c_str()),
                query_state_t::FAILED};
        } else {
            *error_out = admin_err_t{
                strprintf(
                    "Index `%s` does not exist or index `%s` already exists "
                        "on table `%s.%s`.",
                    name.c_str(), new_name.c_str(), db->name.c_str(), table.c_str()),
                query_state_t::FAILED};
        }
        return false;
    } CATCH_NAME_ERRORS(db->name, name, error_out)
      CATCH_OP_ERRORS(db->name, name, error_out,
        "The secondary index was not renamed.",
        "The secondary index may or may not have been renamed.")
}

bool real_reql_cluster_interface_t::sindex_list(
        counted_t<const ql::db_t> db,
        const name_string_t &table_name,
        signal_t *interruptor_on_caller,
        admin_err_t *error_out,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *configs_and_statuses_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    try {
        on_thread_t thread_switcher(home_thread());
        namespace_id_t table_id;
        m_table_meta_client->find(db->id, table_name, &table_id);
        m_table_meta_client->get_sindex_status(
            table_id, &interruptor_on_home, configs_and_statuses_out);
        return true;
    } CATCH_NAME_ERRORS(db->name, table_name, error_out)
      CATCH_OP_ERRORS(db->name, table_name, error_out,
        "Failed to retrieve all secondary indexes.",
        "Failed to retrieve all secondary indexes.")
}

void real_reql_cluster_interface_t::wait_for_cluster_metadata_to_propagate(
        const cluster_semilattice_metadata_t &metadata,
        signal_t *interruptor_on_caller) {
    int threadnum = get_thread_id().threadnum;

    guarantee(m_cross_thread_database_watchables[threadnum].has());
    m_cross_thread_database_watchables[threadnum]->get_watchable()->run_until_satisfied(
        [&](const databases_semilattice_metadata_t &md) -> bool {
            return is_joined(md, metadata.databases);
        },
        interruptor_on_caller);
}

template <class T>
void copy_value(const T *in, T *out) {
    *out = *in;
}

void real_reql_cluster_interface_t::get_databases_metadata(
        databases_semilattice_metadata_t *out) {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(m_cross_thread_database_watchables[threadnum].has());
    m_cross_thread_database_watchables[threadnum]->apply_read(
            std::bind(&copy_value<databases_semilattice_metadata_t>,
                      ph::_1, out));
}

void real_reql_cluster_interface_t::make_single_selection(
        artificial_table_backend_t *table_backend,
        const name_string_t &table_name,
        const uuid_u &primary_key,
        ql::backtrace_id_t bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, admin_op_exc_t) {
    ql::datum_t row;
    admin_err_t error;
    if (!table_backend->read_row(convert_uuid_to_datum(primary_key), env->interruptor,
            &row, &error)) {
        throw admin_op_exc_t(error);
    } else if (!row.has()) {
        /* This is unlikely, but it can happen if the object is deleted between when we
        look up its name and when we call `read_row()` */
        throw no_such_table_exc_t();
    }
    counted_t<ql::table_t> table = make_counted<ql::table_t>(
        counted_t<base_table_t>(new artificial_table_t(table_backend)),
        make_counted<const ql::db_t>(
            nil_uuid(), name_string_t::guarantee_valid("rethinkdb")),
        table_name.str(), read_mode_t::SINGLE, bt);
    *selection_out = make_scoped<ql::val_t>(
        ql::single_selection_t::from_row(env, bt, table, row),
        bt);
}
