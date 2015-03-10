// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/real_reql_cluster_interface.hpp"

#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/servers/config_client.hpp"
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
        mailbox_manager_t *_mailbox_manager,
        boost::shared_ptr<
            semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattices,
        rdb_context_t *_rdb_context,
        server_config_client_t *_server_config_client,
        table_meta_client_t *_table_meta_client,
        watchable_map_t<
            std::pair<peer_id_t, std::pair<namespace_id_t, branch_id_t> >,
            table_query_bcard_t> *_table_query_directory
        ) :
    mailbox_manager(_mailbox_manager),
    semilattice_root_view(_semilattices),
    table_meta_client(_table_meta_client),
    cross_thread_database_watchables(get_num_threads()),
    rdb_context(_rdb_context),
    namespace_repo(
        mailbox_manager,
        _table_query_directory,
        rdb_context),
    changefeed_client(mailbox_manager,
        [this](const namespace_id_t &id, signal_t *interruptor) {
            return this->namespace_repo.get_namespace_interface(id, interruptor);
        }),
    server_config_client(_server_config_client)
{
    for (int thr = 0; thr < get_num_threads(); ++thr) {
        cross_thread_database_watchables[thr].init(
            new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                    (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                        metadata_field(&cluster_semilattice_metadata_t::databases, semilattice_root_view))), threadnum_t(thr)));
    }
}

