// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/report.hpp"

#include "clustering/administration/servers/config_client.hpp"

bool convert_job_type_and_id_from_datum(ql::datum_t primary_key,
                                        std::string *type_out,
                                        uuid_u *id_out) {
    std::string error;
    return primary_key.get_type() == ql::datum_t::R_ARRAY &&
           primary_key.arr_size() == 2 &&
           convert_string_from_datum(primary_key.get(0), type_out, &error) &&
           convert_uuid_from_datum(primary_key.get(1), id_out, &error);
}

ql::datum_t convert_job_type_and_id_to_datum(std::string const &type, uuid_u const &id) {
    ql::datum_array_builder_t primary_key_builder(ql::configured_limits_t::unlimited);
    primary_key_builder.add(convert_string_to_datum(type));
    primary_key_builder.add(convert_uuid_to_datum(id));
    return std::move(primary_key_builder).to_datum();
}

job_report_t::job_report_t() {
}

job_report_t::job_report_t(
        uuid_u const &_id,
        std::string const &_type,
        double _duration,
        ip_and_port_t const &_client_addr_port)
    : id(_id),
      type(_type),
      duration(_duration),
      client_addr_port(_client_addr_port),
      table(nil_uuid()),
      is_ready(false),
      progress_numerator(0.0),
      progress_denominator(0.0),
      destination_server(nil_uuid()) { }

job_report_t::job_report_t(
        uuid_u const &_id,
        std::string const &_type,
        double _duration,
        namespace_id_t const &_table,
        bool _is_ready,
        double progress,
        peer_id_t const &_source_peer,
        server_id_t const &_destination_server)
    : id(_id),
      type(_type),
      duration(_duration),
      table(_table),
      is_ready(_is_ready),
      progress_numerator(progress),
      progress_denominator(1.0),
      source_peer(_source_peer),
      destination_server(_destination_server) { }

job_report_t::job_report_t(
        uuid_u const &_id,
        std::string const &_type,
        double _duration,
        namespace_id_t const &_table,
        std::string const &_index,
        bool _is_ready,
        double progress)
    : id(_id),
      type(_type),
      duration(_duration),
      table(_table),
      index(_index),
      is_ready(_is_ready),
      progress_numerator(progress),
      progress_denominator(1.0),
      destination_server(nil_uuid()) { }

bool job_report_t::to_datum(
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        cluster_semilattice_metadata_t const &metadata,
        ql::datum_t *row_out) const {
    if ((type == "index_construction" || type == "backfill") && is_ready) {
        // All shards are ready, skip this report.
        return false;
    }

    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    for (uuid_u const &server : servers) {
        ql::datum_t server_name_or_uuid;
        if (convert_server_id_to_datum(
                server,
                identifier_format,
                server_config_client,
                &server_name_or_uuid,
                nullptr)) {
            servers_builder.add(server_name_or_uuid);
        }
    }

    ql::datum_object_builder_t info_builder;
    if (!table.is_nil()) {
        ql::datum_t table_name_or_uuid;
        ql::datum_t db_name_or_uuid;
        if (!convert_table_id_to_datums(
                table,
                identifier_format,
                metadata,
                table_meta_client,
                &table_name_or_uuid,
                nullptr,
                &db_name_or_uuid,
                nullptr)) {
            return false;
        }
        info_builder.overwrite("table", table_name_or_uuid);
        info_builder.overwrite("db", db_name_or_uuid);
    }
    if (type == "index_construction") {
        info_builder.overwrite("index", convert_string_to_datum(index));
        info_builder.overwrite("progress",
            ql::datum_t(progress_numerator / progress_denominator));
    } else if (type == "query") {
        info_builder.overwrite("client_address",
            convert_string_to_datum(client_addr_port.ip().to_string()));
        info_builder.overwrite("client_port",
            convert_port_to_datum(client_addr_port.port().value()));
    } else if (type == "backfill") {
        info_builder.overwrite("progress",
            ql::datum_t(progress_numerator / progress_denominator));

        boost::optional<server_id_t> source_server =
            server_config_client->get_server_id_for_peer_id(source_peer);
        if (static_cast<bool>(source_server)) {
            ql::datum_t source_server_name_or_uuid;
            if (convert_server_id_to_datum(
                    source_server.get(),
                    identifier_format,
                    server_config_client,
                    &source_server_name_or_uuid,
                    nullptr)) {
                info_builder.overwrite("source_server", source_server_name_or_uuid);

                // Make sure the source server is also in the servers list.
                if (servers.find(source_server.get()) == servers.end()) {
                    servers_builder.add(source_server_name_or_uuid);
                }
            } else {
                return false;
            }
        } else {
            return false;
        }

        ql::datum_t destination_server_name_or_uuid;
        if (convert_server_id_to_datum(
                destination_server,
                identifier_format,
                server_config_client,
                &destination_server_name_or_uuid,
                nullptr)) {
            info_builder.overwrite("destination_server", destination_server_name_or_uuid);

            if (servers.find(destination_server) == servers.end()) {
                servers_builder.add(destination_server_name_or_uuid);
            }
        } else {
            return false;
        }
    }

    if (servers_builder.empty()) {
        return false;
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_job_type_and_id_to_datum(type, id));
    builder.overwrite("servers", std::move(servers_builder).to_datum());
    builder.overwrite("type", convert_string_to_datum(type));
    builder.overwrite("duration_sec",
        duration >= 0 ? ql::datum_t(duration / 1e6) : ql::datum_t::null());
    builder.overwrite("info", std::move(info_builder).to_datum());
    *row_out = std::move(builder).to_datum();

    return true;
}
RDB_IMPL_SERIALIZABLE_12_FOR_CLUSTER(
    job_report_t,
    type,
    id,
    duration,
    client_addr_port,
    table,
    index,
    is_ready,
    progress_numerator,
    progress_denominator,
    source_peer,
    destination_server,
    servers);

query_job_t::query_job_t(
        microtime_t _start_time,
        ip_and_port_t const &_client_addr_port,
        cond_t *_interruptor)
    : start_time(_start_time),
      client_addr_port(_client_addr_port),
      interruptor(_interruptor) { }

sindex_job_t::sindex_job_t(
        microtime_t _start_time,
        bool _is_ready,
        double _progress)
    : start_time(_start_time),
      is_ready(_is_ready),
      progress(_progress) { }
