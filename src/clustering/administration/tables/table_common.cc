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
    identifier_format(_identifier_format)
{
    semilattice_view->assert_thread();
}

std::string common_table_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool common_table_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor_on_caller,
        std::vector<ql::datum_t> *rows_out,
        UNUSED std::string *error_out) {
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    std::map<namespace_id_t, table_config_and_shards_t> configs;
    std::map<namespace_id_t, table_basic_config_t> disconnected_configs;
    table_meta_client->list_configs(
        &interruptor_on_home, &configs, &disconnected_configs);
    rows_out->clear();
    for (const auto &pair : configs) {
        ql::datum_t db_name_or_uuid;
        if (!convert_database_id_to_datum(
                pair.second.config.basic.database, identifier_format, metadata,
                &db_name_or_uuid, nullptr)) {
            db_name_or_uuid = ql::datum_t("__deleted_database__");
        }
        try {
            ql::datum_t row;
            format_row(pair.first, pair.second, db_name_or_uuid,
                &interruptor_on_home, &row);
            rows_out->push_back(row);
        } catch (const no_such_table_exc_t &) {
            /* The table got deleted between the call to `list_configs()` and the
            call to `format_row()`. Ignore it. */
        }
    }
    for (const auto &pair : disconnected_configs) {
        ql::datum_t db_name_or_uuid;
        if (!convert_database_id_to_datum(
                pair.second.database, identifier_format, metadata,
                &db_name_or_uuid, nullptr)) {
            db_name_or_uuid = ql::datum_t("__deleted_database__");
        }
        rows_out->push_back(make_error_row(
            pair.first, db_name_or_uuid, pair.second.name));
    }
    return true;
}

bool common_table_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor_on_caller,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
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
    try {
        table_basic_config_t basic_config;
        table_meta_client->get_name(table_id, &basic_config);

        ql::datum_t db_name_or_uuid;
        name_string_t db_name;
        if (!convert_database_id_to_datum(basic_config.database, identifier_format,
                metadata, &db_name_or_uuid, &db_name)) {
            db_name_or_uuid = ql::datum_t("__deleted_database__");
            db_name = name_string_t::guarantee_valid("__deleted_database__");
        }

        table_config_and_shards_t config;
        try {
            table_meta_client->get_config(table_id, &interruptor_on_home, &config);
        } catch (const failed_table_op_exc_t &) {
            *row_out = make_error_row(table_id, db_name_or_uuid, basic_config.name);
            return true;
        }

        format_row(table_id, config, db_name_or_uuid, &interruptor_on_home, row_out);
        return true;
    } catch (const no_such_table_exc_t &) {
        *row_out = ql::datum_t();
        return true;
    }
}

ql::datum_t common_table_artificial_table_backend_t::make_error_row(
        const namespace_id_t &table_id,
        const ql::datum_t &db_name_or_uuid,
        const name_string_t &table_name) {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(table_name));
    builder.overwrite("error",
        ql::datum_t("None of the replicas for this table are accessible."));
    return std::move(builder).to_datum();
}