bool real_reql_cluster_interface_t::db_create(const name_string_t &name,
        signal_t *interruptor, ql::datum_t *result_out, std::string *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cluster_semilattice_metadata_t metadata;
    ql::datum_t new_config;
    {
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();

        /* Make sure there isn't an existing database with the same name. */
        for (const auto &pair : metadata.databases.databases) {
            if (!pair.second.is_deleted() &&
                    pair.second.get_ref().name.get_ref() == name) {
                *error_out = strprintf("Database `%s` already exists.", name.c_str());
                return false;
            }
        }

        database_id_t db_id = generate_uuid();
        database_semilattice_metadata_t db;
        db.name = versioned_t<name_string_t>(name);
        metadata.databases.databases.insert(std::make_pair(db_id, make_deletable(db)));

        semilattice_root_view->join(metadata);
        metadata = semilattice_root_view->get();

        new_config = convert_db_config_and_name_to_datum(name, db_id);
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("dbs_created", ql::datum_t(1.0));
    result_builder.overwrite("config_changes",
        make_replacement_pair(ql::datum_t::null(), new_config));
    *result_out = std::move(result_builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::db_drop(const name_string_t &name,
        signal_t *interruptor, ql::datum_t *result_out, std::string *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cluster_semilattice_metadata_t metadata;
    ql::datum_t old_config;
    size_t tables_dropped;
    {
        cross_thread_signal_t interruptor2(interruptor,
            semilattice_root_view->home_thread());
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();
        database_id_t db_id;
        if (!search_db_metadata_by_name(metadata.databases, name, &db_id, error_out)) {
            return false;
        }

        old_config = convert_db_config_and_name_to_datum(name, db_id);

        metadata.databases.databases.at(db_id).mark_deleted();

        semilattice_root_view->join(metadata);
        metadata = semilattice_root_view->get();

        /* Delete all of the tables in the database */
        std::map<namespace_id_t, std::pair<database_id_t, name_string_t> > tables;
        table_meta_client->list_names(&tables);
        tables_dropped = 0;
        for (const auto &pair : tables) {
            if (pair.second.first == db_id) {
                /* Note that we silently ignore failures in this function. This is
                consistent with our behavior if we cannot "see" the table at all. */
                if (table_meta_client->drop(pair.first, &interruptor2)) {
                    ++tables_dropped;
                }
            }
        }
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("dbs_dropped", ql::datum_t(1.0));
    result_builder.overwrite("tables_dropped",
        ql::datum_t(static_cast<double>(tables_dropped)));
    result_builder.overwrite("config_changes",
        make_replacement_pair(old_config, ql::datum_t::null()));
    *result_out = std::move(result_builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::db_list(
        UNUSED signal_t *interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    for (const auto &pair : db_metadata.databases) {
        if (!pair.second.is_deleted()) {
            names_out->insert(pair.second.get_ref().name.get_ref());
        }
    }
    return true;
}

bool real_reql_cluster_interface_t::db_find(const name_string_t &name,
        UNUSED signal_t *interruptor, counted_t<const ql::db_t> *db_out,
        std::string *error_out) {
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
        const ql::protob_t<const Backtrace> &bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    return make_single_selection(admin_tables->db_config_backend.get(),
        name_string_t::guarantee_valid("db_config"), db->id, bt,
        strprintf("Database `%s` does not exist.", db->name.c_str()), env,
        selection_out, error_out);
}

bool real_reql_cluster_interface_t::table_create(const name_string_t &name,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &config_params,
        const std::string &primary_key,
        write_durability_t durability,
        signal_t *interruptor, ql::datum_t *result_out, std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    namespace_id_t table_id;
    cluster_semilattice_metadata_t metadata;
    ql::datum_t new_config;
    {
        cross_thread_signal_t interruptor2(interruptor,
            semilattice_root_view->home_thread());
        on_thread_t thread_switcher(semilattice_root_view->home_thread());

        /* Make sure there isn't an existing table with the same name */
        {
            namespace_id_t dummy_table_id;
            if (table_meta_client->find(db->id, name, &dummy_table_id) !=
                    table_meta_client_t::find_res_t::none) {
                *error_out = strprintf("Table `%s.%s` already exists.", db->name.c_str(),
                    name.c_str());
                return false;
            }
        }

        table_config_and_shards_t config;

        config.config.name = name;
        config.config.database = db->id;
        config.config.primary_key = primary_key;

        /* We don't have any data to generate split points based on, so assume UUIDs */
        calculate_split_points_for_uuids(
            config_params.num_shards, &config.shard_scheme);

        /* Pick which servers to host the data */
        std::map<server_id_t, int> server_usage;
        // RSI(raft): Fill in `server_usage` so we make smarter choices
        if (!table_generate_config(
                server_config_client, nil_uuid(), nullptr, server_usage,
                config_params, config.shard_scheme, &interruptor2,
                &config.config.shards, error_out)) {
            *error_out = "When generating configuration for new table: " + *error_out;
            return false;
        }

        config.config.write_ack_config.mode = write_ack_config_t::mode_t::majority;
        config.config.durability = durability;

        table_id = generate_uuid();
        if (!table_meta_client->create(table_id, config, &interruptor2)) {
            *error_out = "Lost contact with the server(s) that were supposed to host "
                "the newly-created table. The table may or may not have been created.";
            return false;
        }

        new_config = convert_table_config_to_datum(table_id,
            convert_name_to_datum(db->name), config.config,
            admin_identifier_format_t::name, server_config_client);
    }

    // RSI(raft): Wait for table to become available to handle queries

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("tables_created", ql::datum_t(1.0));
    result_builder.overwrite("config_changes",
        make_replacement_pair(ql::datum_t::null(), new_config));
    *result_out = std::move(result_builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::table_drop(const name_string_t &name,
        counted_t<const ql::db_t> db, signal_t *interruptor, ql::datum_t *result_out,
        std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cluster_semilattice_metadata_t metadata;
    ql::datum_t old_config;
    {
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();

        namespace_id_t table_id;
        if (!find_table(db, name, &table_id, nullptr, error_out)) {
            return false;
        }

        table_config_and_shards_t config;
        if (!table_meta_client->get_config(table_id, interruptor, &config)) {
            /* In this situation we actually know for sure that we did not delete the
            table. But we show the same message as below for consistency. */
            *error_out = strprintf("Lost contact with the server(s) hosting table "
                "`%s.%s`. The table may or may not have been deleted.", db->name.c_str(),
                name.c_str());
            return false;
        }
        old_config = convert_table_config_to_datum(table_id,
            convert_name_to_datum(db->name), config.config,
            admin_identifier_format_t::name, server_config_client);

        if (!table_meta_client->drop(table_id, interruptor)) {
            *error_out = strprintf("Lost contact with the server(s) hosting table "
                "`%s.%s`. The table may or may not have been deleted.", db->name.c_str(),
                name.c_str());
            return false;
        }
    }

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("tables_dropped", ql::datum_t(1.0));
    result_builder.overwrite("config_changes",
        make_replacement_pair(old_config, ql::datum_t::null()));
    *result_out = std::move(result_builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::table_list(counted_t<const ql::db_t> db,
        UNUSED signal_t *interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    std::map<namespace_id_t, std::pair<database_id_t, name_string_t> > tables;
    table_meta_client->list_names(&tables);
    for (const auto &pair : tables) {
        if (pair.second.first == db->id) {
            names_out->insert(pair.second.second);
        }
    }
    return true;
}

bool real_reql_cluster_interface_t::table_find(
        const name_string_t &name, counted_t<const ql::db_t> db,
        UNUSED boost::optional<admin_identifier_format_t> identifier_format,
        signal_t *interruptor,
        counted_t<base_table_t> *table_out, std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    namespace_id_t table_id;
    std::string primary_key;
    if (!find_table(db, name, &table_id, &primary_key, error_out)) {
        return false;
    }

    /* Note that we completely ignore `identifier_format`. `identifier_format` is
    meaningless for real tables, so it might seem like we should produce an error. The
    reason we don't is that the user might write a query that access both a system table
    and a real table, and they might specify `identifier_format` as a global optarg.
    So then they would get a spurious error for the real table. This behavior is also
    consistent with that of system tables that aren't affected by `identifier_format`. */
    table_out->reset(new real_table_t(
        table_id,
        namespace_repo.get_namespace_interface(table_id, interruptor),
        primary_key,
        &changefeed_client));

    return true;
}

bool real_reql_cluster_interface_t::table_estimate_doc_counts(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::env_t *env,
        std::vector<int64_t> *doc_counts_out,
        std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t interruptor2(env->interruptor,
        semilattice_root_view->home_thread());
    on_thread_t thread_switcher(semilattice_root_view->home_thread());

    // RSI(raft): Reimplement this once table IO works
    not_implemented();
    (void)db;
    (void)name;
    (void)env;
    (void)doc_counts_out;
    (void)error_out;
#if 0
    /* Find the specified table in the semilattice metadata */
    cluster_semilattice_metadata_t metadata = semilattice_root_view->get();
    namespace_id_t table_id;
    if (!search_table_metadata_by_name(*metadata.rdb_namespaces, db->id, db->name, name,
            &table_id, error_out)) {
        return false;
    }

    /* Perform a distribution query against the database */
    namespace_interface_access_t ns_if_access =
        namespace_repo.get_namespace_interface(table_id, &interruptor2);
    static const int depth = 2;
    static const int limit = 128;
    distribution_read_t inner_read(depth, limit);
    read_t read(inner_read, profile_bool_t::DONT_PROFILE);
    read_response_t resp;
    try {
        ns_if_access.get()->read_outdated(read, &resp, &interruptor2);
    } catch (const cannot_perform_query_exc_t &exc) {
        *error_out = "Could not estimate document counts because table is not available "
            "for reading: " + std::string(exc.what());
        return false;
    }
    const std::map<store_key_t, int64_t> &counts =
        boost::get<distribution_read_response_t>(resp.response).key_counts;

    /* Match the results of the distribution query against the table's shard boundaries
    */
    const table_shard_scheme_t &shard_scheme = metadata.rdb_namespaces->namespaces
        .at(table_id).get_ref().replication_info.get_ref().shard_scheme;
    *doc_counts_out = std::vector<int64_t>(shard_scheme.num_shards(), 0);
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        /* Calculate the range of shards that this key-range overlaps with */
        size_t left_shard = shard_scheme.find_shard_for_key(it->first);
        auto jt = it;
        ++jt;
        size_t right_shard;
        if (jt == counts.end()) {
            right_shard = shard_scheme.num_shards() - 1;
        } else {
            store_key_t right_key = jt->first;
            bool ok = right_key.decrement();
            guarantee(ok, "jt->first cannot be the leftmost key");
            right_shard = shard_scheme.find_shard_for_key(right_key);
        }
        /* We assume that every shard that this key-range overlaps with has an equal
        share of the keys in the key-range. This is shitty but oh well. */
        for (size_t shard = left_shard; shard <= right_shard; ++shard) {
            doc_counts_out->at(shard) += it->second / (right_shard - left_shard + 1);
        }
    }
#endif
    return true;
}

bool real_reql_cluster_interface_t::table_config(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const ql::protob_t<const Backtrace> &bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    namespace_id_t table_id;
    if (!find_table(db, name, &table_id, nullptr, error_out)) {
        return false;
    }
    return make_single_selection(
        admin_tables->table_config_backend[
            static_cast<int>(admin_identifier_format_t::name)].get(),
        name_string_t::guarantee_valid("table_config"), table_id, bt,
        strprintf("Table `%s.%s` does not exist.", db->name.c_str(), name.c_str()),
        env, selection_out, error_out);
}

bool real_reql_cluster_interface_t::table_status(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const ql::protob_t<const Backtrace> &bt,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)db;
    (void)name;
    (void)bt;
    (void)env;
    (void)selection_out;
    (void)error_out;
    return false;
#if 0
    namespace_id_t table_id;
    if (!search_table_metadata_by_name(*get_namespaces_metadata(), db->id, db->name,
            name, &table_id, error_out)) {
        return false;
    }
    return make_single_selection(
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get(),
        name_string_t::guarantee_valid("table_status"), table_id, bt,
        strprintf("Table `%s.%s` does not exist.", db->name.c_str(), name.c_str()),
        env, selection_out, error_out);
#endif
}

/* Waits until all of the tables listed in `tables` are ready to the given level of
readiness, or have been deleted. */
bool real_reql_cluster_interface_t::wait_internal(
        std::set<namespace_id_t> tables,
        table_readiness_t readiness,
        signal_t *interruptor,
        ql::datum_t *result_out,
        int *count_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this oncee table meta operations work
    not_implemented();
    (void)tables;
    (void)readiness;
    (void)interruptor;
    (void)result_out;
    (void)count_out;
    (void)error_out;
#if 0
    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();

    /* Record the old values of `table_status` so we can report them in the result */
    std::map<namespace_id_t, ql::datum_t> old_statuses;
    for (auto it = tables.begin(); it != tables.end();) {
        ql::datum_t status;
        if (!status_backend->read_row(convert_uuid_to_datum(*it), interruptor, &status,
                error_out)) {
            return false;
        }
        if (!status.has()) {
            /* The table was deleted; remove it from the set */
            auto jt = it;
            ++jt;
            tables.erase(it);
            it = jt;
        } else {
            old_statuses[*it] = status;
            ++it;
        }
    }

    std::map<namespace_id_t, ql::datum_t> new_statuses;
    /* We alternate between two checks for readiness: waiting on `table_status_backend`,
    and running test queries. We consider ourselves done when both tests succeed for
    every table with no failures in between. */
    while (true) {
        /* First we wait until all the `table_status_backend` checks succeed in a row */ 
        {
            threadnum_t new_thread = status_backend->home_thread();
            cross_thread_signal_t ct_interruptor(interruptor, new_thread);
            on_thread_t thread_switcher(new_thread);

            // Loop until all tables are ready - we have to check all tables again
            // if a table was not immediately ready, because we're supposed to
            // wait until all the checks succeed with no failures in between.
            bool immediate;
            do {
                new_statuses.clear();
                immediate = true;
                for (auto it = tables.begin(); it != tables.end();) {
                    ql::datum_t status;
                    table_wait_result_t res = wait_for_table_readiness(
                        *it, readiness, status_backend, &ct_interruptor, &status);
                    if (res == table_wait_result_t::DELETED) {
                        /* Remove this entry so we don't keep trying to wait on it after
                        it's been erased */
                        tables.erase(it++);
                    } else {
                        new_statuses[*it] = status;
                        ++it;
                    }
                    immediate = immediate && (res == table_wait_result_t::IMMEDIATE);
                }
            } while (!immediate && tables.size() != 1);
        }

        /* Next we check that test queries succeed. */
        // We cannot wait for anything higher than 'writes' through namespace_interface_t
        table_readiness_t readiness2 = std::min(readiness, table_readiness_t::writes);
        bool success = true;
        for (auto it = tables.begin(); it != tables.end() && success; ++it) {
            namespace_interface_access_t ns_if =
                namespace_repo.get_namespace_interface(*it, interruptor);
            success = success && ns_if.get()->check_readiness(readiness2, interruptor);
        }
        if (success) {
            break;
        }

        /* The `table_status` check succeeded, but the test query didn't. Go back and try
        the `table_status` check again, after waiting for a bit. */
        nap(100, interruptor);
    }

    guarantee(new_statuses.size() == tables.size());

    ql::datum_object_builder_t result_builder;
    result_builder.overwrite("ready", ql::datum_t(static_cast<double>(tables.size())));
    ql::datum_array_builder_t status_changes_builder(ql::configured_limits_t::unlimited);
    for (const namespace_id_t &table_id : tables) {
        ql::datum_object_builder_t change_builder;
        change_builder.overwrite("old_val", old_statuses.at(table_id));
        change_builder.overwrite("new_val", new_statuses.at(table_id));
        status_changes_builder.add(std::move(change_builder).to_datum());
    }
    result_builder.overwrite("status_changes",
        std::move(status_changes_builder).to_datum());
    *result_out = std::move(result_builder).to_datum();
    *count_out = tables.size();
#endif
    return true;
}

bool real_reql_cluster_interface_t::table_wait(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        table_readiness_t readiness,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)db;
    (void)name;
    (void)readiness;
    (void)interruptor;
    (void)result_out;
    (void)error_out;
#if 0
    namespace_id_t table_id;
    if (!search_table_metadata_by_name(*get_namespaces_metadata(), db->id, db->name,
            name, &table_id, error_out)) {
        return false;
    }

    std::set<namespace_id_t> table_ids;
    table_ids.insert(table_id);
    int num_waited;
    if (!wait_internal(table_ids, readiness, interruptor,
            result_out, &num_waited, error_out)) {
        return false;
    }

    /* If the table is deleted, then `wait_internal()` will just return normally. This
    behavior makes sense for `db_wait()` but not for `table_wait()`. So we manually check
    to see if the wait succeeded. */
    if (num_waited != 1) {
        *error_out = strprintf("Table `%s.%s` does not exist.",
            db->name.c_str(), name.c_str());
        return false;
    }
#endif
    return true;
}

bool real_reql_cluster_interface_t::db_wait(
        counted_t<const ql::db_t> db,
        table_readiness_t readiness,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)db;
    (void)readiness;
    (void)interruptor;
    (void)result_out;
    (void)error_out;
    return false;
#if 0
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");

    cow_ptr_t<namespaces_semilattice_metadata_t> ns_metadata = get_namespaces_metadata();
    std::set<namespace_id_t> table_ids;
    for (const auto &pair : ns_metadata->namespaces) {
        if (!pair.second.is_deleted() &&
                pair.second.get_ref().database.get_ref() == db->id) {
            table_ids.insert(pair.first);
        }
    }

    int dummy_num_waited;
    return wait_internal(table_ids, readiness, interruptor,
        result_out, &dummy_num_waited, error_out);
#endif
}

bool real_reql_cluster_interface_t::reconfigure_internal(
        const counted_t<const ql::db_t> &db,
        const namespace_id_t &table_id,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    rassert(get_thread_id() == server_config_client->home_thread());

    /* Fetch the table's current configuration */
    table_config_and_shards_t old_config;
    if (!table_meta_client->get_config(table_id, interruptor, &old_config)) {
        *error_out = "Lost contact with the server(s) that were hosting the table being "
            "reconfigured. The table was not reconfigured.";
        return false;
    }

    // Store the old value of the config and status
    ql::datum_t old_config_datum = convert_table_config_to_datum(
        table_id, convert_name_to_datum(db->name), old_config.config,
        admin_identifier_format_t::name, server_config_client);

    /* RSI(raft): Reimplement this once `table_status` is implemented */
    ql::datum_t old_status_datum("this is a fake status");
#if 0
    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();
    ql::datum_t old_status;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor,
            &old_status, error_out)) {
        return false;
    }
#endif

    table_config_and_shards_t new_config;
    new_config.config.name = old_config.config.name;
    new_config.config.database = old_config.config.database;
    new_config.config.primary_key = old_config.config.primary_key;

    if (!calculate_split_points_intelligently(
            table_id,
            this,
            params.num_shards,
            old_config.shard_scheme,
            interruptor,
            &new_config.shard_scheme,
            error_out)) {
        return false;
    }

    /* RSI(raft): Fill in `server_usage` so we make smarter choices */
    std::map<server_id_t, int> server_usage;

    /* `table_generate_config()` just generates the config; it doesn't apply it */
    if (!table_generate_config(
            server_config_client, table_id, table_meta_client, server_usage,
            params, new_config.shard_scheme, interruptor, &new_config.config.shards,
            error_out)) {
        *error_out = "When generating new configuration for table: " + *error_out;
        return false;
    }

    new_config.config.write_ack_config.mode = write_ack_config_t::mode_t::majority;
    new_config.config.durability = write_durability_t::HARD;

    if (!dry_run) {
        if (!table_meta_client->set_config(table_id, new_config, interruptor)) {
            *error_out = "Lost contact with the server(s) that were hosting the table "
                "being reconfigured. The table may or may not have been reconfigured.";
            return false;
        }
    }

    // Compute the new value of the config and status
    ql::datum_t new_config_datum = convert_table_config_to_datum(
        table_id, convert_name_to_datum(db->name), new_config.config,
        admin_identifier_format_t::name, server_config_client);

    /* RSI(raft): Reimplement this once `table_status` is implemented */
    ql::datum_t new_status_datum("this is a fake status");
#if 0
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor,
            &new_status, error_out)) {
        return false;
    }
#endif

    ql::datum_object_builder_t result_builder;
    if (!dry_run) {
        result_builder.overwrite("reconfigured", ql::datum_t(1.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config_datum, new_config_datum));
        result_builder.overwrite("status_changes",
            make_replacement_pair(old_status_datum, new_status_datum));
    } else {
        result_builder.overwrite("reconfigured", ql::datum_t(0.0));
        result_builder.overwrite("config_changes",
            make_replacement_pair(old_config_datum, new_config_datum));
    }
    *result_out = std::move(result_builder).to_datum();
    return true;
}

bool real_reql_cluster_interface_t::table_reconfigure(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());
    namespace_id_t table_id;
    if (!find_table(db, name, &table_id, nullptr, error_out)) {
        return false;
    }
    return reconfigure_internal(db, table_id, params, dry_run,
                                &ct_interruptor, result_out, error_out);
}

bool real_reql_cluster_interface_t::db_reconfigure(
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)db;
    (void)params;
    (void)dry_run;
    (void)interruptor;
    (void)result_out;
    (void)error_out;
    return false;
#if 0
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());
    cluster_semilattice_metadata_t cluster_md = semilattice_root_view->get();
    ql::datum_t combined_stats = ql::datum_t::empty_object();
    cow_ptr_t<namespaces_semilattice_metadata_t> namespaces_copy =
        cluster_md.rdb_namespaces;
    for (const auto &pair : namespaces_copy->namespaces) {
        if (!pair.second.is_deleted() &&
                pair.second.get_ref().database.get_ref() == db->id) {
            ql::datum_t stats;
            if (!reconfigure_internal(&cluster_md, db, pair.first,
                    pair.second.get_ref().name.get_ref(), params, dry_run,
                    &ct_interruptor, &stats, error_out)) {
                return false;
            }
            std::set<std::string> dummy_conditions;
            combined_stats = combined_stats.merge(stats, &ql::stats_merge,
                ql::configured_limits_t::unlimited, &dummy_conditions);
            guarantee(dummy_conditions.empty());
        }
    }
    *result_out = combined_stats;
    return true;
