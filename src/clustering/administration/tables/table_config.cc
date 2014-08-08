// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/tables/elect_director.hpp"
#include "clustering/administration/tables/lookup.hpp"

counted_t<const ql::datum_t> convert_table_config_shard_to_datum(
        const table_config_t::shard_t &shard) {
    ql::datum_object_builder_t builder;

    if (shard.split_point) {
        builder.overwrite("split_point", make_counted<const ql::datum_t>(
            key_to_unescaped_str(*shard.split_point)
            ));
    }

    builder.overwrite("replicas", convert_set_to_datum<name_string_t>(
            &convert_name_to_datum,
            shard.replica_names));

    builder.overwrite("directors", convert_vector_to_datum<name_string_t>(
            &convert_name_to_datum,
            shard.director_names));

    return std::move(builder).to_counted();
}

bool convert_table_config_shard_from_datum(
        counted_t<const ql::datum_t> datum,
        table_config_t::shard_t *shard_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    counted_t<const ql::datum_t> split_point_datum;
    converter.get_optional("split_point", &split_point_datum);
    if (split_point_datum.has()) {
        if (split_point_datum->get_type() != ql::datum_t::R_STR) {
            *error_out = "In `split_point`: Expected a string, got " +
                split_point_datum->print();
            return false;
        }
        store_key_t split_point_value(split_point_datum->as_str().to_std());
        shard_out->split_point = boost::optional<store_key_t>(split_point_value);
    } else {
        shard_out->split_point = boost::optional<store_key_t>();
    }

    counted_t<const ql::datum_t> replica_names_datum;
    if (!converter.get("replicas", &replica_names_datum, error_out)) {
        return false;
    }
    if (replica_names_datum->get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "In `replicas`: Expected an array, got " +
            replica_names_datum->print();
        return false;
    }
    if (!convert_set_from_datum<name_string_t>(
            [] (counted_t<const ql::datum_t> datum2, name_string_t *val2_out,
                    std::string *error2_out) {
                return convert_name_from_datum(datum2, "server name", val2_out,
                    error2_out);
            },
            false,   /* raise an error if a server appears twice */
            replica_names_datum, &shard_out->replica_names, error_out)) {
        *error_out = "In `replicas`: " + *error_out;
        return false;
    }
    if (shard_out->replica_names.empty()) {
        *error_out = "You must specify at least one replica for each shard.";
        return false;
    }

    counted_t<const ql::datum_t> director_names_datum;
    if (!converter.get("directors", &director_names_datum, error_out)) {
        return false;
    }
    if (!convert_vector_from_datum<name_string_t>(
            [] (counted_t<const ql::datum_t> datum2, name_string_t *val2_out,
                    std::string *error2_out) {
                return convert_name_from_datum(datum2, "server name", val2_out,
                    error2_out);
            },
            director_names_datum,
            &shard_out->director_names, error_out)) {
        *error_out = "In `directors`: " + *error_out;
        return false;
    }
    if (shard_out->director_names.empty()) {
        *error_out = "You must specify at least one director for each shard.";
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    std::set<name_string_t> director_names_seen;
    for (const name_string_t &director : shard_out->director_names) {
        if (shard_out->replica_names.count(director) != 1) {
            *error_out = strprintf("Server `%s` appears in `directors` but not in "
                "`replicas`.", director.c_str());
            return false;
        }
        if (director_names_seen.count(director) != 0) {
            *error_out = strprintf("In `directors`: Server `%s` appears multiple times.",
                director.c_str());
            return false;
        }
        director_names_seen.insert(director);
    }

    return true;
}

counted_t<const ql::datum_t> convert_table_config_to_datum(
        const table_config_t &config,
        name_string_t db_name,
        name_string_t table_name,
        namespace_id_t uuid) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_db_and_table_to_datum(db_name, table_name));
    builder.overwrite("uuid", convert_uuid_to_datum(uuid));
    builder.overwrite("shards",
        convert_vector_to_datum<table_config_t::shard_t>(
            &convert_table_config_shard_to_datum,
            config.shards));
    return std::move(builder).to_counted();
}

