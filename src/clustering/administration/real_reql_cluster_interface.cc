// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/real_reql_cluster_interface.hpp"

#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/reactor_driver.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/administration/tables/generate_config.hpp"
#include "clustering/administration/tables/split_points.hpp"
#include "clustering/administration/tables/table_config.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/artificial_table/artificial_table.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/val.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "rpc/semilattice/view/field.hpp"

#define NAMESPACE_INTERFACE_EXPIRATION_MS (60 * 1000)

counted_t<ql::table_t> make_table_with_backend(
        artificial_table_backend_t *backend,
        const char *table_name,
        const ql::protob_t<const Backtrace> &bt) {
    return make_counted<ql::table_t>(
        counted_t<base_table_t>(new artificial_table_t(backend)),
        make_counted<const ql::db_t>(nil_uuid(), "rethinkdb"),
        table_name, false, bt);
}

real_reql_cluster_interface_t::real_reql_cluster_interface_t(
        mailbox_manager_t *_mailbox_manager,
        boost::shared_ptr<
            semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattices,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                        namespace_directory_metadata_t> *_directory_root_view,
        rdb_context_t *_rdb_context,
        server_config_client_t *_server_config_client
        ) :
    mailbox_manager(_mailbox_manager),
    semilattice_root_view(_semilattices),
    directory_root_view(_directory_root_view),
    cross_thread_namespace_watchables(get_num_threads()),
    cross_thread_database_watchables(get_num_threads()),
    rdb_context(_rdb_context),
    namespace_repo(
        mailbox_manager,
        metadata_field(
            &cluster_semilattice_metadata_t::rdb_namespaces, semilattice_root_view),
        directory_root_view,
        rdb_context),
    changefeed_client(mailbox_manager,
        [this](const namespace_id_t &id, signal_t *interruptor) {
            return this->namespace_repo.get_namespace_interface(id, interruptor);
        }),
    server_config_client(_server_config_client)
{
    for (int thr = 0; thr < get_num_threads(); ++thr) {
        cross_thread_namespace_watchables[thr].init(
            new cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(
                clone_ptr_t<semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
                    (new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(
                        metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, semilattice_root_view))), threadnum_t(thr)));

        cross_thread_database_watchables[thr].init(
            new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                    (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                        metadata_field(&cluster_semilattice_metadata_t::databases, semilattice_root_view))), threadnum_t(thr)));
    }
}

bool real_reql_cluster_interface_t::db_create(const name_string_t &name,
        signal_t *interruptor, std::string *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();
        metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
            &metadata.databases.databases);

        metadata_search_status_t status;
        db_searcher.find_uniq(name, &status);
        if (!check_metadata_status(status, "Database", name.str(), false, error_out)) {
            return false;
        }

        database_semilattice_metadata_t db;
        db.name = versioned_t<name_string_t>(name);
        metadata.databases.databases.insert(
            std::make_pair(generate_uuid(), make_deletable(db)));

        semilattice_root_view->join(metadata);
        metadata = semilattice_root_view->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool real_reql_cluster_interface_t::db_drop(const name_string_t &name,
        signal_t *interruptor, std::string *error_out) {
    guarantee(name != name_string_t::guarantee_valid("rethinkdb"),
        "real_reql_cluster_interface_t should never get queries for system tables");
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();
        metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
            &metadata.databases.databases);

        metadata_search_status_t status;
        auto it = db_searcher.find_uniq(name, &status);
        if (!check_metadata_status(status, "Database", name.str(), true, error_out)) {
            return false;
        }

        /* Delete the database */
        deletable_t<database_semilattice_metadata_t> *db_metadata = &it->second;
        guarantee(!db_metadata->is_deleted());
        db_metadata->mark_deleted();

        /* Delete all of the tables in the database */
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &metadata.rdb_namespaces);
        metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &ns_change.get()->namespaces);
        namespace_predicate_t pred(&it->first);
        for (auto it2 = ns_searcher.find_next(ns_searcher.begin(), pred);
                  it2 != ns_searcher.end();
                  it2 = ns_searcher.find_next(++it2, pred)) {
            guarantee(!it2->second.is_deleted());
            it2->second.mark_deleted();
        }

        semilattice_root_view->join(metadata);
        metadata = semilattice_root_view->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool real_reql_cluster_interface_t::db_list(
        UNUSED signal_t *interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    const_metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
        &db_metadata.databases);
    for (auto it = db_searcher.find_next(db_searcher.begin());
              it != db_searcher.end();
              it = db_searcher.find_next(++it)) {
        guarantee(!it->second.is_deleted());
        names_out->insert(it->second.get_ref().name.get_ref());
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
    const_metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
        &db_metadata.databases);
    metadata_search_status_t status;
    auto it = db_searcher.find_uniq(name, &status);
    if (!check_metadata_status(status, "Database", name.str(), true, error_out)) {
        return false;
    }
    *db_out = make_counted<const ql::db_t>(it->first, name.str());
    return true;
}

