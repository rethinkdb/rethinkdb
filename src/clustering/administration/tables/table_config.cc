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

ql::datum_t convert_replica_list_to_datum(
        const std::set<server_id_t> &replicas,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    ql::datum_array_builder_t replicas_builder(ql::configured_limits_t::unlimited);
    for (const server_id_t &replica : replicas) {
        ql::datum_t replica_datum;
        /* This will return `false` for replicas that have been permanently removed */
        if (convert_server_id_to_datum(
                replica, identifier_format, server_config_client, &replica_datum,
                nullptr)) {
            replicas_builder.add(replica_datum);
        }
    }
    return std::move(replicas_builder).to_datum();
}

bool convert_replica_list_from_datum(
        const ql::datum_t &datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        std::set<server_id_t> *replicas_out,
        std::string *error_out) {
    if (datum.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "Expected an array, got " + datum.print();
        return false;
    }
    replicas_out->clear();
    for (size_t i = 0; i < datum.arr_size(); ++i) {
        server_id_t server_id;
        name_string_t server_name;
        if (!convert_server_id_from_datum(datum.get(i), identifier_format,
                server_config_client, &server_id, &server_name, error_out)) {
            return false;
        }
        auto pair = replicas_out->insert(server_id);
        if (!pair.second) {
            *error_out = strprintf("Server `%s` is listed more than once.",
                server_name.c_str());
            return false;
        }
    }
    return true;
}

ql::datum_t convert_write_ack_config_req_to_datum(
        const write_ack_config_t::req_t &req,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("replicas",
        convert_replica_list_to_datum(req.replicas, identifier_format,
                                      server_config_client));
    const char *acks =
        (req.mode == write_ack_config_t::mode_t::majority) ? "majority" : "single";
    builder.overwrite("acks", ql::datum_t(acks));
    return std::move(builder).to_datum();
}

bool convert_write_ack_config_req_from_datum(
        const ql::datum_t &datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        write_ack_config_t::req_t *req_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t replicas_datum;
    if (!converter.get("replicas", &replicas_datum, error_out)) {
        return false;
    }
    if (!convert_replica_list_from_datum(replicas_datum, identifier_format,
            server_config_client, &req_out->replicas, error_out)) {
        *error_out = "In `replicas`: " + *error_out;
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
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    if (config.mode == write_ack_config_t::mode_t::single) {
        return ql::datum_t("single");
    } else if (config.mode == write_ack_config_t::mode_t::majority) {
        return ql::datum_t("majority");
    } else {
        return convert_vector_to_datum<write_ack_config_t::req_t>(
            [&](const write_ack_config_t::req_t &req) {
                return convert_write_ack_config_req_to_datum(
                    req, identifier_format, server_config_client);
            }, config.complex_reqs);
    }
}

bool convert_write_ack_config_from_datum(
        const ql::datum_t &datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
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
                        datum_2, identifier_format, server_config_client,
                        req_out, error_out_2);
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
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    ql::datum_object_builder_t builder;

    builder.overwrite("replicas",
        convert_replica_list_to_datum(shard.replicas, identifier_format,
                                      server_config_client));

    ql::datum_t primary_replica;
    if (!convert_server_id_to_datum(shard.primary_replica, identifier_format,
                                    server_config_client, &primary_replica, nullptr)) {
        /* If the previous primary replica was declared dead, just display `null`. The
        user will have to change this to a new server before the table will come back 
        online. */
        primary_replica = ql::datum_t::null();
    }
    builder.overwrite("primary_replica", primary_replica);

    return std::move(builder).to_datum();
}

