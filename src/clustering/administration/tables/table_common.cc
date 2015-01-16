// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "concurrency/cross_thread_signal.hpp"

common_table_artificial_table_backend_t::common_table_artificial_table_backend_t(
        boost::shared_ptr<semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        table_meta_client_t *_table_meta_client,
        admin_identifier_format_t _identifier_format) :
    semilattice_view(_semilattice_view),
    table_meta_client(_table_meta_client),
    identifier_format(_identifier_format),
    subs([this]() { notify_all(); }, semilattice_view)
{
    semilattice_view->assert_thread();
}

std::string common_table_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool common_table_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor_on_caller,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    std::map<namespace_id_t, table_config_and_shards_t> configs;
    table_meta_client->list_configs(&interruptor, &configs);
    rows_out->clear();
    for (const auto &pair : configs) {
        ql::datum_t db_name_or_uuid;
        if (!convert_database_id_to_datum(pair.second.config.database, identifier_format,
                metadata, &db_name_or_uuid, nullptr)) {
            db_name_or_uuid = ql::datum_t("__deleted_database__");
        }
        ql::datum_t row;
        if (!format_row(pair.first, db_name_or_uuid, pair.second,
                &interruptor, &row, error_out)) {
            return false;
        }
        rows_out->push_back(row);
    }
    return true;
}

bool common_table_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor_on_caller,
        ql::datum_t *row_out,
        std::string *error_out) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        *row_out = ql::datum_t();
        return true;
    }
    table_config_and_shards_t config;
    if (!table_meta_client->get_config(table_id, &interruptor, &config)) {
        *row_out = ql::datum_t();
        return true;
    }
    ql::datum_t db_name_or_uuid;
    if (!convert_database_id_to_datum(config.config.database, identifier_format,
            metadata, &db_name_or_uuid, nullptr)) {
        db_name_or_uuid = ql::datum_t("__deleted_database__");
    }
    return format_row(table_id, db_name_or_uuid, config,
        &interruptor, row_out, error_out);
}