#endif
}

bool real_reql_cluster_interface_t::rebalance_internal(
        cluster_semilattice_metadata_t *cluster_md,
        const counted_t<const ql::db_t> &db,
        const namespace_id_t &table_id,
        const name_string_t &table_name,
        signal_t *interruptor,
        ql::datum_t *results_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)cluster_md;
    (void)db;
    (void)table_id;
    (void)table_name;
    (void)interruptor;
    (void)results_out;
    (void)error_out;
    return false;
#if 0
    /* Store old status */
    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();
    ql::datum_t old_status;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor,
            &old_status, error_out)) {
        return false;
    }

    /* Find the specified table in the semilattice metadata */
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &cluster_md->rdb_namespaces);
    namespace_semilattice_metadata_t *table_md =
        ns_change.get()->namespaces.at(table_id).get_mutable();

    std::map<store_key_t, int64_t> counts;
    if (!fetch_distribution(table_id, this, interruptor, &counts, error_out)) {
        *error_out = strprintf("When measuring document distribution for table "
            "`%s.%s`: %s", db->name.c_str(), table_name.c_str(), error_out->c_str());
        return false;
    }

    ql::datum_object_builder_t builder;
    table_replication_info_t new_repli_info = table_md->replication_info.get_ref();
    // If there's not enough data to rebalance, just pretend like we did
    if (!calculate_split_points_with_distribution(
            counts,
            new_repli_info.config.shards.size(),
            &new_repli_info.shard_scheme,
            error_out)) {
        builder.overwrite("rebalanced", ql::datum_t(0.0));
    } else {
        builder.overwrite("rebalanced", ql::datum_t(1.0));
        table_md->replication_info.set(new_repli_info);
        semilattice_root_view->join(*cluster_md);
    }

    /* Calculate new status */
    ql::datum_t new_status;
    if (!status_backend->read_row(convert_uuid_to_datum(table_id), interruptor,
            &new_status, error_out)) {
        return false;
    }

    builder.overwrite("status_changes", make_replacement_pair(old_status, new_status));
    *results_out = std::move(builder).to_datum();

    return true;
