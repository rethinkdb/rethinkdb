// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/tables/generate_config.hpp"
#include "clustering/administration/tables/split_points.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "concurrency/cross_thread_signal.hpp"

table_config_artificial_table_backend_t::~table_config_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

bool convert_server_id_from_datum(
        const ql::datum_t &datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        const server_name_map_t &old_server_names,
        server_id_t *server_id_out,
        server_name_map_t *server_names_out,
        admin_err_t *error_out) {
    if (identifier_format == admin_identifier_format_t::name) {
        name_string_t name;
        if (!convert_name_from_datum(datum, "server name", &name, error_out)) {
            return false;
        }
        /* First we look up the server in `old_server_names`; if that fails, we try again
        in `server_config_client->get_server_config_map()`. We prefer `old_server_names`
        because that will reduce the likelihood of an accidental change if the servers'
        names have changed recently. */
        size_t count = 0;
        for (const auto &pair : old_server_names.names) {
            if (pair.second.second == name) {
                *server_id_out = pair.first;
                server_names_out->names.insert(pair);
                ++count;
            }
        }
        if (count == 0) {
            server_config_client->get_server_config_map()->read_all(
            [&](const server_id_t &server_id, const server_config_versioned_t *config) {
                if (config->config.name == name) {
                    *server_id_out = server_id;
                    server_names_out->names.insert(std::make_pair(
                        server_id, std::make_pair(config->version, name)));
                    ++count;
                }
            });
        }
        if (count >= 2) {
            *error_out = admin_err_t{
                strprintf("Server `%s` is ambiguous; there are multiple "
                          "servers with that name.", name.c_str()),
                query_state_t::FAILED};
            return false;
        } else if (count == 0) {
            *error_out = admin_err_t{
                strprintf("Server `%s` does not exist or is not connected.",
                          name.c_str()),
                query_state_t::FAILED};
            return false;
        } else {
            return true;
        }
    } else {
        if (!convert_uuid_from_datum(datum, server_id_out, error_out)) {
            return false;
        }
        /* We know the server's UUID, but we need to confirm that it exists and determine
        its name. First we look up the server name in `get_server_config_map()`; if that
        fails, then we look it up in `old_server_names`. We prefer
        `get_server_config_map()` because the name information there is more likely to be
        up-to-date. */
        bool found;
        server_config_client->get_server_config_map()->read_key(*server_id_out,
            [&](const server_config_versioned_t *config) {
                if (config != nullptr) {
                    server_names_out->names.insert(std::make_pair(*server_id_out,
                        std::make_pair(config->version, config->config.name)));
                    found = true;
                } else {
                    found = false;
                }
            });
        if (found) {
            return true;
        }
        auto it = old_server_names.names.find(*server_id_out);
        if (it != old_server_names.names.end()) {
            server_names_out->names.insert(*it);
            return true;
        }
        *error_out = admin_err_t{
            strprintf("There is no server with UUID `%s`.",
                      uuid_to_str(*server_id_out).c_str()),
            query_state_t::FAILED};
        return false;
    }
}

ql::datum_t convert_replica_list_to_datum(
        const std::set<server_id_t> &replicas,
        admin_identifier_format_t identifier_format,
        const server_name_map_t &server_names) {
    ql::datum_array_builder_t replicas_builder(ql::configured_limits_t::unlimited);
    for (const server_id_t &replica : replicas) {
        replicas_builder.add(convert_name_or_uuid_to_datum(
            server_names.get(replica), replica, identifier_format));
    }
    return std::move(replicas_builder).to_datum();
}

