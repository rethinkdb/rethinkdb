// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_status.hpp"

#include <algorithm>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/administration/tables/calculate_status.hpp"
#include "clustering/table_contract/exec_primary.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

table_status_artificial_table_backend_t::table_status_artificial_table_backend_t(
            boost::shared_ptr<semilattice_readwrite_view_t<
                cluster_semilattice_metadata_t> > _semilattice_view,
            table_meta_client_t *_table_meta_client,
            admin_identifier_format_t _identifier_format,
            server_config_client_t *_server_config_client) :
        common_table_artificial_table_backend_t(
            _semilattice_view, _table_meta_client, _identifier_format),
        server_config_client(_server_config_client) {
}

table_status_artificial_table_backend_t::~table_status_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

ql::datum_t convert_shard_status_to_datum(
        const shard_status_t &shard_status,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    ql::datum_object_builder_t shard_builder;

    ql::datum_array_builder_t primary_replicas_builder(
        ql::configured_limits_t::unlimited);
    for (const auto &primary_replica : shard_status.primary_replicas) {
        ql::datum_t primary_replica_name_or_uuid;
        if (convert_server_id_to_datum(
                primary_replica,
                identifier_format,
                server_config_client,
                &primary_replica_name_or_uuid,
                nullptr)) {
            primary_replicas_builder.add(std::move(primary_replica_name_or_uuid));
        }
    }
    shard_builder.overwrite(
        "primary_replicas", std::move(primary_replicas_builder).to_datum());

    ql::datum_array_builder_t replicas_builder(ql::configured_limits_t::unlimited);
    for (const auto &replica : shard_status.replicas) {
        if (replica.second.empty()) {
            /* This server was in the `nothing` state and thus shouldn't appear in the
               replica map. */
            continue;
        }
        ql::datum_t replica_name_or_uuid;
        if (convert_server_id_to_datum(
                replica.first,
                identifier_format,
                server_config_client,
                &replica_name_or_uuid,
                nullptr)) {
            ql::datum_object_builder_t replica_builder;
            replica_builder.overwrite("server", std::move(replica_name_or_uuid));
            ql::datum_array_builder_t replica_states_builder(
                ql::configured_limits_t::unlimited);
            for (const auto &state : replica.second) {
                switch (state) {
                    case server_status_t::BACKFILLING:
                        replica_states_builder.add(ql::datum_t("backfilling"));
                        break;
                    case server_status_t::DISCONNECTED:
                        replica_states_builder.add(ql::datum_t("disconnected"));
                        break;
                    case server_status_t::NOTHING:
                        // Ignored
                        break;
                    case server_status_t::READY:
                        replica_states_builder.add(ql::datum_t("ready"));
                        break;
                    case server_status_t::TRANSITIONING:
                        replica_states_builder.add(ql::datum_t("transitioning"));
                        break;
                    case server_status_t::WAITING_FOR_PRIMARY:
                        replica_states_builder.add(
                            ql::datum_t("waiting_for_primary_replica"));
                        break;
                    case server_status_t::WAITING_FOR_QUORUM:
                        replica_states_builder.add(
                            ql::datum_t("waiting_for_quorum"));
                        break;
                }
            }
            if (!replica_states_builder.empty()) {
                replica_builder.overwrite(
                    "states", std::move(replica_states_builder).to_datum());
                replicas_builder.add(std::move(replica_builder).to_datum());
            }
        }
    }
    shard_builder.overwrite("replicas", std::move(replicas_builder).to_datum());
    return std::move(shard_builder).to_datum();
}

