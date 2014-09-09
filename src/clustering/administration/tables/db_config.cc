// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/db_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"

ql::datum_t convert_db_config_and_name_to_datum(
        name_string_t db_name,
        namespace_id_t uuid) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(db_name));
    builder.overwrite("uuid", convert_uuid_to_datum(uuid));
    return std::move(builder).to_datum();
}

bool convert_db_config_and_name_from_datum(
        ql::datum_t datum,
        name_string_t *db_name_out,
        namespace_id_t *uuid_out,
        std::string *error_out) {
    /* In practice, the input will always be an object and the `uuid` field will always
    be valid, because `artificial_table_t` will check those thing before passing the
    row to `db_config_artificial_table_backend_t`. But we check them anyway for
    consistency. */
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t name_datum;
    if (!converter.get("name", &name_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(name_datum, "db name", db_name_out, error_out)) {
        *error_out = "In `name`: " + *error_out;
        return false;
    }

    ql::datum_t uuid_datum;
    if (!converter.get("uuid", &uuid_datum, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(uuid_datum, uuid_out, error_out)) {
        *error_out = "In `uuid`: " + *error_out;
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}


std::string db_config_artificial_table_backend_t::get_primary_key_name() {
    return "uuid";
}

bool db_config_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<ql::datum_t> *keys_out,
        UNUSED std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    keys_out->clear();
    databases_semilattice_metadata_t md = database_sl_view->get();
    for (auto it = md.databases.begin();
              it != md.databases.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        keys_out->push_back(convert_uuid_to_datum(it->first));
    }
    return true;
}

bool db_config_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    databases_semilattice_metadata_t md = database_sl_view->get();
    database_id_t database_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &database_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        database_id = nil_uuid();
    }
    auto it = md.databases.find(database_id);
    if (it == md.databases.end() || it->second.is_deleted()) {
        *row_out = ql::datum_t();
        return true;
    }
    name_string_t db_name = it->second.get_ref().name.get_ref();
    *row_out = convert_db_config_and_name_to_datum(db_name, database_id);
    return true;
}

bool db_config_artificial_table_backend_t::write_row(
        ql::datum_t primary_key,
        ql::datum_t new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    if (!new_value.has()) {
        *error_out = "It's illegal to delete from the `rethinkdb.db_config` table. "
            "To delete a database, use `r.db_drop()`.";
        return false;
    }
    on_thread_t thread_switcher(home_thread());
    databases_semilattice_metadata_t md = database_sl_view->get();
    namespace_id_t database_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &database_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        database_id = nil_uuid();
    }
    auto it = md.databases.find(database_id);
    if (it == md.databases.end() || it->second.is_deleted()) {
        *error_out = "It's illegal to insert into the `rethinkdb.db_config` table. "
            "To create a table, use `r.db_create()`.";
        return false;
    }

    name_string_t db_name = it->second.get_ref().name.get_ref();

    name_string_t new_db_name;
    namespace_id_t new_database_id;
    if (!convert_db_config_and_name_from_datum(new_value,
            &new_db_name, &new_database_id, error_out)) {
        *error_out = "The change you're trying to make to "
            "`rethinkdb.db_config` has the wrong format. " + *error_out;
        return false;
    }
    guarantee(new_database_id == database_id, "artificial_table_t should ensure that "
        "the primary key doesn't change.");

    if (new_db_name != db_name) {
        /* Prevent name collisions if possible */
        metadata_searcher_t<database_semilattice_metadata_t> ns_searcher(&md.databases);
        metadata_search_status_t status;
        ns_searcher.find_uniq(new_db_name, &status);
        if (status != METADATA_ERR_NONE) {
            *error_out = strprintf("Cannot rename database `%s` to `%s` because "
                "database `%s` already exists.", db_name.c_str(), new_db_name.c_str(),
                new_db_name.c_str());
            return false;
        }
    }

    it->second.get_mutable()->name.set(new_db_name);

    database_sl_view->join(md);

    return true;
}
