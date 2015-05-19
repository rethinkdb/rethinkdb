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

disk_compaction_job_report_t::disk_compaction_job_report_t()
    : job_report_base_t<disk_compaction_job_report_t>() { }

disk_compaction_job_report_t::disk_compaction_job_report_t(
        uuid_u const &_id,
        double _duration,
        server_id_t const &_server_id)
    : job_report_base_t<disk_compaction_job_report_t>(
        "disk_compaction", _id, _duration, _server_id) { }

void disk_compaction_job_report_t::merge_derived(
        disk_compaction_job_report_t const &) { }

bool disk_compaction_job_report_t::info_derived(
        UNUSED admin_identifier_format_t identifier_format,
        UNUSED server_config_client_t *server_config_client,
        UNUSED table_meta_client_t *table_meta_client,
        UNUSED cluster_semilattice_metadata_t const &metadata,
        UNUSED ql::datum_object_builder_t *info_builder_out) const {
    return true;
}

RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
    disk_compaction_job_report_t, type, id, duration, servers);

backfill_job_report_t::backfill_job_report_t()
    : job_report_base_t<backfill_job_report_t>() { }

backfill_job_report_t::backfill_job_report_t(
        uuid_u const &_id,
        double _duration,
        server_id_t const &_server_id,
        namespace_id_t const &_table,
        bool _is_ready,
        double _progress,
        server_id_t const &_source_server,
        server_id_t const &_destination_server)
    : job_report_base_t<backfill_job_report_t>("backfill", _id, _duration, _server_id),
      table(_table),
      is_ready(_is_ready),
      progress_numerator(_progress),
      progress_denominator(1.0),
      source_server(_source_server),
      destination_server(_destination_server) {
    servers.insert({source_server, destination_server});
}

void backfill_job_report_t::merge_derived(
       backfill_job_report_t const &job_report) {
    is_ready &= job_report.is_ready;
    progress_numerator += job_report.progress_numerator;
    progress_denominator += job_report.progress_denominator;
}

bool backfill_job_report_t::info_derived(
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        cluster_semilattice_metadata_t const &metadata,
        ql::datum_object_builder_t *info_builder_out) const {
    if (is_ready) {
        return false;   // All shards are ready, skip this job.
    }

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
    info_builder_out->overwrite("table", table_name_or_uuid);
    info_builder_out->overwrite("db", db_name_or_uuid);

    ql::datum_t source_server_name_or_uuid;
    if (convert_connected_server_id_to_datum(
            source_server,
            identifier_format,
            server_config_client,
            &source_server_name_or_uuid,
            nullptr)) {
        info_builder_out->overwrite("source_server", source_server_name_or_uuid);
    } else {
        return false;
    }

    ql::datum_t destination_server_name_or_uuid;
    if (convert_connected_server_id_to_datum(
            destination_server,
            identifier_format,
            server_config_client,
            &destination_server_name_or_uuid,
            nullptr)) {
        info_builder_out->overwrite(
            "destination_server", destination_server_name_or_uuid);
    } else {
        return false;
    }

    info_builder_out->overwrite("progress",
        ql::datum_t(progress_numerator / progress_denominator));

    return true;
}

RDB_IMPL_SERIALIZABLE_10_FOR_CLUSTER(
    backfill_job_report_t,
    type,
    id,
    duration,
    servers,
    table,
    is_ready,
    progress_numerator,
    progress_denominator,
    source_server,
    destination_server);

index_construction_job_report_t::index_construction_job_report_t()
    : job_report_base_t<index_construction_job_report_t>() { }

index_construction_job_report_t::index_construction_job_report_t(
        uuid_u const &_id,
        double _duration,
        server_id_t const &_server_id,
        namespace_id_t const &_table,
        std::string const &_index,
        bool _is_ready,
        double _progress)
    : job_report_base_t<index_construction_job_report_t>(
        "index_construction", _id, _duration, _server_id),
      table(_table),
      index(_index),
      is_ready(_is_ready),
      progress_numerator(_progress),
      progress_denominator(1.0) { }

void index_construction_job_report_t::merge_derived(
       index_construction_job_report_t const &job_report) {
    is_ready &= job_report.is_ready;
    progress_numerator += job_report.progress_numerator;
    progress_denominator += job_report.progress_denominator;
}

bool index_construction_job_report_t::info_derived(
        admin_identifier_format_t identifier_format,
        UNUSED server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        cluster_semilattice_metadata_t const &metadata,
        ql::datum_object_builder_t *info_builder_out) const {
    if (is_ready) {
        return false;   // All shards are ready, skip this job.
    }

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
    info_builder_out->overwrite("table", table_name_or_uuid);
    info_builder_out->overwrite("db", db_name_or_uuid);

    info_builder_out->overwrite("index", convert_string_to_datum(index));
    info_builder_out->overwrite("progress",
        ql::datum_t(progress_numerator / progress_denominator));

    return true;
}

RDB_IMPL_SERIALIZABLE_9_FOR_CLUSTER(
    index_construction_job_report_t,
    type,
    id,
    duration,
    servers,
    table,
    index,
    is_ready,
    progress_numerator,
    progress_denominator);

query_job_report_t::query_job_report_t()
    : job_report_base_t<query_job_report_t>() { }

query_job_report_t::query_job_report_t(
        uuid_u const &_id,
        double _duration,
        server_id_t const &_server_id,
        ip_and_port_t const &_client_addr_port,
        std::string const &_query)
    : job_report_base_t<query_job_report_t>("query", _id, _duration, _server_id),
      client_addr_port(_client_addr_port),
      query(_query) { }

void query_job_report_t::merge_derived(query_job_report_t const &) { }

bool query_job_report_t::info_derived(
        UNUSED admin_identifier_format_t identifier_format,
        UNUSED server_config_client_t *server_config_client,
        UNUSED table_meta_client_t *table_meta_client,
        UNUSED cluster_semilattice_metadata_t const &metadata,
        ql::datum_object_builder_t *info_builder_out) const {
    info_builder_out->overwrite("client_address",
        convert_string_to_datum(client_addr_port.ip().to_string()));
    info_builder_out->overwrite("client_port",
        convert_port_to_datum(client_addr_port.port().value()));
    info_builder_out->overwrite("query", convert_string_to_datum(query));

    return true;
}

RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(
    query_job_report_t, type, id, duration, servers, client_addr_port, query);