bool convert_replica_list_from_datum(
        const ql::datum_t &datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        const server_name_map_t &old_server_names,
        std::set<server_id_t> *replicas_out,
        server_name_map_t *server_names_out,
        admin_err_t *error_out) {
    if (datum.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = admin_err_t{
            "Expected an array, got " + datum.print(),
            query_state_t::FAILED};
        return false;
    }
    replicas_out->clear();
    for (size_t i = 0; i < datum.arr_size(); ++i) {
        server_id_t server_id;
        if (!convert_server_id_from_datum(datum.get(i), identifier_format,
                server_config_client, old_server_names, &server_id, server_names_out,
                error_out)) {
            return false;
        }
        auto pair = replicas_out->insert(server_id);
        if (!pair.second) {
            *error_out = admin_err_t{
                strprintf("Server `%s` is listed more than once.",
                          server_names_out->get(server_id).c_str()),
                query_state_t::FAILED};
            return false;
        }
    }
    return true;
}

ql::datum_t convert_write_ack_config_to_datum(
        const write_ack_config_t &config) {
    switch (config) {
        case write_ack_config_t::SINGLE:
            return ql::datum_t("single");
        case write_ack_config_t::MAJORITY:
            return ql::datum_t("majority");
        default:
            unreachable();
    }
}

