// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/report.hpp"

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
      table(nil_uuid()) { }

job_report_t::job_report_t(
        uuid_u const &_id,
        std::string const &_type,
        double _duration,
        namespace_id_t const &_table,
        std::string const &_index)
    : id(_id),
      type(_type),
      duration(_duration),
      table(_table),
      index(_index) { }

bool job_report_t::to_datum(
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        cluster_semilattice_metadata_t const &metadata,
        ql::datum_t *row_out) const {
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
    if (servers_builder.empty()) {
        return false;
    }

    ql::datum_object_builder_t info_builder;
    if (!table.is_nil()) {
        ql::datum_t table_name_or_uuid;
        ql::datum_t db_name_or_uuid;
        if (!convert_table_id_to_datums(
                table,
                identifier_format,
                metadata,
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
    } else if (type == "query") {
        info_builder.overwrite("client_address",
            convert_string_to_datum(client_addr_port.ip().to_string()));
        double port = client_addr_port.port().value();
        info_builder.overwrite("client_port", ql::datum_t(port));
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(id));
    builder.overwrite("servers", std::move(servers_builder).to_datum());
    builder.overwrite("type", convert_string_to_datum(type));
    builder.overwrite("duration_sec",
        duration >= 0 ? ql::datum_t(duration / 1e6) : ql::datum_t::null());
    builder.overwrite("info", std::move(info_builder).to_datum());
    *row_out = std::move(builder).to_datum();

    return true;
}
RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(
    job_report_t, type, id, duration, client_addr_port, table, index, servers);

query_job_t::query_job_t(microtime_t _start_time, ip_and_port_t const &_client_addr_port)
    : start_time(_start_time), client_addr_port(_client_addr_port) { }