bool real_reql_cluster_interface_t::db_config(
        const std::vector<name_string_t> &db_names,
        const ql::protob_t<const Backtrace> &bt, signal_t *interruptor,
        scoped_ptr_t<ql::val_t> *resp_out, std::string *error_out) {
    databases_semilattice_metadata_t db_metadata;
    get_databases_metadata(&db_metadata);
    const_metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
        &db_metadata.databases);

    std::vector<ql::datum_t> result_array;
    if (db_names.empty()) {
        for (auto it = db_searcher.find_next(db_searcher.begin());
                  it != db_searcher.end();
                  it = db_searcher.find_next(++it)) {
            guarantee(!it->second.is_deleted());
            ql::datum_t row;
            if (!admin_tables->db_config_backend->read_row(
                    convert_uuid_to_datum(it->first), interruptor, &row, error_out)) {
                return false;
            }
            if (row.has()) {
                result_array.push_back(row);
            }
        }
    } else {
        for (auto const &name : db_names) {
            metadata_search_status_t status;
            auto it = db_searcher.find_uniq(name, &status);
            if (!check_metadata_status(status, "Database", name.str(), true,
                    error_out)) return false;
            guarantee(!it->second.is_deleted());
            ql::datum_t row;
            if (!admin_tables->db_config_backend->read_row(
                    convert_uuid_to_datum(it->first), interruptor, &row, error_out)) {
                return false;
            }
            if (!row.has()) {
                *error_out = strprintf("Database `%s` does not exist.", name.c_str());
                return false;
            }
            result_array.push_back(row);
        }
    }

    counted_t<ql::table_t> table = make_table_with_backend(
        admin_tables->db_config_backend.get(), "db_config", bt);
    counted_t<ql::datum_stream_t> stream =
        make_counted<ql::vector_datum_stream_t>(
            bt,
            std::move(result_array),
            boost::optional<ql::changefeed::keyspec_t>());
    resp_out->init(new ql::val_t(make_counted<ql::selection_t>(table, stream), bt));
    return true;
}

