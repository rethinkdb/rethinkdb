// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"

std::string common_table_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool common_table_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());
    rows_out->clear();
    cluster_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md.rdb_namespaces->namespaces.begin();
              it != md.rdb_namespaces->namespaces.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        name_string_t table_name;
        ql::datum_t db_name_or_uuid;
        bool ok = convert_table_id_to_datums(it->first, identifier_format, md,
                nullptr, &table_name, &db_name_or_uuid, nullptr);
        guarantee(ok, "we found this table by iterating, so it must exist.");
        ql::datum_t row;
        if (!format_row(it->first, table_name, db_name_or_uuid, it->second.get_ref(),
                        &interruptor2, &row, error_out)) {
            return false;
        }
        rows_out->push_back(row);
    }
    return true;
}

bool common_table_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());
    cluster_semilattice_metadata_t md = semilattice_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        table_id = nil_uuid();
    }
    name_string_t table_name;
    ql::datum_t db_name_or_uuid;
    if (!convert_table_id_to_datums(table_id, identifier_format, md,
            nullptr, &table_name, &db_name_or_uuid, nullptr)) {
        *row_out = ql::datum_t();
        return true;
    }
    auto it = md.rdb_namespaces->namespaces.find(table_id);
    guarantee(it != md.rdb_namespaces->namespaces.end() && !it->second.is_deleted());
    return format_row(table_id, table_name, db_name_or_uuid, it->second.get_ref(),
                      &interruptor2, row_out, error_out);
}

#if 0
RSI: remove me completely
name_string_t common_table_artificial_table_backend_t::get_db_name(
        const database_id_t &db_id) {
    assert_thread();
    databases_semilattice_metadata_t dbs = semilattice_view->get().databases;
    auto it = dbs.databases.find(db_id);
    guarantee(it != dbs.databases.end());
    if (it->second.is_deleted()) {
        return name_string_t::guarantee_valid("__deleted_database__");
    } else {
        return it->second.get_ref().name.get_ref();
    }
}

ql::datum_t common_table_artificial_table_backend_t::get_db_name_or_uuid(
        const database_id_t &db_id) {
    assert_thread();
    databases_semilattice_metadata_t dbs = semilattice_view->get().databases;
    ql::datum_t res;
    if (!convert_database_id_to_datum(db_id, identifier_format, dbs, &res)) {
        /* This can occur due to a race condition, if a new table is added to a database
        at the same time as it is being deleted. */
        res = convert_name_to_datum(
            name_string_t::guarantee_valid("__deleted_database__"));
    }
    return res;
}
#endif

