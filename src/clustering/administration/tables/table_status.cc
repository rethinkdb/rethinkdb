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

ql::datum_t convert_table_status_to_datum(
        const namespace_id_t &table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_and_shards_t &config_and_shards,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_readiness_t readiness,
        const std::vector<shard_status_t> &shard_statuses) {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(config_and_shards.config.name));

    ql::datum_array_builder_t shards_builder(ql::configured_limits_t::unlimited);
    for (const auto &shard_status : shard_statuses) {
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
        if (!replicas_builder.empty()) {
            shard_builder.overwrite("replicas", std::move(replicas_builder).to_datum());
            shards_builder.add(std::move(shard_builder).to_datum());
        }
    }
    builder.overwrite("shards", std::move(shards_builder).to_datum());

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

bool table_status_artificial_table_backend_t::format_row(
        namespace_id_t table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_and_shards_t &config_and_shards,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    assert_thread();

    table_readiness_t readiness;
    std::vector<shard_status_t> shard_statuses;
    if (!calculate_status(
            table_id,
            config_and_shards,
            interruptor,
            table_meta_client,
            server_config_client,
            &readiness,
            &shard_statuses,
            error_out)) {
        return false;
    }

    *row_out = convert_table_status_to_datum(
        table_id,
        db_name_or_uuid,
        config_and_shards,
        identifier_format,
        server_config_client,
        readiness,
        shard_statuses);

    return true;
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
        ql::datum_t *status_out) {
    backend->assert_thread();

    bool immediate = true;
    /* Start with `initial_poll_ms`, then double the waiting period after each attempt up
       to a maximum of `max_poll_ms`. */
    uint32_t current_poll_ms = initial_poll_ms;
    while (true) {
        table_config_and_shards_t config_and_shards;
        if (backend->table_meta_client->get_config(
                table_id, interruptor, &config_and_shards)) {
            ql::datum_t db_name_or_uuid;
            name_string_t db_name;
            if (!convert_table_id_to_datums(
                    table_id,
                    backend->identifier_format,
                    backend->semilattice_view->get(),
                    backend->table_meta_client,
                    nullptr,
                    nullptr,
                    &db_name_or_uuid,
                    &db_name) || db_name.str() == "__deleted_database__") {
                // Either the database or the table was deleted.
                return table_wait_result_t::DELETED;
            }

            table_readiness_t readiness;
            std::vector<shard_status_t> shard_statuses;
            bool status_result = calculate_status(
                table_id,
                config_and_shards,
                interruptor,
                backend->table_meta_client,
                backend->server_config_client,
                &readiness,
                &shard_statuses,
                nullptr);

            if (status_result && readiness >= wait_readiness) {
                if (status_out != nullptr) {
                    *status_out = convert_table_status_to_datum(
                        table_id,
                        db_name_or_uuid,
                        config_and_shards,
                        backend->identifier_format,
                        backend->server_config_client,
                        readiness,
                        shard_statuses);
                }
                return immediate
                    ? table_wait_result_t::IMMEDIATE
                    : table_wait_result_t::WAITED;
            }
        }

        immediate = false;
        nap(current_poll_ms, interruptor);
        current_poll_ms = std::min(max_poll_ms, current_poll_ms * 2u);
    }
}
