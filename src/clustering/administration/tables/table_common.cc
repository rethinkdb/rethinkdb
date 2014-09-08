// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "concurrency/cross_thread_signal.hpp"

std::string common_table_artificial_table_backend_t::get_primary_key_name() {
    return "uuid";
}

bool common_table_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<ql::datum_t> *keys_out,
        UNUSED std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    keys_out->clear();
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        keys_out->push_back(convert_uuid_to_datum(it->first));
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
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        table_id = nil_uuid();
    }
    auto it = md->namespaces.find(table_id);
    if (it == md->namespaces.end() || it->second.is_deleted()) {
        *row_out = ql::datum_t();
        return true;
    }
    name_string_t table_name = it->second.get_ref().name.get_ref();
    name_string_t db_name = get_db_name(it->second.get_ref().database.get_ref());
    return read_row_impl(table_id, table_name, db_name, it->second.get_ref(),
                         &interruptor2, row_out, error_out);
}

name_string_t common_table_artificial_table_backend_t::get_db_name(database_id_t db_id) {
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