bool convert_write_ack_config_from_datum(
        const ql::datum_t &datum,
        write_ack_config_t *config_out,
        admin_err_t *error_out) {
    if (datum == ql::datum_t("single")) {
        *config_out = write_ack_config_t::SINGLE;
    } else if (datum == ql::datum_t("majority")) {
        *config_out = write_ack_config_t::MAJORITY;
    } else {
        *error_out = admin_err_t{
            "Expected \"single\" or \"majority\", got: " + datum.print(),
            query_state_t::FAILED};
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
        admin_err_t *error_out) {
    if (datum == ql::datum_t("soft")) {
        *durability_out = write_durability_t::SOFT;
    } else if (datum == ql::datum_t("hard")) {
        *durability_out = write_durability_t::HARD;
    } else {
        *error_out = admin_err_t{
            "Expected \"soft\" or \"hard\", got: " + datum.print(),
            query_state_t::FAILED};
        return false;
    }
    return true;
}

ql::datum_t convert_table_config_shard_to_datum(
        const table_config_t::shard_t &shard,
        admin_identifier_format_t identifier_format,
        const server_name_map_t &server_names) {
    ql::datum_object_builder_t builder;

    builder.overwrite("replicas",
        convert_replica_list_to_datum(
            shard.all_replicas, identifier_format, server_names));
    builder.overwrite("nonvoting_replicas",
        convert_replica_list_to_datum(
            shard.nonvoting_replicas, identifier_format, server_names));
    builder.overwrite("primary_replica",
        convert_name_or_uuid_to_datum(server_names.get(shard.primary_replica),
            shard.primary_replica, identifier_format));

    return std::move(builder).to_datum();
}

bool convert_table_config_shard_from_datum(
        ql::datum_t datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        const server_name_map_t &old_server_names,
        table_config_t::shard_t *shard_out,
        server_name_map_t *server_names_out,
        admin_err_t *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t all_replicas_datum;
    if (!converter.get("replicas", &all_replicas_datum, error_out)) {
        return false;
    }
    if (!convert_replica_list_from_datum(all_replicas_datum, identifier_format,
            server_config_client, old_server_names, &shard_out->all_replicas,
            server_names_out, error_out)) {
        error_out->msg = "In `replicas`: " + error_out->msg;
        return false;
    }
    if (shard_out->all_replicas.empty()) {
        *error_out = admin_err_t{
            "You must specify at least one replica for each shard.",
            query_state_t::FAILED};
        return false;
    }

    ql::datum_t nonvoting_replicas_datum;
    if (converter.get("nonvoting_replicas", &nonvoting_replicas_datum, error_out)) {
        if (!convert_replica_list_from_datum(nonvoting_replicas_datum, identifier_format,
                server_config_client, old_server_names, &shard_out->nonvoting_replicas,
                server_names_out, error_out)) {
            error_out->msg = "In `nonvoting_replicas`: " + error_out->msg;
            return false;
        }
        for (const server_id_t &server : shard_out->nonvoting_replicas) {
            if (shard_out->all_replicas.count(server) != 1) {
                *error_out = admin_err_t{
                    "Every server listed in the `nonvoting_replicas` field "
                    "must also appear in `replicas`.",
                    query_state_t::FAILED};
                return false;
            }
        }
    } else {
        shard_out->nonvoting_replicas.clear();
    }

    ql::datum_t primary_replica_datum;
    if (!converter.get("primary_replica", &primary_replica_datum, error_out)) {
        return false;
    }

    if (!convert_server_id_from_datum(primary_replica_datum, identifier_format,
            server_config_client, old_server_names, &shard_out->primary_replica,
            server_names_out, error_out)) {
        error_out->msg = "In `primary_replica`: " + error_out->msg;
        return false;
    }
    if (shard_out->all_replicas.count(shard_out->primary_replica) != 1) {
        *error_out = admin_err_t{
            strprintf("The server listed in the `primary_replica` field "
                      "(`%s`) must also appear in `replicas`.",
                      server_names_out->get(shard_out->primary_replica).c_str()),
            query_state_t::FAILED};
        return false;
    }
    if (shard_out->nonvoting_replicas.count(shard_out->primary_replica) == 1) {
        *error_out = admin_err_t{
            strprintf("The primary replica (`%s`) must not be a nonvoting replica.",
                      server_names_out->get(shard_out->primary_replica).c_str()),
            query_state_t::FAILED};
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}

ql::datum_t convert_sindexes_to_datum(
        const std::map<std::string, sindex_config_t> &sindexes) {
    ql::datum_array_builder_t sindexes_builder(ql::configured_limits_t::unlimited);
    for (const auto &sindex : sindexes) {
        sindexes_builder.add(convert_string_to_datum(sindex.first));
    }
    return std::move(sindexes_builder).to_datum();
}

bool convert_sindexes_from_datum(
        ql::datum_t datum,
        std::set<std::string> *indexes_out,
        admin_err_t *error_out) {
    if (!convert_set_from_datum<std::string>(
            &convert_string_from_datum, false, datum, indexes_out, error_out)) {
        error_out->msg = "In `indexes`: " + error_out->msg;
        return false;
    }

    return true;
}

/* This is separate from `format_row()` because it needs to be publicly exposed so it can
   be used to create the return value of `table.reconfigure()`. */
ql::datum_t convert_table_config_to_datum(
        namespace_id_t table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_t &config,
        admin_identifier_format_t identifier_format,
        const server_name_map_t &server_names) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(config.basic.name));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("indexes", convert_sindexes_to_datum(config.sindexes));
    builder.overwrite("primary_key", convert_string_to_datum(config.basic.primary_key));
    builder.overwrite("shards",
        convert_vector_to_datum<table_config_t::shard_t>(
            [&](const table_config_t::shard_t &shard) {
                return convert_table_config_shard_to_datum(
                    shard, identifier_format, server_names);
            },
            config.shards));
    builder.overwrite("write_acks",
        convert_write_ack_config_to_datum(config.write_ack_config));
    builder.overwrite("durability",
        convert_durability_to_datum(config.durability));
    return std::move(builder).to_datum();
}

void table_config_artificial_table_backend_t::format_row(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &config,
        const ql::datum_t &db_name_or_uuid,
        UNUSED signal_t *interruptor_on_home,
        ql::datum_t *row_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    assert_thread();
    *row_out = convert_table_config_to_datum(table_id, db_name_or_uuid,
        config.config, identifier_format, config.server_names);
}

