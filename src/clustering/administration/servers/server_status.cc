// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_status.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"
#include "clustering/administration/main/watchable_fields.hpp"

bool directory_has_role_for_table(const reactor_business_card_t &business_card) {
    for (auto it = business_card.activities.begin();
              it != business_card.activities.end();
            ++it) {
        if (!boost::get<reactor_business_card_t::nothing_t>(&it->second.activity)) {
            return true;
        }
    }
    return false;
}

bool table_has_role_for_server(const machine_id_t &server,
                               const table_config_t &table_config) {
    for (const table_config_t::shard_t &shard : table_config.shards) {
        if (shard.replicas.count(server) == 1) {
            return true;
        }
    }
    return false;
}

server_status_artificial_table_backend_t::server_status_artificial_table_backend_t(
    boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> >
        _servers_sl_view,
    server_name_client_t *_name_client,
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> > > _directory_view,
    boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<
        namespaces_semilattice_metadata_t> > > _table_sl_view,
    boost::shared_ptr<semilattice_readwrite_view_t<databases_semilattice_metadata_t> >
        _database_sl_view) :
    common_server_artificial_table_backend_t(_servers_sl_view, _name_client),
    directory_view(_directory_view), table_sl_view(_table_sl_view),
    database_sl_view(_database_sl_view),
    last_seen_tracker(
        _servers_sl_view,
        _directory_view->incremental_subview(
            incremental_field_getter_t<machine_id_t, cluster_directory_metadata_t>(
                                    &cluster_directory_metadata_t::machine_id)))
{
    table_sl_view->assert_thread();
    database_sl_view->assert_thread();
}

bool server_status_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    machines_semilattice_metadata_t servers_sl = servers_sl_view->get();
    name_string_t server_name;
    machine_id_t server_id;
    machine_semilattice_metadata_t *server_sl;
    if (!lookup(primary_key, &servers_sl, &server_name, &server_id, &server_sl)) {
        *row_out = ql::datum_t();
        return true;
    }
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(server_name));
    builder.overwrite("uuid", convert_uuid_to_datum(server_id));

    boost::optional<peer_id_t> peer_id =
        name_client->get_peer_id_for_machine_id(server_id);
    boost::optional<cluster_directory_metadata_t> directory;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *dv) {
            auto it = dv->get_inner().find(*peer_id);
            if (it != dv->get_inner().end()) {
                directory = boost::optional<cluster_directory_metadata_t>(it->second);
            }
        });

    if (directory) {
        builder.overwrite("status", ql::datum_t("available"));
    } else {
        builder.overwrite("status", ql::datum_t("unavailable"));
    }

    if (directory) {
        builder.overwrite("time_started",
            convert_microtime_to_datum(directory->time_started));
        microtime_t connected_time =
            last_seen_tracker.get_connected_time(server_id);
        builder.overwrite("time_connected",
            convert_microtime_to_datum(connected_time));
        builder.overwrite("time_disconnected", ql::datum_t::null());
    } else {
        builder.overwrite("time_started", ql::datum_t::null());
        builder.overwrite("time_connected", ql::datum_t::null());
        microtime_t disconnected_time =
            last_seen_tracker.get_disconnected_time(server_id);
        builder.overwrite("time_disconnected",
            convert_microtime_to_datum(disconnected_time));
    }

    if (directory) {
        builder.overwrite("version",
            ql::datum_t(datum_string_t(directory->version)));
        builder.overwrite("cache_size_mb",
            ql::datum_t(static_cast<double>(directory->cache_size)/MEGABYTE));
        builder.overwrite("pid", ql::datum_t(static_cast<double>(directory->pid)));
    } else {
        builder.overwrite("version", ql::datum_t::null());
        builder.overwrite("cache_size_mb", ql::datum_t::null());
        builder.overwrite("pid", ql::datum_t::null());
    }

    if (directory) {
        builder.overwrite("hostname",
            ql::datum_t(datum_string_t(directory->hostname)));
        builder.overwrite("cluster_port",
            convert_port_to_datum(directory->cluster_port));
        builder.overwrite("reql_port",
            convert_port_to_datum(directory->reql_port));
        builder.overwrite("http_admin_port",
            directory->http_admin_port
                ? convert_port_to_datum(*directory->http_admin_port)
                : ql::datum_t("<no http admin>"));
    } else {
        builder.overwrite("hostname", ql::datum_t::null());
        builder.overwrite("cluster_port", ql::datum_t::null());
        builder.overwrite("reql_port", ql::datum_t::null());
        builder.overwrite("http_admin_port", ql::datum_t::null());
    }

    if (directory) {
        cow_ptr_t<namespaces_semilattice_metadata_t> sl = table_sl_view->get();
        databases_semilattice_metadata_t dbs = database_sl_view->get();
        ql::datum_array_builder_t array_builder(ql::configured_limits_t::unlimited);
        for (auto it = sl->namespaces.begin(); it != sl->namespaces.end(); ++it) {
            if (it->second.is_deleted()) {
                continue;
            }
            bool in_config = table_has_role_for_server(server_id,
                it->second.get_ref().replication_info.get_ref().config);
            auto directory_it = directory->rdb_namespaces.reactor_bcards.find(it->first);
            bool in_directory =
                (directory_it != directory->rdb_namespaces.reactor_bcards.end()) &&
                directory_has_role_for_table(*directory_it->second.internal.get());
            if (!in_config && !in_directory) {
                continue;
            }
            name_string_t table_name = it->second.get_ref().name.get_ref();
            name_string_t db_name = get_db_name(it->second.get_ref().database.get_ref());
            ql::datum_object_builder_t pair_builder;
            pair_builder.overwrite("db", convert_name_to_datum(db_name));
            pair_builder.overwrite("table", convert_name_to_datum(table_name));
            array_builder.add(std::move(pair_builder).to_datum());
        }
        builder.overwrite("tables", std::move(array_builder).to_datum());
    } else {
        builder.overwrite("tables", ql::datum_t::null());
    }

    *row_out = std::move(builder).to_datum();
    return true;
}

bool server_status_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb.server_status` table.";
    return false;
}

name_string_t server_status_artificial_table_backend_t::get_db_name(
        database_id_t db_id) {
    assert_thread();
    databases_semilattice_metadata_t dbs = database_sl_view->get();
    if (dbs.databases.at(db_id).is_deleted()) {
        /* This can occur due to a race condition, if a new table is added to a database
        at the same time as it is being deleted. */
        return name_string_t::guarantee_valid("__deleted_database__");
    } else {
        return dbs.databases.at(db_id).get_ref().name.get_ref();
    }
}

