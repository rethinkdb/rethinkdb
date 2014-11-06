// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/tables/generate_config.hpp"
#include "clustering/administration/tables/split_points.hpp"
#include "concurrency/cross_thread_signal.hpp"

bool lookup_server(
        name_string_t name,
        server_name_client_t *name_client,
        server_id_t *server_id_out,
        std::string *error_out) {
    bool ok;
    name_client->get_name_to_server_id_map()->apply_read(
        [&](const std::multimap<name_string_t, server_id_t> *map) {
            if (map->count(name) == 0) {
                *error_out = strprintf("Server `%s` does not exist.", name.c_str());
                ok = false;
            } else if (map->count(name) > 1) {
                *error_out = strprintf("Server `%s` is ambiguous; there are multiple "
                    "servers with that name.", name.c_str());
                ok = false;
            } else {
                *server_id_out = map->find(name)->second;
                ok = true;
            }
        });
    return ok;
}

ql::datum_t convert_replica_list_to_datum(
        const std::set<server_id_t> &replicas,
        server_name_client_t *name_client) {
    ql::datum_array_builder_t replicas_builder(ql::configured_limits_t::unlimited);
    for (const server_id_t &replica : replicas) {
        boost::optional<name_string_t> name =
            name_client->get_name_for_server_id(replica);
        if (static_cast<bool>(name)) {
            replicas_builder.add(convert_name_to_datum(*name));
        }
    }
    return std::move(replicas_builder).to_datum();
}

bool convert_replica_list_from_datum(
        const ql::datum_t &datum,
        server_name_client_t *name_client,
        std::set<server_id_t> *replicas_out,
        std::string *error_out) {
    if (datum.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "Expected an array, got " + datum.print();
        return false;
    }
    replicas_out->clear();
    for (size_t i = 0; i < datum.arr_size(); ++i) {
        name_string_t name;
        if (!convert_name_from_datum(datum.get(i), "server name", &name, error_out)) {
            return false;
        }
        server_id_t server_id;
        if (!lookup_server(name, name_client, &server_id, error_out)) {
            return false;
        }
        auto pair = replicas_out->insert(server_id);
        if (!pair.second) {
            *error_out = strprintf("Server `%s` is listed more than once.",
                name.c_str());
            return false;
        }
    }
    return true;
}

ql::datum_t convert_write_ack_config_req_to_datum(
        const write_ack_config_t::req_t &req,
        server_name_client_t *name_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("replicas",
        convert_replica_list_to_datum(req.replicas, name_client));
    const char *acks =
        (req.mode == write_ack_config_t::mode_t::majority) ? "majority" : "single";
    builder.overwrite("acks", ql::datum_t(acks));
    return std::move(builder).to_datum();
}