bool real_reql_cluster_interface_t::table_create(const name_string_t &name,
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &config_params,
        const std::string &primary_key,
        signal_t *interruptor, std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");

    namespace_id_t table_id = generate_uuid();
    cluster_semilattice_metadata_t metadata;
    {
        cross_thread_signal_t interruptor2(interruptor,
            semilattice_root_view->home_thread());
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();

        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &metadata.rdb_namespaces);
        metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &ns_change.get()->namespaces);

        /* Make sure there isn't an existing table with the same name */
        {
            metadata_search_status_t status;
            namespace_predicate_t pred(&name, &db->id);
            ns_searcher.find_uniq(pred, &status);
            if (!check_metadata_status(status, "Table", db->name + "." + name.str(),
                false, error_out)) return false;
        }

        table_replication_info_t repli_info;

        /* We don't have any data to generate split points based on, so assume UUIDs */
        calculate_split_points_for_uuids(
            config_params.num_shards, &repli_info.shard_scheme);

        /* Construct a configuration for the new namespace */
        std::map<server_id_t, int> server_usage;
        for (auto it = ns_change.get()->namespaces.begin();
                  it != ns_change.get()->namespaces.end();
                ++it) {
            if (it->second.is_deleted()) {
                continue;
            }
            calculate_server_usage(
                it->second.get_ref().replication_info.get_ref().config, &server_usage);
        }
        if (!table_generate_config(
                server_config_client, nil_uuid(), nullptr, server_usage,
                config_params, table_shard_scheme_t(), &interruptor2,
                &repli_info.config, error_out)) {
            *error_out = "When generating configuration for new table: " + *error_out;
            return false;
        }

        repli_info.config.write_ack_config.mode = write_ack_config_t::mode_t::majority;
        repli_info.config.durability = write_durability_t::HARD;

        namespace_semilattice_metadata_t table_metadata;
        table_metadata.name = versioned_t<name_string_t>(name);
        table_metadata.database = versioned_t<database_id_t>(db->id);
        table_metadata.primary_key = versioned_t<std::string>(primary_key);
        table_metadata.replication_info =
            versioned_t<table_replication_info_t>(repli_info);

        ns_change.get()->namespaces.insert(
            std::make_pair(table_id, make_deletable(table_metadata)));

        semilattice_root_view->join(metadata);
        metadata = semilattice_root_view->get();

        table_status_artificial_table_backend_t *status_backend =
            admin_tables->table_status_backend[
                static_cast<int>(admin_identifier_format_t::name)].get();

        wait_for_table_readiness(
            table_id,
            table_readiness_t::finished,
            status_backend,
            &interruptor2);
    }

    // This could hang because of a node going down or the user deleting the table.
    // In that case, timeout after 10 seconds and pretend everything's alright.
    signal_timer_t timer_interruptor;
    wait_any_t combined_interruptor(interruptor, &timer_interruptor);
    timer_interruptor.start(10000);
    try {
        namespace_interface_access_t ns_if =
            namespace_repo.get_namespace_interface(table_id, &combined_interruptor);
        while (!ns_if.get()->check_readiness(table_readiness_t::writes,
                                             &combined_interruptor)) {
            nap(100, &combined_interruptor);
        }
    } catch (const interrupted_exc_t &) {
        // Do nothing
    }
    return true;
}

bool real_reql_cluster_interface_t::table_drop(const name_string_t &name,
        counted_t<const ql::db_t> db, signal_t *interruptor, std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cluster_semilattice_metadata_t metadata;
    {
        on_thread_t thread_switcher(semilattice_root_view->home_thread());
        metadata = semilattice_root_view->get();

        /* Find the specified table */
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
                &metadata.rdb_namespaces);
        metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
                &ns_change.get()->namespaces);
        metadata_search_status_t status;
        namespace_predicate_t pred(&name, &db->id);
        metadata_searcher_t<namespace_semilattice_metadata_t>::iterator
            ns_metadata = ns_searcher.find_uniq(pred, &status);
        if (!check_metadata_status(status, "Table", db->name + "." + name.str(), true,
                error_out)) return false;
        guarantee(!ns_metadata->second.is_deleted());

        /* Delete the table. */
        ns_metadata->second.mark_deleted();

        semilattice_root_view->join(metadata);
        metadata = semilattice_root_view->get();
    }
    wait_for_metadata_to_propagate(metadata, interruptor);

    return true;
}

bool real_reql_cluster_interface_t::table_list(counted_t<const ql::db_t> db,
        UNUSED signal_t *interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cow_ptr_t<namespaces_semilattice_metadata_t> ns_metadata = get_namespaces_metadata();
    const_metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
        &ns_metadata->namespaces);
    namespace_predicate_t pred(&db->id);
    for (auto it = ns_searcher.find_next(ns_searcher.begin(), pred);
              it != ns_searcher.end();
              it = ns_searcher.find_next(++it, pred)) {
        guarantee(!it->second.is_deleted());
        names_out->insert(it->second.get_ref().name.get_ref());
    }

    return true;
}

