// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_status.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"
#include "clustering/administration/main/watchable_fields.hpp"

ql::datum_t convert_ip_to_datum(const ip_address_t &ip) {
    return ql::datum_t(datum_string_t(ip.to_string()));
}

ql::datum_t convert_host_and_port_to_datum(const host_and_port_t &x) {
    ql::datum_object_builder_t builder;
    builder.overwrite("host", ql::datum_t(datum_string_t(x.host())));
    builder.overwrite("port", convert_port_to_datum(x.port().value()));
    return std::move(builder).to_datum();
}

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

bool table_has_role_for_server(const server_id_t &server,
                               const table_config_t &table_config) {
    for (const table_config_t::shard_t &shard : table_config.shards) {
        if (shard.replicas.count(server) == 1) {
            return true;
        }
    }
    return false;
}

server_status_artificial_table_backend_t::server_status_artificial_table_backend_t(
    boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
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
            incremental_field_getter_t<server_id_t, cluster_directory_metadata_t>(
                                    &cluster_directory_metadata_t::server_id)))
{
    table_sl_view->assert_thread();
    database_sl_view->assert_thread();
}

bool server_status_artificial_table_backend_t::format_row(
        name_string_t const & server_name,
        server_id_t const & server_id,
        UNUSED server_semilattice_metadata_t const & server,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(server_name));
    builder.overwrite("id", convert_uuid_to_datum(server_id));

    boost::optional<peer_id_t> peer_id =
        name_client->get_peer_id_for_server_id(server_id);
    boost::optional<cluster_directory_metadata_t> directory;
    if (static_cast<bool>(peer_id)) {
        directory_view->apply_read(
            [&](const change_tracking_map_t<peer_id_t,
                    cluster_directory_metadata_t> *dv) {
                auto it = dv->get_inner().find(*peer_id);
                if (it != dv->get_inner().end()) {
                    directory = it->second;
                }
            });
    }

    if (directory) {
        builder.overwrite("status", ql::datum_t("available"));
    } else {
        builder.overwrite("status", ql::datum_t("unavailable"));
    }

    ql::datum_object_builder_t conn_builder;
    if (directory) {
        microtime_t connected_time =
            last_seen_tracker.get_connected_time(server_id);
        conn_builder.overwrite("time_connected",
            convert_microtime_to_datum(connected_time));
        conn_builder.overwrite("time_disconnected", ql::datum_t::null());
    } else {
        conn_builder.overwrite("time_connected", ql::datum_t::null());
        microtime_t disconnected_time =
            last_seen_tracker.get_disconnected_time(server_id);
        conn_builder.overwrite("time_disconnected",
            convert_microtime_to_datum(disconnected_time));
    }
    builder.overwrite("connection", std::move(conn_builder).to_datum());

    ql::datum_object_builder_t proc_builder;
    if (directory) {
        proc_builder.overwrite("time_started",
            convert_microtime_to_datum(directory->time_started));
        proc_builder.overwrite("version",
            ql::datum_t(datum_string_t(directory->version)));
        proc_builder.overwrite("cache_size_mb",
            ql::datum_t(static_cast<double>(directory->cache_size)/MEGABYTE));
        proc_builder.overwrite("pid", ql::datum_t(static_cast<double>(directory->pid)));
    } else {
        proc_builder.overwrite("time_started", ql::datum_t::null());
        proc_builder.overwrite("version", ql::datum_t::null());
        proc_builder.overwrite("cache_size_mb", ql::datum_t::null());
        proc_builder.overwrite("pid", ql::datum_t::null());
    }
    builder.overwrite("process", std::move(proc_builder).to_datum());

    ql::datum_object_builder_t net_builder;
    if (directory) {
        net_builder.overwrite("hostname",
            ql::datum_t(datum_string_t(directory->hostname)));
        net_builder.overwrite("cluster_port",
            convert_port_to_datum(directory->cluster_port));
        net_builder.overwrite("reql_port",
            convert_port_to_datum(directory->reql_port));
        net_builder.overwrite("http_admin_port",
            directory->http_admin_port
                ? convert_port_to_datum(*directory->http_admin_port)
                : ql::datum_t("<no http admin>"));
        net_builder.overwrite("canonical_addresses",
            convert_set_to_datum<host_and_port_t>(
                &convert_host_and_port_to_datum,
                directory->canonical_addresses));
    } else {
        net_builder.overwrite("hostname", ql::datum_t::null());
        net_builder.overwrite("cluster_port", ql::datum_t::null());
        net_builder.overwrite("reql_port", ql::datum_t::null());
        net_builder.overwrite("http_admin_port", ql::datum_t::null());
        net_builder.overwrite("canonical_addresses", ql::datum_t::null());
    }
    builder.overwrite("network", std::move(net_builder).to_datum());

    *row_out = std::move(builder).to_datum();
    return true;
}

bool server_status_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
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