ql::datum_t convert_table_status_to_datum(
        const namespace_id_t &table_id,
        const name_string_t &table_name,
        const ql::datum_t &db_name_or_uuid,
        table_readiness_t readiness,
        const std::vector<shard_status_t> &shard_statuses,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(table_name));

    if (!shard_statuses.empty()) {
        ql::datum_array_builder_t shards_builder(ql::configured_limits_t::unlimited);
        for (const auto &shard_status : shard_statuses) {
            shards_builder.add(convert_shard_status_to_datum(
                shard_status, identifier_format, server_config_client));
        }
        builder.overwrite("shards", std::move(shards_builder).to_datum());
    } else {
        guarantee(readiness == table_readiness_t::unavailable);
        builder.overwrite("shards", ql::datum_t::null());
    }

    ql::datum_object_builder_t status_builder;
    status_builder.overwrite("ready_for_outdated_reads", ql::datum_t::boolean(
        readiness >= table_readiness_t::outdated_reads));
    status_builder.overwrite("ready_for_reads", ql::datum_t::boolean(
        readiness >= table_readiness_t::reads));
    status_builder.overwrite("ready_for_writes", ql::datum_t::boolean(
        readiness >= table_readiness_t::writes));
    status_builder.overwrite("all_replicas_ready", ql::datum_t::boolean(
        readiness == table_readiness_t::finished));
    builder.overwrite("status", std::move(status_builder).to_datum());

    return std::move(builder).to_datum();
}

void table_status_artificial_table_backend_t::format_row(
        const namespace_id_t &table_id,
        const table_basic_config_t &basic_config,
        const ql::datum_t &db_name_or_uuid,
        signal_t *interruptor,
        ql::datum_t *row_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            admin_op_exc_t) {
    assert_thread();

    table_readiness_t readiness;
    std::vector<shard_status_t> shard_statuses;
    calculate_status(
        table_id,
        interruptor,
        table_meta_client,
        server_config_client,
        &readiness,
        &shard_statuses);

    *row_out = convert_table_status_to_datum(
        table_id,
        basic_config.name,
        db_name_or_uuid,
        readiness,
        shard_statuses,
        identifier_format,
        server_config_client);
}

bool table_status_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb.table_status` table.";
    return false;
}

// These numbers are equal to those in `index_wait`.
uint32_t initial_poll_ms = 50;
uint32_t max_poll_ms = 10000;

table_wait_result_t wait_for_table_readiness(
        const namespace_id_t &table_id,
        table_readiness_t wait_readiness,
        const table_status_artificial_table_backend_t *backend,
        signal_t *interruptor,
        ql::datum_t *status_out)
        THROWS_ONLY(interrupted_exc_t) {
    backend->assert_thread();

    bool immediate = true;
    /* Start with `initial_poll_ms`, then double the waiting period after each attempt up
       to a maximum of `max_poll_ms`. */
    uint32_t current_poll_ms = initial_poll_ms;
    while (true) {
        try {
            name_string_t table_name;
            ql::datum_t db_name_or_uuid;
            name_string_t db_name;
            if (!convert_table_id_to_datums(
                    table_id,
                    backend->identifier_format,
                    backend->semilattice_view->get(),
                    backend->table_meta_client,
                    nullptr,
                    &table_name,
                    &db_name_or_uuid,
                    &db_name) || db_name.str() == "__deleted_database__") {
                // Either the database or the table was deleted.
                throw no_such_table_exc_t();
            }

            table_readiness_t readiness;
            std::vector<shard_status_t> shard_statuses;
            calculate_status(
                table_id,
                interruptor,
                backend->table_meta_client,
                backend->server_config_client,
                &readiness,
                &shard_statuses);

            if (readiness >= wait_readiness) {
                if (status_out != nullptr) {
                    *status_out = convert_table_status_to_datum(
                        table_id,
                        table_name,
                        db_name_or_uuid,
                        readiness,
                        shard_statuses,
                        backend->identifier_format,
                        backend->server_config_client);
                }
                return immediate
                    ? table_wait_result_t::IMMEDIATE
                    : table_wait_result_t::WAITED;
            }
        } catch (const no_such_table_exc_t &) {
            return table_wait_result_t::DELETED;
        }

        immediate = false;
        nap(current_poll_ms, interruptor);
        current_poll_ms = std::min(max_poll_ms, current_poll_ms * 2u);
    }
}
