// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"

std::string common_table_artificial_table_backend_t::get_primary_key_name() {
    return "uuid";
}

bool common_table_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());
    rows_out->clear();
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }

        name_string_t table_name = it->second.get_ref().name.get_ref();
        name_string_t db_name = get_db_name(it->second.get_ref().database.get_ref());
        ql::datum_t row;
        if (!format_row(it->first, table_name, db_name, it->second.get_ref(),
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
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        table_id = nil_uuid();
    }
    std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t> >
        ::const_iterator it;
    if (search_const_metadata_by_uuid(&md->namespaces, table_id, &it)) {
        name_string_t table_name = it->second.get_ref().name.get_ref();
        name_string_t db_name = get_db_name(it->second.get_ref().database.get_ref());
        return format_row(table_id, table_name, db_name, it->second.get_ref(),
                          &interruptor2, row_out, error_out);
    } else {
        *row_out = ql::datum_t();
        return true;
    }
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

bool common_table_artificial_table_backend_t::get_db_id(name_string_t db_name,
        database_id_t *db_out, std::string *error_out) {
    assert_thread();
    databases_semilattice_metadata_t dbs = database_sl_view->get();
    metadata_searcher_t<database_semilattice_metadata_t> searcher(&dbs.databases);
    metadata_search_status_t status;
    auto db_it = searcher.find_uniq(db_name, &status);
    if (!check_metadata_status(status, "Database", db_name.str(), true, error_out)) {
        return false;
    }
    *db_out = db_it->first;
    return true;
}