#endif
}

bool real_reql_cluster_interface_t::table_rebalance(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)db;
    (void)name;
    (void)interruptor;
    (void)result_out;
    (void)error_out;
    return false;
#if 0
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());
    cluster_semilattice_metadata_t cluster_md = semilattice_root_view->get();
    namespace_id_t table_id;
    if (!search_table_metadata_by_name(*cluster_md.rdb_namespaces, db->id, db->name,
            name, &table_id, error_out)) {
        return false;
    }
    return rebalance_internal(&cluster_md, db, table_id, name, &ct_interruptor,
                              result_out, error_out);
#endif
}

bool real_reql_cluster_interface_t::db_rebalance(
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    // RSI(raft): Reimplement this once table meta operations work
    not_implemented();
    (void)db;
    (void)interruptor;
    (void)result_out;
    (void)error_out;
    return false;
#if 0
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());
    cluster_semilattice_metadata_t cluster_md = semilattice_root_view->get();
    ql::datum_t combined_stats = ql::datum_t::empty_object();
    cow_ptr_t<namespaces_semilattice_metadata_t> namespaces_copy =
        cluster_md.rdb_namespaces;
    for (const auto &pair : namespaces_copy->namespaces) {
        if (!pair.second.is_deleted() &&
                pair.second.get_ref().database.get_ref() == db->id) {
            ql::datum_t stats;
            if (!rebalance_internal(&cluster_md, db, pair.first,
                    pair.second.get_ref().name.get_ref(),
                    &ct_interruptor, &stats, error_out)) {
                return false;
            }
            std::set<std::string> dummy_conditions;
            combined_stats = combined_stats.merge(stats, &ql::stats_merge,
                ql::configured_limits_t::unlimited, &dummy_conditions);
            guarantee(dummy_conditions.empty());
        }
    }
    *result_out = combined_stats;
    return true;