bool real_reql_cluster_interface_t::table_find(
        const name_string_t &name, counted_t<const ql::db_t> db,
        UNUSED boost::optional<admin_identifier_format_t> identifier_format,
        signal_t *interruptor,
        counted_t<base_table_t> *table_out, std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    /* Find the specified table in the semilattice metadata */
    cow_ptr_t<namespaces_semilattice_metadata_t> namespaces_metadata
        = get_namespaces_metadata();
    const_metadata_searcher_t<namespace_semilattice_metadata_t>
        ns_searcher(&namespaces_metadata.get()->namespaces);
    namespace_predicate_t pred(&name, &db->id);
    metadata_search_status_t status;
    auto ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    if (!check_metadata_status(status, "Table", db->name + "." + name.str(), true,
            error_out)) return false;
    guarantee(!ns_metadata_it->second.is_deleted());
    /* Note that we completely ignore `identifier_format`. `identifier_format` is
    meaningless for real tables, so it might seem like we should produce an error. The
    reason we don't is that the user might write a query that access both a system table
    and a real table, and they might specify `identifier_format` as a global optarg.
    So then they would get a spurious error for the real table. This behavior is also
    consistent with that of system tables that aren't affected by `identifier_format`. */
    table_out->reset(new real_table_t(
        ns_metadata_it->first,
        namespace_repo.get_namespace_interface(ns_metadata_it->first, interruptor),
        ns_metadata_it->second.get_ref().primary_key.get_ref(),
        &changefeed_client));

    return true;
}

bool real_reql_cluster_interface_t::get_table_ids_for_query(
        const counted_t<const ql::db_t> &db,
        const std::vector<name_string_t> &table_names,
        std::vector<std::pair<namespace_id_t, name_string_t> > *tables_out,
        std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");

    tables_out->clear();
    cow_ptr_t<namespaces_semilattice_metadata_t> ns_metadata = get_namespaces_metadata();
    const_metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
        &ns_metadata->namespaces);

    if (table_names.empty()) {
        namespace_predicate_t pred(&db->id);
        for (auto it = ns_searcher.find_next(ns_searcher.begin(), pred);
                  it != ns_searcher.end();
                  it = ns_searcher.find_next(++it, pred)) {
            guarantee(!it->second.is_deleted());
            tables_out->push_back({ it->first, it->second.get_ref().name.get_ref() });
        }
    } else {
        for (auto const &name : table_names) {
            namespace_predicate_t pred(&name, &db->id);
            metadata_search_status_t status;
            auto it = ns_searcher.find_uniq(pred, &status);
            if (!check_metadata_status(status, "Table", db->name + "." + name.str(), true,
                    error_out)) return false;
            guarantee(!it->second.is_deleted());
            tables_out->push_back({ it->first, it->second.get_ref().name.get_ref() });
        }
    }
    return true;
}

bool real_reql_cluster_interface_t::table_config(
        counted_t<const ql::db_t> db,
        const std::vector<name_string_t> &tables,
        const ql::protob_t<const Backtrace> &bt,
        signal_t *interruptor, scoped_ptr_t<ql::val_t> *resp_out,
        std::string *error_out) {
    std::vector<std::pair<namespace_id_t, name_string_t> > table_ids;
    if (!get_table_ids_for_query(db, tables, &table_ids, error_out)) {
        return false;
    }

    artificial_table_backend_t *backend =
        admin_tables->table_config_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();

    std::vector<ql::datum_t> result_array;
    if (!table_meta_read(backend, db, table_ids, true,
            interruptor, &result_array, error_out)) {
        return false;
    }

    counted_t<ql::table_t> table = make_table_with_backend(backend, "table_config", bt);
    counted_t<ql::datum_stream_t> stream =
        make_counted<ql::vector_datum_stream_t>(
            bt,
            std::move(result_array),
            boost::optional<ql::changefeed::keyspec_t>());
    resp_out->init(new ql::val_t(make_counted<ql::selection_t>(table, stream), bt));
    return true;
}

