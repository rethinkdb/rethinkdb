// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/tables/split_points.hpp"
#include "concurrency/cross_thread_signal.hpp"

ql::datum_t convert_table_config_shard_to_datum(
        const table_config_t::shard_t &shard) {
    ql::datum_object_builder_t builder;

    builder.overwrite("replicas", convert_set_to_datum<name_string_t>(
            &convert_name_to_datum,
            shard.replica_names));

    builder.overwrite("director", convert_name_to_datum(shard.director_name));

    return std::move(builder).to_datum();
}

bool convert_table_config_shard_from_datum(
        ql::datum_t datum,
        table_config_t::shard_t *shard_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t replica_names_datum;
    if (!converter.get("replicas", &replica_names_datum, error_out)) {
        return false;
    }
    if (replica_names_datum.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "In `replicas`: Expected an array, got " +
            replica_names_datum.print();
        return false;
    }
    if (!convert_set_from_datum<name_string_t>(
            [] (ql::datum_t datum2, name_string_t *val2_out,
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

    ql::datum_t director_name_datum;
    if (!converter.get("director", &director_name_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(director_name_datum, "server name",
            &shard_out->director_name, error_out)) {
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    if (shard_out->replica_names.count(shard_out->director_name) != 1) {
        *error_out = strprintf("Server `%s` is listed as `director` but does not appear "
            "in `replicas`.", shard_out->director_name.c_str());
        return false;
    }

    return true;
}

/* This is separate from `convert_table_config_and_name_to_datum` because it needs to be
publicly exposed so it can be used to create the return value of `table.reconfigure()`.
*/
ql::datum_t convert_table_config_to_datum(
        const table_config_t &config) {
    ql::datum_object_builder_t builder;
    builder.overwrite("shards",
        convert_vector_to_datum<table_config_t::shard_t>(
            &convert_table_config_shard_to_datum,
            config.shards));
    return std::move(builder).to_datum();
}

ql::datum_t convert_table_config_and_name_to_datum(
        const table_config_t &config,
        name_string_t table_name,
        name_string_t db_name,
        namespace_id_t uuid,
        const std::string &primary_key) {
    ql::datum_t start = convert_table_config_to_datum(config);
    ql::datum_object_builder_t builder(start);
    builder.overwrite("name", convert_name_to_datum(table_name));
    builder.overwrite("db", convert_name_to_datum(db_name));
    builder.overwrite("uuid", convert_uuid_to_datum(uuid));
    builder.overwrite("primary_key", convert_string_to_datum(primary_key));
    return std::move(builder).to_datum();
}

bool convert_table_config_and_name_from_datum(
        ql::datum_t datum,
        name_string_t *table_name_out,
        name_string_t *db_name_out,
        namespace_id_t *uuid_out,
        table_config_t *config_out,
        std::string *primary_key_out,
        std::string *error_out) {
    /* In practice, the input will always be an object and the `uuid` field will always
    be valid, because `artificial_table_t` will check those thing before passing the
    row to `table_config_artificial_table_backend_t`. But we check them anyway for
    consistency. */
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t name_datum;
    if (!converter.get("name", &name_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(name_datum, "table name", table_name_out, error_out)) {
        *error_out = "In `name`: " + *error_out;
        return false;
    }

    ql::datum_t db_datum;
    if (!converter.get("db", &db_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(db_datum, "database name", db_name_out, error_out)) {
        *error_out = "In `db`: " + *error_out;
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

    ql::datum_t primary_key_datum;
    if (!converter.get("primary_key", &primary_key_datum, error_out)) {
        return false;
    }
    if (!convert_string_from_datum(primary_key_datum, primary_key_out, error_out)) {
        *error_out = "In `primary_key`: " + *error_out;
        return false;
    }

    ql::datum_t shards_datum;
    if (!converter.get("shards", &shards_datum, error_out)) {
        return false;
    }
    if (!convert_vector_from_datum<table_config_t::shard_t>(
            &convert_table_config_shard_from_datum, shards_datum,
            &config_out->shards, error_out)) {
        *error_out = "In `shards`: " + *error_out;
        return false;
    }
    if (config_out->shards.empty()) {
        *error_out = "In `shards`: You must specify at least one shard.";
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
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
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    assert_thread();
    *row_out = convert_table_config_and_name_to_datum(
        metadata.replication_info.get_ref().config,
        table_name, db_name, table_id, metadata.primary_key.get_ref());
    return true;
}

bool table_config_artificial_table_backend_t::write_row(
        ql::datum_t primary_key,
        bool pkey_was_autogenerated,
        ql::datum_t new_value,
        signal_t *interruptor,
        std::string *error_out) {
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Look for an existing table with the given UUID */
    cow_ptr_t<namespaces_semilattice_metadata_t> md = table_sl_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        guarantee(!pkey_was_autogenerated, "auto-generated primary key should have been "
            "a valid UUID string.");
        table_id = nil_uuid();
    }
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t md_change(&md);
    auto it = md_change.get()->namespaces.find(table_id);


    if (new_value.has()) {
        /* We're creating a new table or updating an existing one */

        /* Parse the new value the user provided for the table */
        table_replication_info_t replication_info;
        name_string_t new_table_name;
        name_string_t new_db_name;
        namespace_id_t new_table_id;
        std::string new_primary_key;
        if (!convert_table_config_and_name_from_datum(new_value,
                &new_table_name, &new_db_name, &new_table_id,
                &replication_info.config, &new_primary_key, error_out)) {
            *error_out = "The change you're trying to make to "
                "`rethinkdb.table_config` has the wrong format. " + *error_out;
            return false;
        }
        guarantee(new_table_id == table_id, "artificial_table_t should ensure that the "
            "primary key doesn't change.");

        /* The way we handle the `db` field is a bit convoluted, but for good reason. If
        we're updating an existing table, we require that the DB name is the same as it
        is before. By not looking up the DB's UUID, we avoid any problems if there is a
        DB name collision or if the DB was deleted. If we're creating a new table, only
        then do we actually look up the DB's UUID. */

        /* Determine whether we're creating a new table or updating an existing one */
        name_string_t old_table_name;
        database_id_t old_db_id;
        bool is_creation;
        if (it != md_change.get()->namespaces.end() && !it->second.is_deleted()) {
            guarantee(!pkey_was_autogenerated, "UUID collision happened");
            is_creation = false;
            old_table_name = it->second.get_ref().name.get_ref();
            old_db_id = it->second.get_ref().database.get_ref();
            if (new_db_name != get_db_name(old_db_id)) {
                *error_out = "It's illegal to change a table's `database` field.";
                return false;
            }
            if (new_primary_key != it->second.get_ref().primary_key.get_ref()) {
                *error_out = "It's illegal to change a table's primary key.";
                return false;
            }
        } else {
            is_creation = true;
            if (!pkey_was_autogenerated) {
                *error_out = "If you want to create a new table by inserting into "
                    "`rethinkdb.table_config`, you must use an auto-generated primary "
                    "key.";
                return false;
            }
            guarantee(it == md_change.get()->namespaces.end(),
                "UUID collision happened");
        }

        database_id_t new_db_id;
        if (is_creation) {
            if (!get_db_id(new_db_name, &new_db_id, error_out)) {
                return false;
            }
        } else {
            new_db_id = old_db_id;
        }

        /* Decide on the sharding scheme for the table */
        if (is_creation) {
            if (replication_info.config.shards.size() != 1) {
                *error_out = "Newly created tables must start with exactly one shard";
                return false;
            }
            replication_info.shard_scheme = table_shard_scheme_t::one_shard();
        } else {
            table_replication_info_t prev =
                it->second.get_mutable()->replication_info.get_ref();
            if (!calculate_split_points_intelligently(table_id, reql_cluster_interface,
                    replication_info.config.shards.size(), prev.shard_scheme,
                    &interruptor2, &replication_info.shard_scheme, error_out)) {
                return false;
            }
        }

        if (is_creation || new_table_name != old_table_name) {
            /* Prevent name collisions if possible */
            metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
                &md_change.get()->namespaces);
            metadata_search_status_t status;
            namespace_predicate_t pred(&new_table_name, &new_db_id);
            ns_searcher.find_uniq(pred, &status);
            if (status != METADATA_ERR_NONE) {
                if (is_creation) {
                    *error_out = strprintf("Table `%s.%s` already exists.",
                        new_db_name.c_str(), new_table_name.c_str());
                } else {
                    *error_out = strprintf("Cannot rename table `%s.%s` to `%s.%s` "
                        "because table `%s.%s` already exists.", new_db_name.c_str(),
                        old_table_name.c_str(), new_db_name.c_str(),
                        new_table_name.c_str(), new_db_name.c_str(),
                        new_table_name.c_str());
                }
                return false;
            }
        }

        /* Update `md`. The change will be committed to the semilattices at the end of
        this function. */
        if (is_creation) {
            namespace_semilattice_metadata_t table_md;
            table_md.name = versioned_t<name_string_t>(new_table_name);
            table_md.database = versioned_t<database_id_t>(new_db_id);
            table_md.primary_key = versioned_t<std::string>(new_primary_key);
            table_md.replication_info =
                versioned_t<table_replication_info_t>(replication_info);
            md_change.get()->namespaces[table_id] =
                deletable_t<namespace_semilattice_metadata_t>(table_md);
        } else {
            it->second.get_mutable()->name.set(new_table_name);
            it->second.get_mutable()->replication_info.set(replication_info);
        }

    } else {
        /* We're deleting a table (or it was already deleted) */
        if (it != md_change.get()->namespaces.end() && !it->second.is_deleted()) {
            guarantee(!pkey_was_autogenerated, "UUID collision happened");
            it->second.mark_deleted();
        }
    }

    table_sl_view->join(md);

    return true;
}