#endif
}

bool real_reql_cluster_interface_t::sindex_change_internal(
        const counted_t<const ql::db_t> &db,
        const name_string_t &table_name,
        const std::function<bool(std::map<std::string, sindex_config_t> *)> &cb,
        signal_t *interruptor,
        std::string *error_out) {
    guarantee(db->name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());
    namespace_id_t table_id;
    if (!find_table(db, table_name, &table_id, nullptr, error_out)) {
        return false;
    }
    table_config_and_shards_t config;
    if (!table_meta_client->get_config(table_id, interruptor, &config)) {
        *error_out = strprintf("Lost contact with the server(s) hosting table `%s.%s`. "
            "The sindexes were not changed.", db->name.c_str(), table_name.c_str());
        return false;
    }
    if (!cb(&config.config.sindexes)) {
        return false;
    }
    if (!table_meta_client->set_config(table_id, config, interruptor)) {
        *error_out = strprintf("Lost contact with the server(s) hosting table `%s.%s`. "
            "The sindexes may or may not have been changed.", db->name.c_str(),
            table_name.c_str());
        return false;
    }
    return true;
}

bool real_reql_cluster_interface_t::sindex_create(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const sindex_config_t &config,
        signal_t *interruptor,
        std::string *error_out) {
    return sindex_change_internal(
        db, table,
        [&](std::map<std::string, sindex_config_t> *map) -> bool {
            if (map->count(name) == 1) {
                *error_out = strprintf("Index `%s` already exists on table `%s.%s`.",
                    name.c_str(), db->name.c_str(), table.c_str());
                return false;
            }
            map->insert(std::make_pair(name, config));
            return true;
        },
        interruptor, error_out);
}