bool real_reql_cluster_interface_t::table_status(
        counted_t<const ql::db_t> db,
        const std::vector<name_string_t> &tables,
        const ql::protob_t<const Backtrace> &bt,
        signal_t *interruptor, scoped_ptr_t<ql::val_t> *resp_out,
        std::string *error_out) {
    std::vector<std::pair<namespace_id_t, name_string_t> > table_ids;
    if (!get_table_ids_for_query(db, tables, &table_ids, error_out)) {
        return false;
    }

    artificial_table_backend_t *backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();

    std::vector<ql::datum_t> result_array;
    if (!table_meta_read(backend, db, table_ids, true,
            interruptor, &result_array, error_out)) {
        return false;
    }

    counted_t<ql::table_t> table = make_table_with_backend(backend, "table_status", bt);
    counted_t<ql::datum_stream_t> stream =
        make_counted<ql::vector_datum_stream_t>(
            bt,
            std::move(result_array),
            boost::optional<ql::changefeed::keyspec_t>());
    resp_out->init(new ql::val_t(make_counted<ql::selection_t>(table, stream), bt));
    return true;
}

bool real_reql_cluster_interface_t::table_wait(
        counted_t<const ql::db_t> db,
        const std::vector<name_string_t> &tables,
        table_readiness_t readiness,
        const ql::protob_t<const Backtrace> &bt,
        signal_t *interruptor,
        scoped_ptr_t<ql::val_t> *resp_out,
        std::string *error_out) {
    std::vector<std::pair<namespace_id_t, name_string_t> > table_ids;
    if (!get_table_ids_for_query(db, tables, &table_ids, error_out)) {
        return false;
    }

    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();

    std::vector<ql::datum_t> result_array;
    while (true) {
        result_array.clear();
        {
            threadnum_t new_thread = directory_root_view->home_thread();
            cross_thread_signal_t ct_interruptor(interruptor, new_thread);
            on_thread_t thread_switcher(new_thread);
            rassert(new_thread == status_backend->home_thread());

            // Loop until all tables are ready - we have to check all tables again
            // if a table was not immediately ready
            bool immediate;
            do {
                immediate = true;
                for (auto it = table_ids.begin(); it != table_ids.end(); ++it) {
                    table_wait_result_t res = wait_for_table_readiness(
                        it->first, readiness, status_backend, &ct_interruptor);
                    immediate = immediate && (res == table_wait_result_t::IMMEDIATE);
                    // Error out if a table was deleted and it was explicitly waited on
                    if (res == table_wait_result_t::DELETED) {
                        if (tables.size() != 0) {
                            *error_out = strprintf("Table `%s.%s` does not exist.",
                                                   db->name.c_str(),
                                                   it->second.str().c_str());
                            return false;
                        } else {
                            // We erase the table_id here to avoid getting a DELETED result
                            // every loop, so that `immediate` is never true
                            table_ids.erase(it);
                            break;
                        }
                    }
                }
            } while (!immediate && table_ids.size() != 1);

            // It is important for correctness that nothing runs in-between the wait and the
            // `table_status` read.
            ASSERT_FINITE_CORO_WAITING;
            if (!table_meta_read(
                    status_backend, db, table_ids,
                    !tables.empty(), // A db-level wait shouldn't error if a table is deleted
                    &ct_interruptor, &result_array, error_out)) {
                return false;
            }
        }

        // Make sure the tables are available - if not, wait a bit and loop again
        // We cannot wait for anything higher than 'writes' through namespace_interface_t
        table_readiness_t readiness2 = std::min(readiness, table_readiness_t::writes);
        bool success = true;
        for (auto it = table_ids.begin(); it != table_ids.end() && success; ++it) {
            namespace_interface_access_t ns_if =
                namespace_repo.get_namespace_interface(it->first, interruptor);
            success = success && ns_if.get()->check_readiness(readiness2, interruptor);
        }

        if (success) {
            break;
        }

        nap(100, interruptor);
    }

    counted_t<ql::table_t> table = make_table_with_backend(
        status_backend, "table_status", bt);
    counted_t<ql::datum_stream_t> stream =
        make_counted<ql::vector_datum_stream_t>(
            bt,
            std::move(result_array),
            boost::optional<ql::changefeed::keyspec_t>());
    resp_out->init(new ql::val_t(make_counted<ql::selection_t>(table, stream), bt));
    return true;
}

