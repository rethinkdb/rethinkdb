// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/tables/elect_director.hpp"

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
        name_string_t table_name,
        name_string_t db_name,
        namespace_id_t uuid) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(table_name));
    builder.overwrite("db", convert_name_to_datum(db_name));
    builder.overwrite("uuid", convert_uuid_to_datum(uuid));
    builder.overwrite("shards",
        convert_vector_to_datum<table_config_t::shard_t>(
            &convert_table_config_shard_to_datum,
            config.shards));
    return std::move(builder).to_counted();
}

bool convert_table_config_from_datum(
        counted_t<const ql::datum_t> datum,
        name_string_t *table_name_out,
        name_string_t *db_name_out,
        namespace_id_t *uuid_out,
        table_config_t *config_out,
        std::string *error_out) {
    /* In practice, the input will always be an object and the `uuid` field will always
    be valid, because `artificial_table_t` will check those thing before passing the
    row to `table_config_artificial_table_backend_t`. But we check them anyway for
    consistency. */
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    counted_t<const ql::datum_t> name_datum;
    if (!converter.get("name", &name_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(name_datum, "table name", table_name_out, error_out)) {
        *error_out = "In `name`: " + *error_out;
        return false;
    }

    counted_t<const ql::datum_t> db_datum;
    if (!converter.get("db", &db_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(db_datum, "database name", db_name_out, error_out)) {
        *error_out = "In `db`: " + *error_out;
        return false;
    }

    counted_t<const ql::datum_t> uuid_datum;
    if (!converter.get("uuid", &uuid_datum, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(uuid_datum, uuid_out, error_out)) {
        *error_out = "In `uuid`: " + *error_out;
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

bool table_config_artificial_table_backend_t::read_row_impl(
        namespace_id_t table_id,
        name_string_t table_name,
        name_string_t db_name,
        const namespace_semilattice_metadata_t &metadata,
        UNUSED signal_t *interruptor,
        counted_t<const ql::datum_t> *row_out,
        UNUSED std::string *error_out) {
    *row_out = convert_table_config_to_datum(
        metadata.replication_info.get_ref().config,
        table_name, db_name, table_id);
    return true;
}

bool table_config_artificial_table_backend_t::write_row(
        counted_t<const ql::datum_t> primary_key,
        counted_t<const ql::datum_t> new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    if (!new_value.has()) {
        *error_out = "It's illegal to delete from the `rethinkdb.table_config` table. "
            "To delete a table, use `r.table_drop()`.";
        return false;
    }
    on_thread_t thread_switcher(home_thread());
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        table_id = nil_uuid();
    }
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t md_change(&md);
    auto it = md_change.get()->namespaces.find(table_id);
    if (it == md->namespaces.end()) {
        *error_out = "It's illegal to insert into the `rethinkdb.table_config` table. "
            "To create a table, use `r.table_create()`.";
        return false;
    }

    table_replication_info_t replication_info;
    name_string_t new_table_name;
    name_string_t new_db_name;
    namespace_id_t new_table_id;
    if (!convert_table_config_from_datum(new_value,
            &new_table_name, &new_db_name, &new_table_id, &replication_info.config,
            error_out)) {
        *error_out = "The change you're trying to make to "
            "`rethinkdb.table_config` has the wrong format. " + *error_out;
        return false;
    }
    guarantee(new_table_id == table_id, "artificial_table_t should ensure that the "
        "primary key doesn't change.");
    if (new_db_name != get_db_name(it->second.get_ref().database.get_ref())) {
        *error_out = "It's illegal to change a table's `database` field.";
        return false;
    }

    replication_info.chosen_directors =
        table_elect_directors(replication_info.config, name_client);

    it->second.get_mutable()->name =
        it->second.get_ref().name.make_resolving_version(new_table_name, my_machine_id);
    it->second.get_mutable()->replication_info =
        it->second.get_ref().replication_info.make_resolving_version(
            replication_info, my_machine_id);
    table_sl_view->join(md);

    return true;
}