bool real_reql_cluster_interface_t::sindex_drop(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        signal_t *interruptor,
        std::string *error_out) {
    return sindex_change_internal(
        db, table,
        [&](std::map<std::string, sindex_config_t> *map) -> bool {
            if (map->count(name) == 0) {
                *error_out = strprintf("Index `%s` does not exist on table `%s.%s`.",
                    name.c_str(), db->name.c_str(), table.c_str());
                return false;
            }
            map->erase(name);
            return true;
        },
        interruptor, error_out);
}

bool real_reql_cluster_interface_t::sindex_rename(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        const std::string &name,
        const std::string &new_name,
        bool overwrite,
        signal_t *interruptor,
        std::string *error_out) {
    return sindex_change_internal(
        db, table,
        [&](std::map<std::string, sindex_config_t> *map) -> bool {
            if (map->count(name) == 0) {
                *error_out = strprintf("Index `%s` does not exist on table `%s.%s`.",
                    name.c_str(), db->name.c_str(), table.c_str());
                return false;
            }
            if (map->count(new_name) == 1) {
                if (overwrite) {
                    map->erase(new_name);
                } else {
                    *error_out = strprintf("Index `%s` already exists on table `%s.%s`.",
                        name.c_str(), db->name.c_str(), table.c_str());
                    return false;
                }
            }
            sindex_config_t config = map->at(name);
            map->erase(name);
            map->insert(std::make_pair(name, config));
            return true;
        },
        interruptor, error_out);
}