bool real_reql_cluster_interface_t::reconfigure_internal(
        const counted_t<const ql::db_t> &db,
        const namespace_id_t &table_id,
        const name_string_t &table_name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    rassert(get_thread_id() == server_config_client->home_thread());
    std::vector<std::pair<namespace_id_t, name_string_t> > table_map;
    table_map.push_back(std::make_pair(table_id, table_name));

    /* Find the specified table in the semilattice metadata */
    cluster_semilattice_metadata_t metadata = semilattice_root_view->get();
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &metadata.rdb_namespaces);
    metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &ns_change.get()->namespaces);
    auto ns_metadata_it = ns_change.get()->namespaces.find(table_id);
    r_sanity_check(ns_metadata_it != ns_change.get()->namespaces.end() &&
                   !ns_metadata_it->second.is_deleted());

    // Store the old value of the config and status
    ql::datum_object_builder_t result_builder;
    {
        ql::datum_object_builder_t old_val_builder;
        old_val_builder.overwrite("config",
            convert_table_config_to_datum(
                ns_metadata_it->second.get_ref().replication_info.get_ref().config,
                admin_identifier_format_t::name,
                server_config_client));

        table_status_artificial_table_backend_t *backend =
            admin_tables->table_status_backend[
                static_cast<int>(admin_identifier_format_t::name)].get();

        std::vector<ql::datum_t> old_status;
        if (!table_meta_read(backend, db, table_map,
                             true, interruptor, &old_status, error_out)) {
            return false;
        }
        guarantee(old_status.size() == 1);
        old_val_builder.overwrite("status", old_status[0]);
        result_builder.overwrite("old_val", std::move(old_val_builder).to_datum());
    }

    std::map<server_id_t, int> server_usage;
    for (auto it = ns_searcher.find_next(ns_searcher.begin());
              it != ns_searcher.end();
              it = ns_searcher.find_next(++it)) {
        if (it == ns_metadata_it) {
            /* We don't want to take into account the table's current configuration,
            since we're about to change that anyway */
            continue;
        }
        calculate_server_usage(it->second.get_ref().replication_info.get_ref().config,
                               &server_usage);
    }

    table_replication_info_t new_repli_info;

    if (!calculate_split_points_intelligently(
            table_id,
            this,
            params.num_shards,
            ns_metadata_it->second.get_ref().replication_info.get_ref().shard_scheme,
            interruptor,
            &new_repli_info.shard_scheme,
            error_out)) {
        return false;
    }

    /* This just generates a new configuration; it doesn't put it in the
    semilattices. */
    if (!table_generate_config(
            server_config_client,
            table_id,
            directory_root_view,
            server_usage,
            params,
            new_repli_info.shard_scheme,
            interruptor,
            &new_repli_info.config,
            error_out)) {
        return false;
    }

    new_repli_info.config.write_ack_config.mode = write_ack_config_t::mode_t::majority;
    new_repli_info.config.durability = write_durability_t::HARD;

    if (!dry_run) {
        /* Commit the change */
        ns_metadata_it->second.get_mutable()->replication_info.set(new_repli_info);
        semilattice_root_view->join(metadata);
    }

    // Store the new value of the config and status
    {
        ql::datum_object_builder_t new_val_builder;
        new_val_builder.overwrite("config",
            convert_table_config_to_datum(
                new_repli_info.config,
                admin_identifier_format_t::name,
                server_config_client));

        table_status_artificial_table_backend_t *backend =
            admin_tables->table_status_backend[
                static_cast<int>(admin_identifier_format_t::name)].get();

        std::vector<ql::datum_t> new_status;
        if (!table_meta_read(backend, db, table_map,
                             true, interruptor, &new_status, error_out)) {
            return false;
        }
        guarantee(new_status.size() == 1);
        new_val_builder.overwrite("status", new_status[0]);
        result_builder.overwrite("new_val", std::move(new_val_builder).to_datum());
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
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(
        interruptor, server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());

    std::vector<std::pair<namespace_id_t, name_string_t> > tables;
    if (!get_table_ids_for_query(db, { name }, &tables, error_out)) {
        return false;
    }
    rassert(tables.size() == 1);

    return reconfigure_internal(db, tables.begin()->first, name, params, dry_run,
                                &ct_interruptor, result_out, error_out);
}

