// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/report.hpp"

job_report_t::job_report_t() {
}

job_report_t::job_report_t(
        uuid_u const &_id,
        std::string const &_type,
        double _duration,
        uuid_u const &_table,
        std::string const &_index)
    : id(_id),
      type(_type),
      duration(_duration),
      table(_table),
      index(_index) {
}

ql::datum_t job_report_t::to_datum(
        admin_identifier_format_t identifier_format,
        server_name_client_t *name_client,
        cluster_semilattice_metadata_t const &metadata) const {
    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    for (uuid_u const &server : servers) {
        ql::datum_t server_name_or_uuid;
        if (convert_server_id_to_datum(
                server,
                identifier_format,
                name_client,
                &server_name_or_uuid,
                nullptr) == true) {
            servers_builder.add(server_name_or_uuid);
        }
    }

    ql::datum_object_builder_t info_builder;
    ql::datum_t table_name_or_uuid;
    ql::datum_t db_name_or_uuid;
    if (convert_table_id_to_datums(
            table,
            identifier_format,
            metadata,
            &table_name_or_uuid,
            nullptr,
            &db_name_or_uuid,
            nullptr) == true) {
        info_builder.overwrite("table", table_name_or_uuid);
        info_builder.overwrite("db", db_name_or_uuid);
    }
    if (!index.empty()) {
        info_builder.overwrite("index", convert_string_to_datum(index));
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(id));
    builder.overwrite("servers", std::move(servers_builder).to_datum());
    builder.overwrite("type", convert_string_to_datum(type));
    builder.overwrite("duration_sec",
        duration >= 0 ? ql::datum_t(duration / 1e6) : ql::datum_t::null());
    builder.overwrite("info", std::move(info_builder).to_datum());
    return std::move(builder).to_datum();
}

RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(job_report_t, type, id, duration, table, index, servers);

sindex_job_t::sindex_job_t(microtime_t _start_time)
    : start_time(_start_time) {
};