bool real_reql_cluster_interface_t::sindex_list(
        counted_t<const ql::db_t> db,
        const name_string_t &table,
        signal_t *interruptor,
        std::string *error_out,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *configs_and_statuses_out) {
    (void)db;
    (void)table;
    (void)interruptor;
    (void)configs_and_statuses_out;
    *error_out = "This isn't implemented yet.";
    return false;
}

bool real_reql_cluster_interface_t::find_table(
        const counted_t<const ql::db_t> &db,
        const name_string_t &name,
        namespace_id_t *table_id_out,
        std::string *primary_key_out,
        std::string *error_out) {
    table_meta_client_t::find_res_t res =
        table_meta_client->find(db->id, name, table_id_out, primary_key_out);
    if (res == table_meta_client_t::find_res_t::none) {
        *error_out = strprintf("Table `%s.%s` does not exist.", db->name.c_str(),
            name.c_str());
        return false;
    } else if (res == table_meta_client_t::find_res_t::multiple) {
        *error_out = strprintf("Table `%s.%s` is ambiguous; there are multiple "
            "tables with that name.", db->name.c_str(), name.c_str());
        return false;
    } else {
        return true;
    }
}

/* Checks that divisor is indeed a divisor of multiple. */
template <class T>
bool is_joined(const T &multiple, const T &divisor) {
    T cpy = multiple;

    semilattice_join(&cpy, divisor);
    return cpy == multiple;
}