bool real_reql_cluster_interface_t::db_reconfigure(
        counted_t<const ql::db_t> db,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());

    std::vector<std::pair<namespace_id_t, name_string_t> > tables;
    if (!get_table_ids_for_query(db, std::vector<name_string_t>(), &tables, error_out)) {
        return false;
    }

    ql::datum_array_builder_t array_builder(ql::configured_limits_t::unlimited);
    for (auto const &pair : tables) {
        ql::datum_t table_result;
        if (!reconfigure_internal(db, pair.first, pair.second, params, dry_run,
                                  &ct_interruptor, &table_result, error_out)) {
            return false;
        }
        array_builder.add(table_result);
    }
    *result_out = std::move(array_builder).to_datum();
    return true;
}

bool real_reql_cluster_interface_t::rebalance_internal(
        const counted_t<const ql::db_t> &db,
        const namespace_id_t &table_id,
        const name_string_t &table_name,
        signal_t *interruptor,
        ql::datum_t *results_out,
        std::string *error_out) {
    std::vector<std::pair<namespace_id_t, name_string_t> > tables;
    tables.push_back(std::make_pair(table_id, table_name));

    table_status_artificial_table_backend_t *status_backend =
        admin_tables->table_status_backend[
            static_cast<int>(admin_identifier_format_t::name)].get();

    std::vector<ql::datum_t> old_status;
    if (!table_meta_read(status_backend, db, tables, true,
            interruptor, &old_status, error_out)) {
        return false;
    }
    guarantee(old_status.size() == 1);

    /* Find the specified table in the semilattice metadata */
    cluster_semilattice_metadata_t metadata = semilattice_root_view->get();
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t ns_change(
            &metadata.rdb_namespaces);
    metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &ns_change.get()->namespaces);
    auto it = ns_change.get()->namespaces.find(table_id);
    r_sanity_check(it != ns_change.get()->namespaces.end() && !it->second.is_deleted());

    std::map<store_key_t, int64_t> counts;
    if (!fetch_distribution(table_id, this, interruptor, &counts, error_out)) {
        *error_out = "When measuring document distribution: " + *error_out;
        return false;
    }

    table_replication_info_t new_repli_info =
        it->second.get_ref().replication_info.get_ref();
    if (!calculate_split_points_with_distribution(
            counts,
            new_repli_info.config.shards.size(),
            &new_repli_info.shard_scheme,
            error_out)) {
        return false;
    }

    it->second.get_mutable()->replication_info.set(new_repli_info);
    semilattice_root_view->join(metadata);

    std::vector<ql::datum_t> new_status;
    if (!table_meta_read(status_backend, db, tables, true,
            interruptor, &new_status, error_out)) {
        return false;
    }
    guarantee(new_status.size() == 1);

    ql::datum_object_builder_t builder;
    builder.overwrite("old_status", old_status[0]);
    builder.overwrite("new_status", new_status[0]);
    *results_out = std::move(builder).to_datum();

    return true;
}

bool real_reql_cluster_interface_t::table_rebalance(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());

    std::vector<std::pair<namespace_id_t, name_string_t> > tables;
    if (!get_table_ids_for_query(db, { name }, &tables, error_out)) {
        return false;
    }
    rassert(tables.size() == 1);

    return rebalance_internal(db, tables.begin()->first, name, &ct_interruptor,
        result_out, error_out);
}