bool convert_write_ack_config_req_from_datum(
        const ql::datum_t &datum,
        server_name_client_t *name_client,
        write_ack_config_t::req_t *req_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t replica_names_datum;
    if (!converter.get("replicas", &replica_names_datum, error_out)) {
        return false;
    }
    if (!convert_replica_list_from_datum(replica_names_datum, name_client,
            &req_out->replicas, error_out)) {
        return false;
    }

    ql::datum_t acks_datum;
    if (!converter.get("acks", &acks_datum, error_out)) {
        return false;
    }
    if (acks_datum == ql::datum_t("single")) {
        req_out->mode = write_ack_config_t::mode_t::single;
    } else if (acks_datum == ql::datum_t("majority")) {
        req_out->mode = write_ack_config_t::mode_t::majority;
    } else {
        *error_out = "In `acks`: Expected 'single' or 'majority', got: " +
            acks_datum.print();
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}

ql::datum_t convert_write_ack_config_to_datum(
        const write_ack_config_t &config,
        server_name_client_t *name_client) {
    if (config.mode == write_ack_config_t::mode_t::single) {
        return ql::datum_t("single");
    } else if (config.mode == write_ack_config_t::mode_t::majority) {
        return ql::datum_t("majority");
    } else {
        return convert_vector_to_datum<write_ack_config_t::req_t>(
            [&](const write_ack_config_t::req_t &req) {
                return convert_write_ack_config_req_to_datum(req, name_client);
            }, config.complex_reqs);
    }
}

bool convert_write_ack_config_from_datum(
        const ql::datum_t &datum,
        server_name_client_t *name_client,
        write_ack_config_t *config_out,
        std::string *error_out) {
    if (datum == ql::datum_t("single")) {
        config_out->mode = write_ack_config_t::mode_t::single;
        config_out->complex_reqs.clear();
    } else if (datum == ql::datum_t("majority")) {
        config_out->mode = write_ack_config_t::mode_t::majority;
        config_out->complex_reqs.clear();
    } else if (datum.get_type() == ql::datum_t::R_ARRAY) {
        config_out->mode = write_ack_config_t::mode_t::complex;
        if (!convert_vector_from_datum<write_ack_config_t::req_t>(
                [&](const ql::datum_t &datum_2, write_ack_config_t::req_t *req_out,
                        std::string *error_out_2) {
                    return convert_write_ack_config_req_from_datum(
                        datum_2, name_client, req_out, error_out_2);
                }, datum, &config_out->complex_reqs, error_out)) {
            return false;
        }
    } else {
        *error_out = "Expected `single`, `majority`, or a list of ack requirements, but "
            "instead got: " + datum.print();
        return false;
    }
    return true;
}

ql::datum_t convert_durability_to_datum(
        write_durability_t durability) {
    switch (durability) {
        case write_durability_t::SOFT:
            return ql::datum_t("soft");
        case write_durability_t::HARD:
            return ql::datum_t("hard");
        case write_durability_t::INVALID:
        default:
            unreachable();
    }
}

bool convert_durability_from_datum(
        const ql::datum_t &datum,
        write_durability_t *durability_out,
        std::string *error_out) {
    if (datum == ql::datum_t("soft")) {
        *durability_out = write_durability_t::SOFT;
    } else if (datum == ql::datum_t("hard")) {
        *durability_out = write_durability_t::HARD;
    } else {
        *error_out = "Expected \"soft\" or \"hard\", got: " + datum.print();
        return false;
    }
    return true;
}

ql::datum_t convert_table_config_shard_to_datum(
        const table_config_t::shard_t &shard,
        server_name_client_t *name_client) {
    ql::datum_object_builder_t builder;

    builder.overwrite("replicas",
        convert_replica_list_to_datum(shard.replicas, name_client));

    boost::optional<name_string_t> director_name =
        name_client->get_name_for_server_id(shard.director);
    if (static_cast<bool>(director_name)) {
        builder.overwrite("director", convert_name_to_datum(*director_name));
    } else {
        /* If the previous director was declared dead, just display `null`. The user will
        have to change this to a new server before the table will come back online. */
        builder.overwrite("director", ql::datum_t::null());
    }

    return std::move(builder).to_datum();
}

bool convert_table_config_shard_from_datum(
        ql::datum_t datum,
        server_name_client_t *name_client,
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
    if (!convert_replica_list_from_datum(replica_names_datum, name_client,
            &shard_out->replicas, error_out)) {
        *error_out = "In `replicas`: " + *error_out;
        return false;
    }
    if (shard_out->replicas.empty()) {
        *error_out = "You must specify at least one replica for each shard.";
        return false;
    }

    ql::datum_t director_name_datum;
    if (!converter.get("director", &director_name_datum, error_out)) {
        return false;
    }
    if (director_name_datum.get_type() == ql::datum_t::R_NULL) {
        /* There's never a good reason for the user to intentionally set the director to
        `null`; setting the director to `null` will ensure that the table cannot accept
        queries. We allow it because if the director is declared dead, it will appear to
        the user as `null`; and we want to allow the user to do things like
        `r.table_config("foo").update({"name": "bar"})` even when the director is in that
        state. */
        shard_out->director = nil_uuid();
    } else {
        name_string_t director_name;
        if (!convert_name_from_datum(director_name_datum, "server name", &director_name,
                error_out)) {
            *error_out = "In `director`: " + *error_out;
            return false;
        }
        if (!lookup_server(director_name, name_client, &shard_out->director, error_out)) {
            *error_out = "In `director`: " + *error_out;
            return false;
        }
        if (shard_out->replicas.count(shard_out->director) != 1) {
            *error_out = strprintf("Server `%s` is listed as `director` but does not "
                "appear in `replicas`.", director_name.c_str());
            return false;
        }
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}

/* This is separate from `format_row()` because it needs to be publicly exposed so it can
   be used to create the return value of `table.reconfigure()`. */
ql::datum_t convert_table_config_to_datum(
        const table_config_t &config,
        server_name_client_t *name_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("shards",
        convert_vector_to_datum<table_config_t::shard_t>(
            [&](const table_config_t::shard_t &shard) {
                return convert_table_config_shard_to_datum(shard, name_client);
            },
            config.shards));
    builder.overwrite("write_acks",
        convert_write_ack_config_to_datum(config.write_ack_config, name_client));
    builder.overwrite("durability",
        convert_durability_to_datum(config.durability));
    return std::move(builder).to_datum();
}

bool table_config_artificial_table_backend_t::format_row(
        namespace_id_t table_id,
        name_string_t table_name,
        name_string_t db_name,
        const namespace_semilattice_metadata_t &metadata,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    assert_thread();

    ql::datum_t start = convert_table_config_to_datum(
        metadata.replication_info.get_ref().config, name_client);
    ql::datum_object_builder_t builder(start);
    builder.overwrite("name", convert_name_to_datum(table_name));
    builder.overwrite("db", convert_name_to_datum(db_name));
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite(
        "primary_key", convert_string_to_datum(metadata.primary_key.get_ref()));
    *row_out = std::move(builder).to_datum();

    return true;
}

bool convert_table_config_and_name_from_datum(
        ql::datum_t datum,
        bool existed_before,
        const namespaces_semilattice_metadata_t &all_table_metadata,
        server_name_client_t *name_client,
        signal_t *interruptor,
        name_string_t *table_name_out,
        name_string_t *db_name_out,
        namespace_id_t *id_out,
        table_config_t *config_out,
        std::string *primary_key_out,
        std::string *error_out) {
    /* In practice, the input will always be an object and the `id` field will always
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

    ql::datum_t id_datum;
    if (!converter.get("id", &id_datum, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(id_datum, id_out, error_out)) {
        *error_out = "In `id`: " + *error_out;
        return false;
    }

    if (existed_before || converter.has("primary_key")) {
        ql::datum_t primary_key_datum;
        if (!converter.get("primary_key", &primary_key_datum, error_out)) {
            return false;
        }
        if (!convert_string_from_datum(primary_key_datum, primary_key_out, error_out)) {
            *error_out = "In `primary_key`: " + *error_out;
            return false;
        }
    } else {
        *primary_key_out = "id";
    }

    if (existed_before || converter.has("shards")) {
        ql::datum_t shards_datum;
        if (!converter.get("shards", &shards_datum, error_out)) {
            return false;
        }
        if (!convert_vector_from_datum<table_config_t::shard_t>(
                [&](ql::datum_t shard_datum, table_config_t::shard_t *shard_out,
                        std::string *error_out_2) {
                    return convert_table_config_shard_from_datum(
                        shard_datum, name_client, shard_out, error_out_2);
                },
                shards_datum,
                &config_out->shards,
                error_out)) {
            *error_out = "In `shards`: " + *error_out;
            return false;
        }
        if (config_out->shards.empty()) {
            *error_out = "In `shards`: You must specify at least one shard.";
            return false;
        }
    } else {
        std::map<server_id_t, int> server_usage;
        for (const auto &pair : all_table_metadata.namespaces) {
            if (pair.second.is_deleted()) {
                continue;
            }
            calculate_server_usage(
                pair.second.get_ref().replication_info.get_ref().config, &server_usage);
        }
        if (!table_generate_config(
                name_client, nil_uuid(), nullptr, server_usage,
                table_generate_config_params_t::make_default(), table_shard_scheme_t(),
                interruptor, config_out, error_out)) {
            *error_out = "When generating configuration for new table: " + *error_out;
            return false;
        }
    }

    if (existed_before || converter.has("write_acks")) {
        ql::datum_t write_acks_datum;
        if (!converter.get("write_acks", &write_acks_datum, error_out)) {
            return false;
        }
        if (!convert_write_ack_config_from_datum(write_acks_datum, name_client,
                &config_out->write_ack_config, error_out)) {
            *error_out = "In `write_acks`: " + *error_out;
            return false;
        }
    } else {
        config_out->write_ack_config.mode = write_ack_config_t::mode_t::majority;
        config_out->write_ack_config.complex_reqs.clear();
    }

    if (existed_before || converter.has("durability")) {
        ql::datum_t durability_datum;
        if (!converter.get("durability", &durability_datum, error_out)) {
            return false;
        }
        if (!convert_durability_from_datum(durability_datum, &config_out->durability,
                error_out)) {
            *error_out = "In `durability`: " + *error_out;
            return false;
        }
    } else {
        config_out->durability = write_durability_t::HARD;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}

bool table_config_artificial_table_backend_t::write_row(
        ql::datum_t primary_key,
        bool pkey_was_autogenerated,
        ql::datum_t *new_value_inout,
        signal_t *interruptor,
        std::string *error_out) {
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Look for an existing table with the given UUID */
    cluster_semilattice_metadata_t md = semilattice_view->get();
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        guarantee(!pkey_was_autogenerated, "auto-generated primary key should have been "
            "a valid UUID string.");
        table_id = nil_uuid();
    }
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t md_change(&md.rdb_namespaces);
    std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t> >
        ::iterator it;
    bool existed_before = search_metadata_by_uuid(
        &md_change.get()->namespaces, table_id, &it);


    if (new_value_inout->has()) {
        /* We're updating an existing table (if `existed_before == true`) or creating
        a new one (if `existed_before == false`) */

        /* Parse the new value the user provided for the table */
        table_replication_info_t replication_info;
        name_string_t new_table_name;
        name_string_t new_db_name;
        namespace_id_t new_table_id;
        std::string new_primary_key;
        if (!convert_table_config_and_name_from_datum(*new_value_inout, existed_before,
                *md_change.get(), name_client, interruptor,
                &new_table_name, &new_db_name, &new_table_id,
                &replication_info.config, &new_primary_key, error_out)) {
            *error_out = "The change you're trying to make to "
                "`rethinkdb.table_config` has the wrong format. " + *error_out;
            return false;
        }
        guarantee(new_table_id == table_id, "artificial_table_t should ensure that the "
            "primary key doesn't change.");

        if (existed_before) {
            guarantee(!pkey_was_autogenerated, "UUID collision happened");
        } else {
            if (!pkey_was_autogenerated) {
                *error_out = "If you want to create a new table by inserting into "
                    "`rethinkdb.table_config`, you must use an auto-generated primary "
                    "key.";
                return false;
            }
            /* Assert that we didn't randomly generate the UUID of a table that used to
            exist but was deleted */
            guarantee(md_change.get()->namespaces.count(table_id) == 0,
                "UUID collision happened");
        }

        /* The way we handle the `db` field is a bit convoluted, but for good reason. If
        we're updating an existing table, we require that the DB name is the same as it
        is before. By not looking up the DB's UUID, we avoid any problems if there is a
        DB name collision or if the DB was deleted. If we're creating a new table, only
        then do we actually look up the DB's UUID. */
        database_id_t db_id;
        if (existed_before) {
            db_id = it->second.get_ref().database.get_ref();
            if (new_db_name != get_db_name(db_id)) {
                *error_out = "It's illegal to change a table's `database` field.";
                return false;
            }
        } else {
            if (!get_db_id(new_db_name, &db_id, error_out)) {
                return false;
            }
        }

        if (existed_before) {
            if (new_primary_key != it->second.get_ref().primary_key.get_ref()) {
                *error_out = "It's illegal to change a table's primary key.";
                return false;
            }
        }

        /* Decide on the sharding scheme for the table */
        if (existed_before) {
            table_replication_info_t prev =
                it->second.get_mutable()->replication_info.get_ref();
            if (!calculate_split_points_intelligently(table_id, reql_cluster_interface,
                    replication_info.config.shards.size(), prev.shard_scheme,
                    &interruptor2, &replication_info.shard_scheme, error_out)) {
                return false;
            }
        } else {
            if (replication_info.config.shards.size() != 1) {
                *error_out = "Newly created tables must start with exactly one shard";
                return false;
            }
            replication_info.shard_scheme = table_shard_scheme_t::one_shard();
        }

        name_string_t old_table_name;
        if (existed_before) {
            old_table_name = it->second.get_ref().name.get_ref();
        }

        if (!existed_before || new_table_name != old_table_name) {
            /* Prevent name collisions if possible */
            metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
                &md_change.get()->namespaces);
            metadata_search_status_t status;
            namespace_predicate_t pred(&new_table_name, &db_id);
            ns_searcher.find_uniq(pred, &status);
            if (status != METADATA_ERR_NONE) {
                if (!existed_before) {
                    /* This message looks weird in the context of the variable named
                    `existed_before`, but it's correct. `existed_before` is true if a
                    table with the specified UUID already exists; but we're showing the
                    user an error if a table with the specified name already exists. */
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
        if (existed_before) {
            it->second.get_mutable()->name.set(new_table_name);
            it->second.get_mutable()->replication_info.set(replication_info);
        } else {
            namespace_semilattice_metadata_t table_md;
            table_md.name = versioned_t<name_string_t>(new_table_name);
            table_md.database = versioned_t<database_id_t>(db_id);
            table_md.primary_key = versioned_t<std::string>(new_primary_key);
            table_md.replication_info =
                versioned_t<table_replication_info_t>(replication_info);
            md_change.get()->namespaces[table_id] =
                deletable_t<namespace_semilattice_metadata_t>(table_md);
        }

        /* Because we might have filled in the `primary_key` and `shards` fields, we need
        to write back to `new_value_inout` */
        if (!format_row(table_id, new_table_name, new_db_name,
                md_change.get()->namespaces[table_id].get_ref(),
                interruptor, new_value_inout, error_out)) {
            return false;
        }

    } else {
        /* We're deleting a table (or it was already deleted) */
        if (existed_before) {
            guarantee(!pkey_was_autogenerated, "UUID collision happened");
            it->second.mark_deleted();
        }
    }

    semilattice_view->join(md);

    return true;
}


