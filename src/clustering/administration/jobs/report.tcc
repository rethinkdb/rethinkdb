// Copyright 2010-2014 RethinkDB, all rights reserved.

template <typename T>
job_report_base_t<T>::job_report_base_t() {
}

template <typename T>
job_report_base_t<T>::job_report_base_t(
        std::string const &_type,
        uuid_u const &_id,
        double _duration,
        server_id_t const &_server_id)
    : type(_type),
      id(_id),
      duration(_duration),
      servers({_server_id}) { }

template <typename T>
void job_report_base_t<T>::merge(T const & job_report) {
    duration = std::max(duration, job_report.duration);
    servers.insert(job_report.servers.begin(), job_report.servers.end());

    static_cast<T *>(this)->merge_derived(job_report);
}

template <typename T>
bool job_report_base_t<T>::to_datum(
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        cluster_semilattice_metadata_t const &metadata,
        ql::datum_t *row_out) const {
    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    for (server_id_t const &server : servers) {
        ql::datum_t server_name_or_uuid;
        if (convert_connected_server_id_to_datum(
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
    if (static_cast<T const *>(this)->info_derived(
            identifier_format, server_config_client, table_meta_client,
            metadata, &info_builder) == false) {
        return false;
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("type", convert_string_to_datum(type));
    builder.overwrite("id", convert_job_type_and_id_to_datum(type, id));
    builder.overwrite("duration_sec",
        duration >= 0 ? ql::datum_t(duration / 1e6) : ql::datum_t::null());
    builder.overwrite("servers", std::move(servers_builder).to_datum());
    builder.overwrite("info", std::move(info_builder).to_datum());
    *row_out = std::move(builder).to_datum();

    return true;
}