bool convert_table_config_and_name_from_datum(
        ql::datum_t datum,
        bool existed_before,
        const cluster_semilattice_metadata_t &all_metadata,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        const table_config_and_shards_t &old_config,
        table_meta_client_t *table_meta_client,
        signal_t *interruptor,
        namespace_id_t *id_out,
        table_config_t *config_out,
        server_name_map_t *server_names_out,
        name_string_t *db_name_out,
        admin_err_t *error_out)
        THROWS_ONLY(interrupted_exc_t, admin_op_exc_t) {
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
    if (!convert_name_from_datum(
            name_datum, "table name", &config_out->basic.name, error_out)) {
        error_out->msg = "In `name`: " + error_out->msg;
        return false;
    }

    ql::datum_t db_datum;
    if (!converter.get("db", &db_datum, error_out)) {
        return false;
    }
    if (!convert_database_id_from_datum(
            db_datum, identifier_format, all_metadata, &config_out->basic.database,
            db_name_out, error_out)) {
        return false;
    }

    ql::datum_t id_datum;
    if (!converter.get("id", &id_datum, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(id_datum, id_out, error_out)) {
        error_out->msg = "In `id`: " + error_out->msg;
        return false;
    }

    /* As a special case, we allow the user to omit `indexes`, `primary_key`, `shards`,
    `write_acks`, and/or `durability` for newly-created tables. */

    if (converter.has("indexes")) {
        ql::datum_t indexes_datum;
        if (!converter.get("indexes", &indexes_datum, error_out)) {
            return false;
        }
        std::set<std::string> sindexes;
        if (!convert_sindexes_from_datum(indexes_datum, &sindexes, error_out)) {
            return false;
        }

        if (existed_before) {
            bool equal = sindexes.size() == old_config.config.sindexes.size();
            for (const auto &old_sindex : old_config.config.sindexes) {
                equal &= sindexes.count(old_sindex.first) == 1;
            }
            if (!equal) {
                error_out->msg = "The `indexes` field is read-only and can't be used to "
                                 "create or drop indexes.";
                return false;
            }
            config_out->sindexes = old_config.config.sindexes;
        } else if (!sindexes.empty()) {
            error_out->msg = "The `indexes` field is read-only and can't be used to "
                             "create indexes.";
            return false;
        }
    } else {
        if (existed_before) {
            error_out->msg = "Expected a field named `indexes`.";
            return false;
        }
    }

    if (existed_before || converter.has("primary_key")) {
        ql::datum_t primary_key_datum;
        if (!converter.get("primary_key", &primary_key_datum, error_out)) {
            return false;
        }
        if (!convert_string_from_datum(primary_key_datum,
                &config_out->basic.primary_key, error_out)) {
            error_out->msg = "In `primary_key`: " + error_out->msg;
            return false;
        }
    } else {
        config_out->basic.primary_key = "id";
    }

    if (existed_before || converter.has("shards")) {
        ql::datum_t shards_datum;
        if (!converter.get("shards", &shards_datum, error_out)) {
            return false;
        }
        if (!convert_vector_from_datum<table_config_t::shard_t>(
                [&](ql::datum_t shard_datum, table_config_t::shard_t *shard_out,
                        admin_err_t *error_out_2) {
                    return convert_table_config_shard_from_datum(
                        shard_datum, identifier_format, server_config_client,
                        old_config.server_names, shard_out, server_names_out,
                        error_out_2);
                },
                shards_datum,
                &config_out->shards,
                error_out)) {
            error_out->msg = "In `shards`: " + error_out->msg;
            return false;
        }
        if (config_out->shards.empty()) {
            *error_out = admin_err_t{
                "In `shards`: You must specify at least one shard.",
                query_state_t::FAILED};
            return false;
        }
    } else {
        try {
            table_generate_config(
                server_config_client, nil_uuid(), table_meta_client,
                table_generate_config_params_t::make_default(), table_shard_scheme_t(),
                interruptor, &config_out->shards, server_names_out);
        } catch (const admin_op_exc_t &msg) {
            throw admin_op_exc_t(
                "Unable to automatically generate configuration for "
                "new table: " + std::string(msg.what()),
                query_state_t::FAILED);
        } catch (const no_such_table_exc_t &) {
            /* This can't happen when calling `table_generate_config()` for a new table,
            only when updating an existing one */
            unreachable();
        } catch (const failed_table_op_exc_t &) {
            /* Same as with `no_such_table_exc_t` */
            unreachable();
        }
    }

    if (existed_before || converter.has("write_acks")) {
        ql::datum_t write_acks_datum;
        if (!converter.get("write_acks", &write_acks_datum, error_out)) {
            return false;
        }
        if (!convert_write_ack_config_from_datum(write_acks_datum,
                &config_out->write_ack_config, error_out)) {
            error_out->msg = "In `write_acks`: " + error_out->msg;
            return false;
        }
    } else {
        config_out->write_ack_config = write_ack_config_t::MAJORITY;
    }

    if (existed_before || converter.has("durability")) {
        ql::datum_t durability_datum;
        if (!converter.get("durability", &durability_datum, error_out)) {
            return false;
        }
        if (!convert_durability_from_datum(durability_datum, &config_out->durability,
                                           error_out)) {
            error_out->msg = "In `durability`: " + error_out->msg;
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

void table_config_artificial_table_backend_t::do_modify(
        const namespace_id_t &table_id,
        table_config_and_shards_t &&old_config,
        table_config_t &&new_config_no_shards,
        server_name_map_t &&new_server_names,
        const name_string_t &old_db_name,
        const name_string_t &new_db_name,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t, admin_op_exc_t) {
    table_config_and_shards_t new_config;
    new_config.config = std::move(new_config_no_shards);
    new_config.server_names = std::move(new_server_names);

    if (new_config.config.basic.primary_key != old_config.config.basic.primary_key) {
        throw admin_op_exc_t("It's illegal to change a table's primary key",
                             query_state_t::FAILED);
    }

    if (new_config.config.basic.database != old_config.config.basic.database ||
            new_config.config.basic.name != old_config.config.basic.name) {
        if (table_meta_client->exists(
                new_config.config.basic.database, new_config.config.basic.name)) {
            throw admin_op_exc_t(
                strprintf(
                    "Can't rename table `%s.%s` to `%s.%s` because table `%s.%s` "
                    "already exists.",
                    old_db_name.c_str(), old_config.config.basic.name.c_str(),
                    new_db_name.c_str(), new_config.config.basic.name.c_str(),
                    new_db_name.c_str(), new_config.config.basic.name.c_str()),
                query_state_t::FAILED);
        }
    }

    calculate_split_points_intelligently(table_id, reql_cluster_interface,
        new_config.config.shards.size(), old_config.shard_scheme, interruptor,
        &new_config.shard_scheme);

    table_meta_client->set_config(table_id, new_config, interruptor);
}

void table_config_artificial_table_backend_t::do_create(
        const namespace_id_t &table_id,
        table_config_t &&new_config_no_shards,
        server_name_map_t &&new_server_names,
        const name_string_t &new_db_name,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t, admin_op_exc_t) {
    table_config_and_shards_t new_config;
    new_config.config = std::move(new_config_no_shards);
    new_config.server_names = std::move(new_server_names);

    if (table_meta_client->exists(
            new_config.config.basic.database, new_config.config.basic.name)) {
        throw admin_op_exc_t(
            strprintf("Table `%s.%s` already exists.",
                      new_db_name.c_str(), new_config.config.basic.name.c_str()),
            query_state_t::FAILED);
    }

    calculate_split_points_for_uuids(
        new_config.config.shards.size(), &new_config.shard_scheme);

    table_meta_client->create(table_id, new_config, interruptor);
}

bool table_config_artificial_table_backend_t::write_row(
        ql::datum_t primary_key,
        bool pkey_was_autogenerated,
        ql::datum_t *new_value_inout,
        signal_t *interruptor_on_caller,
        admin_err_t *error_out) {
    /* Parse primary key */
    namespace_id_t table_id;
    admin_err_t dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        guarantee(!pkey_was_autogenerated, "auto-generated primary key should have "
            "been a valid UUID string.");
        table_id = nil_uuid();
    }

    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    cluster_semilattice_metadata_t metadata = semilattice_view->get();

    try {
        try {
            /* Fetch the name of the table and its database for error messages */
            table_basic_config_t old_basic_config;
            table_meta_client->get_name(table_id, &old_basic_config);
            guarantee(!pkey_was_autogenerated, "UUID collision happened");
            name_string_t old_db_name;
            if (!convert_database_id_to_datum(old_basic_config.database,
                    identifier_format, metadata, nullptr, &old_db_name)) {
                old_db_name = name_string_t::guarantee_valid("__deleted_database__");
            }

            if (new_value_inout->has()) {
                table_config_and_shards_t old_config;
                table_meta_client->get_config(
                    table_id, &interruptor_on_home, &old_config);

                table_config_t new_config;
                server_name_map_t new_server_names;
                namespace_id_t new_table_id;
                name_string_t new_db_name;
                if (!convert_table_config_and_name_from_datum(*new_value_inout, true,
                        metadata, identifier_format, server_config_client,
                        old_config, table_meta_client, &interruptor_on_home,
                        &new_table_id, &new_config, &new_server_names, &new_db_name,
                        error_out)) {
                    error_out->msg = "The change you're trying to make to "
                        "`rethinkdb.table_config` has the wrong format. "
                        + error_out->msg;
                    return false;
                }
                guarantee(new_table_id == table_id, "artificial_table_t shouldn't have "
                    "allowed the primary key to change");
                try {
                    do_modify(table_id, std::move(old_config), std::move(new_config),
                        std::move(new_server_names), old_db_name, new_db_name,
                        &interruptor_on_home);
                    return true;
                } CATCH_OP_ERRORS(old_db_name, old_basic_config.name, error_out,
                    "The table's configuration was not changed.",
                    "The table's configuration may or may not have been changed.")
            } else {
                try {
                    table_meta_client->drop(table_id, &interruptor_on_home);
                    return true;
                } CATCH_OP_ERRORS(old_db_name, old_basic_config.name, error_out,
                    "The table was not dropped.",
                    "The table may or may not have been dropped.")
            }
        } catch (const no_such_table_exc_t &) {
            /* Fall through */
        }
        if (new_value_inout->has()) {
            if (!pkey_was_autogenerated) {
                *error_out = admin_err_t{
                    "There is no existing table with the given ID. To create a "
                    "new table by inserting into `rethinkdb.table_config`, you must let "
                    "the database generate the primary key automatically.",
                    query_state_t::FAILED};
                return false;
            }

            namespace_id_t new_table_id;
            table_config_t new_config;
            server_name_map_t new_server_names;
            name_string_t new_db_name;
            if (!convert_table_config_and_name_from_datum(*new_value_inout, false,
                    metadata, identifier_format, server_config_client,
                    table_config_and_shards_t(), table_meta_client,
                    &interruptor_on_home, &new_table_id, &new_config, &new_server_names,
                    &new_db_name, error_out)) {
                error_out->msg = "The change you're trying to make to "
                    "`rethinkdb.table_config` has the wrong format. " + error_out->msg;
                return false;
            }
            guarantee(new_table_id == table_id, "artificial_table_t shouldn't have "
                "allowed the primary key to change");

            /* `convert_table_config_and_name_from_datum()` might have filled in missing
            fields, so we need to write back the filled-in values to `new_value_inout`.
            */
            *new_value_inout = convert_table_config_to_datum(
                table_id, new_value_inout->get_field("db"), new_config,
                identifier_format, new_server_names);

            try {
                do_create(table_id, std::move(new_config), std::move(new_server_names),
                    new_db_name, &interruptor_on_home);
                return true;
            } CATCH_OP_ERRORS(new_db_name, new_config.basic.name, error_out,
                "The table was not created.",
                "The table may or may not have been created.")
        } else {
            /* The user is deleting a table that doesn't exist. Do nothing. */
            return true;
        }
    } catch (const admin_op_exc_t &msg) {
        *error_out = admin_err_t{msg.what(), msg.query_state};
        return false;
    }
}