bool convert_table_config_shard_from_datum(
        ql::datum_t datum,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_config_t::shard_t *shard_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    ql::datum_t replicas_datum;
    if (!converter.get("replicas", &replicas_datum, error_out)) {
        return false;
    }
    if (!convert_replica_list_from_datum(replicas_datum, identifier_format,
            server_config_client, &shard_out->replicas, error_out)) {
        *error_out = "In `replicas`: " + *error_out;
        return false;
    }
    if (shard_out->replicas.empty()) {
        *error_out = "You must specify at least one replica for each shard.";
        return false;
    }

    ql::datum_t primary_replica_datum;
    if (!converter.get("primary_replica", &primary_replica_datum, error_out)) {
        return false;
    }
    if (primary_replica_datum.get_type() == ql::datum_t::R_NULL) {
        /* There's never a good reason for the user to intentionally set the primary
        replica to `null`; setting the primary replica to `null` will ensure that the
        table cannot accept queries. We allow it because if the primary replica is 
        declared dead, it will appear to the user as `null`; and we want to allow the
        user to do things like `r.table_config("foo").update({"name": "bar"})` even when
        the primary replica is in that state. */
        shard_out->primary_replica = nil_uuid();
    } else {
        name_string_t primary_replica_name;
        if (!convert_server_id_from_datum(primary_replica_datum, identifier_format,
                server_config_client, &shard_out->primary_replica, &primary_replica_name, error_out)) {
            *error_out = "In `primary_replica`: " + *error_out;
            return false;
        }
        if (shard_out->replicas.count(shard_out->primary_replica) != 1) {
            *error_out = strprintf("The server listed in the `primary_replica` field "
                                   "(`%s`) must also appear in `replicas`.",
                                   primary_replica_name.c_str());
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
        namespace_id_t table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_t &config,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(config.basic.name));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("primary_key", convert_string_to_datum(config.basic.primary_key));
    builder.overwrite("shards",
        convert_vector_to_datum<table_config_t::shard_t>(
            [&](const table_config_t::shard_t &shard) {
                return convert_table_config_shard_to_datum(
                    shard, identifier_format, server_config_client);
            },
            config.shards));
    builder.overwrite("write_acks",
        convert_write_ack_config_to_datum(
            config.write_ack_config, identifier_format, server_config_client));
    builder.overwrite("durability",
        convert_durability_to_datum(config.durability));
    return std::move(builder).to_datum();
}

void table_config_artificial_table_backend_t::format_row(
        const namespace_id_t &table_id,
        const table_basic_config_t &,
        const ql::datum_t &db_name_or_uuid,
        signal_t *interruptor,
        ql::datum_t *row_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            admin_op_exc_t) {
    assert_thread();
    table_config_and_shards_t config_and_shards;
    table_meta_client->get_config(table_id, interruptor, &config_and_shards);
    *row_out = convert_table_config_to_datum(table_id, db_name_or_uuid,
        config_and_shards.config, identifier_format, server_config_client);
}

bool convert_table_config_and_name_from_datum(
        ql::datum_t datum,
        bool existed_before,
        const cluster_semilattice_metadata_t &all_metadata,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        signal_t *interruptor,
        namespace_id_t *id_out,
        table_config_t *config_out,
        name_string_t *db_name_out,
        std::string *error_out)
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
        *error_out = "In `name`: " + *error_out;
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
        *error_out = "In `id`: " + *error_out;
        return false;
    }

    /* As a special case, we allow the user to omit `primary_key`, `shards`,
    `write_acks`, and/or `durability` for newly-created tables. */

    if (existed_before || converter.has("primary_key")) {
        ql::datum_t primary_key_datum;
        if (!converter.get("primary_key", &primary_key_datum, error_out)) {
            return false;
        }
        if (!convert_string_from_datum(primary_key_datum,
                &config_out->basic.primary_key, error_out)) {
            *error_out = "In `primary_key`: " + *error_out;
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
                        std::string *error_out_2) {
                    return convert_table_config_shard_from_datum(
                        shard_datum, identifier_format, server_config_client,
                        shard_out, error_out_2);
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
        try {
            table_generate_config(
                server_config_client, nil_uuid(), table_meta_client,
                table_generate_config_params_t::make_default(), table_shard_scheme_t(),
                interruptor, &config_out->shards);
        } catch (const admin_op_exc_t &msg) {
            throw admin_op_exc_t("Unable to automatically generate configuration for "
                "new table: " + std::string(msg.what()));
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
        if (!convert_write_ack_config_from_datum(write_acks_datum, identifier_format,
                server_config_client, &config_out->write_ack_config, error_out)) {
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

    write_ack_config_checker_t ack_checker(*config_out, all_metadata.servers);
    for (const table_config_t::shard_t &shard : config_out->shards) {
        std::set<server_id_t> replicas;
        replicas.insert(shard.replicas.begin(), shard.replicas.end());
        if (!ack_checker.check_acks(replicas)) {
            *error_out = "The `write_acks` settings you provided make some shard(s) "
                "unwritable. This usually happens because different shards have "
                "different numbers of replicas; the 'majority' write ack setting "
                "applies the same threshold to every shard, but it computes the "
                "threshold based on the shard with the most replicas.";
            return false;
        }
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}

void table_config_artificial_table_backend_t::do_modify(
        const namespace_id_t &table_id,
        table_config_t &&new_config_no_shards,
        const name_string_t &old_db_name,
        const name_string_t &new_db_name,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t, admin_op_exc_t) {
    table_config_and_shards_t new_config;
    new_config.config = std::move(new_config_no_shards);

    /* Fetch the previous table configuration */
    table_config_and_shards_t old_config;
    table_meta_client->get_config(table_id, interruptor, &old_config);

    if (new_config.config.basic.primary_key != old_config.config.basic.primary_key) {
        throw admin_op_exc_t("It's illegal to change a table's primary key");
    }

    if (new_config.config.basic.database != old_config.config.basic.database ||
            new_config.config.basic.name != old_config.config.basic.name) {
        if (table_meta_client->exists(
                new_config.config.basic.database, new_config.config.basic.name)) {
            throw admin_op_exc_t(strprintf(
                "Can't rename table `%s.%s` to `%s.%s` because table `%s.%s` already "
                "exists.",
                old_db_name.c_str(), old_config.config.basic.name.c_str(),
                new_db_name.c_str(), new_config.config.basic.name.c_str(),
                new_db_name.c_str(), new_config.config.basic.name.c_str()));
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
        const name_string_t &new_db_name,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t, admin_op_exc_t) {
    table_config_and_shards_t new_config;
    new_config.config = std::move(new_config_no_shards);

    if (table_meta_client->exists(
            new_config.config.basic.database, new_config.config.basic.name)) {
        throw admin_op_exc_t(strprintf("Table `%s.%s` already exists.",
            new_db_name.c_str(), new_config.config.basic.name.c_str()));
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
        std::string *error_out) {
    /* Parse primary key */
    namespace_id_t table_id;
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, &table_id, &dummy_error)) {
        /* If the primary key was not a valid UUID, then it must refer to a nonexistent
        row. */
        guarantee(!pkey_was_autogenerated, "auto-generated primary key should have "
            "been a valid UUID string.");
        table_id = nil_uuid();
    }

    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
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
                table_config_t new_config;
                namespace_id_t new_table_id;
                name_string_t new_db_name;
                if (!convert_table_config_and_name_from_datum(*new_value_inout, true,
                        metadata, identifier_format, server_config_client,
                        table_meta_client, &interruptor, &new_table_id, &new_config,
                        &new_db_name, error_out)) {
                    *error_out = "The change you're trying to make to "
                        "`rethinkdb.table_config` has the wrong format. " + *error_out;
                    return false;
                }
                guarantee(new_table_id == table_id, "artificial_table_t shouldn't have "
                    "allowed the primary key to change");
                try {
                    do_modify(table_id, std::move(new_config), old_db_name, new_db_name,
                        &interruptor);
                    return true;
                } CATCH_OP_ERRORS(old_db_name, old_basic_config.name, error_out,
                    "The table's configuration was not changed.",
                    "The table's configuration may or may not have been changed.")
            } else {
                try {
                    table_meta_client->drop(table_id, &interruptor);
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
                *error_out = "There is no existing table with the given ID. To create a "
                    "new table by inserting into `rethinkdb.table_config`, you must let "
                    "the database generate the primary key automatically.";
                return false;
            }

            namespace_id_t new_table_id;
            table_config_t new_config;
            name_string_t new_db_name;
            if (!convert_table_config_and_name_from_datum(*new_value_inout, false,
                    metadata, identifier_format, server_config_client, table_meta_client,
                    &interruptor, &new_table_id, &new_config, &new_db_name, error_out)) {
                *error_out = "The change you're trying to make to "
                    "`rethinkdb.table_config` has the wrong format. " + *error_out;
                return false;
            }
            guarantee(new_table_id == table_id, "artificial_table_t shouldn't have "
                "allowed the primary key to change");

            /* `convert_table_config_and_name_from_datum()` might have filled in missing
            fields, so we need to write back the filled-in values to `new_value_inout`.
            */
            *new_value_inout = convert_table_config_to_datum(
                table_id, new_value_inout->get_field("db"), new_config,
                identifier_format, server_config_client);

            try {
                do_create(table_id, std::move(new_config), new_db_name, &interruptor);
                return true;
            } CATCH_OP_ERRORS(new_db_name, new_config.basic.name, error_out,
                "The table was not created.",
                "The table may or may not have been created.")
        } else {
            /* The user is deleting a table that doesn't exist. Do nothing. */
            return true;
        }
    } catch (const admin_op_exc_t &msg) {
        *error_out = msg.what();
        return false;
    }
}