void real_reql_cluster_interface_t::wait_for_metadata_to_propagate(
        const cluster_semilattice_metadata_t &metadata, signal_t *interruptor) {
    int threadnum = get_thread_id().threadnum;

    guarantee(cross_thread_database_watchables[threadnum].has());
    cross_thread_database_watchables[threadnum]->get_watchable()->run_until_satisfied(
            [&] (const databases_semilattice_metadata_t &md) -> bool
                { return is_joined(md, metadata.databases); },
            interruptor);
}

template <class T>
void copy_value(const T *in, T *out) {
    *out = *in;
}

void real_reql_cluster_interface_t::get_databases_metadata(
        databases_semilattice_metadata_t *out) {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_database_watchables[threadnum].has());
    cross_thread_database_watchables[threadnum]->apply_read(
            std::bind(&copy_value<databases_semilattice_metadata_t>,
                      ph::_1, out));
}

bool real_reql_cluster_interface_t::make_single_selection(
        artificial_table_backend_t *table_backend,
        const name_string_t &table_name,
        const uuid_u &primary_key,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &msg_if_not_found,
        ql::env_t *env,
        scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    ql::datum_t row;
    if (!table_backend->read_row(convert_uuid_to_datum(primary_key), env->interruptor,
            &row, error_out)) {
        return false;
    }
    if (!row.has()) {
        /* This is unlikely, but it can happen if the object is deleted between when we
        look up its name and when we call `read_row()` */
        *error_out = msg_if_not_found;
        return false;
    }
    counted_t<ql::table_t> table = make_counted<ql::table_t>(
        counted_t<base_table_t>(new artificial_table_t(table_backend)),
        make_counted<const ql::db_t>(
            nil_uuid(), name_string_t::guarantee_valid("rethinkdb")),
        table_name.str(), false, bt);
    *selection_out = make_scoped<ql::val_t>(
        ql::single_selection_t::from_row(env, bt, table, row),
        bt);
    return true;
}