bool convert_table_config_from_datum(
        counted_t<const ql::datum_t> datum,
        name_string_t expected_db_name,
        name_string_t expected_table_name,
        namespace_id_t expected_uuid,
        table_config_t *config_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        crash("artificial_table_t should confirm row is an object");
    }

    counted_t<const ql::datum_t> name_datum;
    if (!converter.get("name", &name_datum, error_out)) {
        crash("artificial_table_t should confirm primary key is unchanged");
    }
    name_string_t db_name_value, table_name_value;
    if (!convert_db_and_table_from_datum(name_datum, &db_name_value,
            &table_name_value, error_out)) {
        crash("artificial_table_t should confirm primary key is unchanged");
    }
    guarantee(db_name_value == expected_db_name,
        "artificial_table_t should confirm primary key is unchanged");
    guarantee(table_name_value == expected_table_name,
        "artificial_table_t should confirm primary key is unchanged");

    counted_t<const ql::datum_t> uuid_datum;
    if (!converter.get("uuid", &uuid_datum, error_out)) {
        return false;
    }
    uuid_u uuid_value;
    if (!convert_uuid_from_datum(uuid_datum, &uuid_value, error_out)) {
        *error_out = "It's illegal to modify a table's UUID";
        return false;
    }
    if (uuid_value != expected_uuid) {
        *error_out = "It's illegal to modify a table's UUID";
        return false;
    }

    counted_t<const ql::datum_t> shards_datum;
    if (!converter.get("shards", &shards_datum, error_out)) {
        return false;
    }
    if (!convert_vector_from_datum<table_config_t::shard_t>(
            &convert_table_config_shard_from_datum, shards_datum,
            &config_out->shards, error_out)) {
        *error_out = "In `shards`: " + *error_out;
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    store_key_t prev_split_point = store_key_t::min();
    for (size_t i = 0; i < config_out->shards.size() - 1; i++) {
        if (!config_out->shards[i].split_point) {
            *error_out = "Every shard except the last must have a split point.";
            return false;
        }
        store_key_t split_point = *config_out->shards[i].split_point;
        if (split_point < prev_split_point) {
            *error_out = "Shard split points must be monotonically increasing.";
            return false;
        }
        if (split_point == prev_split_point) {
            *error_out = "Shards must not be empty; i.e. split points must be distinct.";
            return false;
        }
    }
    if (config_out->shards.back().split_point) {
        *error_out = "The last shard must not have a split point.";
        return false;
    }

    return true;
}

std::string table_config_artificial_table_backend_t::get_primary_key_name() {
    return "name";
}

bool table_config_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<counted_t<const ql::datum_t> > *keys_out,
        std::string *error_out) {
    keys_out->clear();
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        if (it->second.get_ref().database.in_conflict() ||
                it->second.get_ref().name.in_conflict()) {
            /* TODO: Handle conflict differently */
            *error_out = "Metadata is in conflict";
            return false;
        }
        database_id_t db_id = it->second.get_ref().database.get_ref();
        auto jt = database_sl_view->get().databases.find(db_id);
        guarantee(jt != database_sl_view->get().databases.end());
        /* RSI(reql_admin): This can actually happen. We should handle this case. */
        guarantee(!jt->second.is_deleted());
        if (jt->second.get_ref().name.in_conflict()) {
            *error_out = "Metadata is in conflict";
            return false;
        }
        name_string_t db_name = jt->second.get_ref().name.get_ref();
        name_string_t table_name = it->second.get_ref().name.get_ref();
        /* TODO: How to handle table name collisions? */
        keys_out->push_back(convert_db_and_table_to_datum(db_name, table_name));
    }
    return true;
}

bool table_config_artificial_table_backend_t::read_row(
        counted_t<const ql::datum_t> primary_key,
        UNUSED signal_t *interruptor,
        counted_t<const ql::datum_t> *row_out,
        std::string *error_out) {
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    name_string_t db_name, table_name;
    std::string dummy_error;
    namespace_id_t table_id;
    if (!convert_db_and_table_from_datum(primary_key, &db_name, &table_name,
                                               &dummy_error)) {
        /* If the primary key was not a valid table name, then it must refer to a
        nonexistent row. 
        TODO: Would it be more user-friendly to error instead? */
        table_id = nil_uuid();
    } else {
        table_id = lookup_table_with_database(db_name, table_name,
            database_sl_view->get(), md);
    }
    if (table_id == nil_uuid()) {
        *row_out = counted_t<const ql::datum_t>();
        return true;
    }

    auto it = md->namespaces.find(table_id);
    if (it->second.get_ref().replication_info.in_conflict()) {
        *error_out = "Metadata is in conflict.";
        return false;
    }
    table_config_t config =
        it->second.get_ref().replication_info.get_ref().config;
    *row_out = convert_table_config_to_datum(config, db_name, table_name, it->first);
    return true;
}

bool table_config_artificial_table_backend_t::write_row(
        counted_t<const ql::datum_t> primary_key,
        counted_t<const ql::datum_t> new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    name_string_t db_name, table_name;
    std::string dummy_error;
    namespace_id_t table_id;
    if (!convert_db_and_table_from_datum(primary_key, &db_name, &table_name,
                                         &dummy_error)) {
        table_id = nil_uuid();
    } else {
        table_id = lookup_table_with_database(db_name, table_name,
            database_sl_view->get(), md);
    }
    if (table_id == nil_uuid()) {
        *error_out = "To create a table, you must use table_create() instead of "
            "inserting into the `rethinkdb.table_config` table.";
        return false;
    }
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t md_change(&md);
    auto it = md_change.get()->namespaces.find(table_id);
    table_replication_info_t replication_info;
    if (!convert_table_config_from_datum(new_value, db_name, table_name, it->first,
            &replication_info.config, error_out)) {
        *error_out = "The change you're trying to make to "
            "`rethinkdb.table_config` has the wrong format. " + *error_out;
        return false;
    }
    replication_info.chosen_directors =
        table_elect_directors(replication_info.config, name_client);
    it->second.get_mutable()->replication_info =
        it->second.get_ref().replication_info.make_resolving_version(
            replication_info, my_machine_id);
    table_sl_view->join(md);
    return true;
}