bool real_reql_cluster_interface_t::db_rebalance(
        counted_t<const ql::db_t> db,
        signal_t *interruptor,
        ql::datum_t *result_out,
        std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t ct_interruptor(interruptor,
        server_config_client->home_thread());
    on_thread_t thread_switcher(server_config_client->home_thread());

    std::vector<std::pair<namespace_id_t, name_string_t> > tables;
    if (!get_table_ids_for_query(db, std::vector<name_string_t>(), &tables, error_out)) {
        return false;
    }

    ql::datum_array_builder_t array_builder(ql::configured_limits_t::unlimited);
    for (auto const &pair : tables) {
        ql::datum_t table_result;
        if (!rebalance_internal(db, pair.first, pair.second, &ct_interruptor,
                                &table_result, error_out)) {
            return false;
        }
        array_builder.add(table_result);
    }
    *result_out = std::move(array_builder).to_datum();
    return true;
}

bool real_reql_cluster_interface_t::table_estimate_doc_counts(
        counted_t<const ql::db_t> db,
        const name_string_t &name,
        ql::env_t *env,
        std::vector<int64_t> *doc_counts_out,
        std::string *error_out) {
    guarantee(db->name != "rethinkdb",
        "real_reql_cluster_interface_t should never get queries for system tables");
    cross_thread_signal_t interruptor2(env->interruptor,
        semilattice_root_view->home_thread());
    on_thread_t thread_switcher(semilattice_root_view->home_thread());

    /* Find the specified table in the semilattice metadata */
    cluster_semilattice_metadata_t metadata = semilattice_root_view->get();
    const_metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &metadata.rdb_namespaces->namespaces);
    namespace_predicate_t pred(&name, &db->id);
    metadata_search_status_t status;
    auto ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    if (!check_metadata_status(status, "Table", db->name + "." + name.str(), true,
            error_out)) return false;

    /* Perform a distribution query against the database */
    namespace_interface_access_t ns_if_access =
        namespace_repo.get_namespace_interface(ns_metadata_it->first, &interruptor2);
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
    const table_shard_scheme_t &shard_scheme =
        ns_metadata_it->second.get_ref().replication_info.get_ref().shard_scheme;
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
    return true;
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

    guarantee(cross_thread_namespace_watchables[threadnum].has());
    cross_thread_namespace_watchables[threadnum]->get_watchable()->run_until_satisfied(
            [&] (const cow_ptr_t<namespaces_semilattice_metadata_t> &md) -> bool
                { return is_joined(md, metadata.rdb_namespaces); },
            interruptor);

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

cow_ptr_t<namespaces_semilattice_metadata_t>
real_reql_cluster_interface_t::get_namespaces_metadata() {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_namespace_watchables[threadnum].has());
    cow_ptr_t<namespaces_semilattice_metadata_t> ret;
    cross_thread_namespace_watchables[threadnum]->apply_read(
            std::bind(&copy_value< cow_ptr_t<namespaces_semilattice_metadata_t> >,
                      ph::_1, &ret));
    return ret;
}

void real_reql_cluster_interface_t::get_databases_metadata(
        databases_semilattice_metadata_t *out) {
    int threadnum = get_thread_id().threadnum;
    r_sanity_check(cross_thread_database_watchables[threadnum].has());
    cross_thread_database_watchables[threadnum]->apply_read(
            std::bind(&copy_value<databases_semilattice_metadata_t>,
                      ph::_1, out));
}

bool real_reql_cluster_interface_t::table_meta_read(
        artificial_table_backend_t *backend,
        const counted_t<const ql::db_t> &db,
        const std::vector<std::pair<namespace_id_t, name_string_t> > &table_ids,
        bool error_on_missing,
        signal_t *interruptor,
        std::vector<ql::datum_t> *res_out,
        std::string *error_out) {
    for (auto const &pair : table_ids) {
        ql::datum_t row;
        if (!backend->read_row(convert_uuid_to_datum(pair.first),
                               interruptor, &row, error_out)) {
            return false;
        }

        if (row.has()) {
            res_out->push_back(row);
        } else if (error_on_missing) {
            *error_out = strprintf("Table `%s.%s` does not exist.",
                                   db->name.c_str(), pair.second.str().c_str());
            return false;
        }
    }
    return true;
}

