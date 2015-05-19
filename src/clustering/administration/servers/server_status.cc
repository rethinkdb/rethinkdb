// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_status.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
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

server_status_artificial_table_backend_t::server_status_artificial_table_backend_t(
        boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
            _servers_sl_view,
        server_config_client_t *_server_config_client,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory_view) :
    common_server_artificial_table_backend_t(_servers_sl_view, _server_config_client),
    directory_view(_directory_view),
    last_seen_tracker(_servers_sl_view, _directory_view),
    directory_subs(directory_view,
        [this](const peer_id_t &, const cluster_directory_metadata_t *md) {
            if (md == nullptr) {
                notify_all();
            } else {
                /* This is one of the rare cases where we can tell exactly which row
                needs to be recomputed */
                notify_row(convert_uuid_to_datum(md->server_id));
            }
        }, false)
    { }

server_status_artificial_table_backend_t::~server_status_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

bool server_status_artificial_table_backend_t::format_row(
        name_string_t const & server_name,
        server_id_t const & server_id,
        UNUSED server_semilattice_metadata_t const & server,
        UNUSED signal_t *interruptor_on_home,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(server_name));
    builder.overwrite("id", convert_uuid_to_datum(server_id));

    /* We don't have a subscription watching `get_peer_id_for_server_id()`, even though
    we use it in this calculation. This is OK because it's computed from the directory,
    and we do have a subscription watching the directory. It's true that there might be
    a gap between when they update, but because `cfeed_artificial_table_backend_t`
    recomputes asynchronously, it's OK. */
    boost::optional<peer_id_t> peer_id =
        server_config_client->get_peer_id_for_server_id(server_id);
    boost::optional<cluster_directory_metadata_t> directory;
    if (static_cast<bool>(peer_id)) {
        directory = directory_view->get_key(*peer_id);
    }

    if (directory) {
        builder.overwrite("status", ql::datum_t("connected"));
    } else {
        builder.overwrite("status", ql::datum_t("disconnected"));
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
        if (disconnected_time == 0) {
            conn_builder.overwrite("time_disconnected", ql::datum_t::null());
        } else {
            conn_builder.overwrite("time_disconnected",
                convert_microtime_to_datum(disconnected_time));
        }
    }
    builder.overwrite("connection", std::move(conn_builder).to_datum());

    ql::datum_object_builder_t proc_builder;
    if (directory) {
        proc_builder.overwrite("time_started",
            convert_microtime_to_datum(directory->time_started));
        proc_builder.overwrite("version",
            ql::datum_t(datum_string_t(directory->version)));
        proc_builder.overwrite("pid", ql::datum_t(static_cast<double>(directory->pid)));
        proc_builder.overwrite("argv",
            convert_vector_to_datum<std::string>(
                &convert_string_to_datum,
                directory->argv));
        proc_builder.overwrite("cache_size_mb", ql::datum_t(
            static_cast<double>(directory->actual_cache_size_bytes) / MEGABYTE));
    } else {
        proc_builder.overwrite("time_started", ql::datum_t::null());
        proc_builder.overwrite("version", ql::datum_t::null());
        proc_builder.overwrite("pid", ql::datum_t::null());
        proc_builder.overwrite("argv", ql::datum_t::null());
        proc_builder.overwrite("cache_size_mb", ql::datum_t::null());
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
        UNUSED signal_t *interruptor_on_caller,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb.server_status` table.";
    return false;
}

