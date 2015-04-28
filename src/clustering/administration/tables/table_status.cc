// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_status.hpp"

#include <algorithm>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
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

struct server_acks_t {
    contract_id_t latest_contract_id;
    std::map<server_id_t, std::pair<contract_id_t, contract_ack_t> > acks;
};

bool get_contracts_and_acks(
        namespace_id_t const &table_id,
        signal_t *interruptor,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        std::map<server_id_t, contracts_and_contract_acks_t> *contracts_and_acks_out,
        std::map<
                contract_id_t,
                std::reference_wrapper<const std::pair<region_t, contract_t> >
            > *contracts_out,
        server_id_t *latest_contracts_server_id_out) {
    std::map<peer_id_t, contracts_and_contract_acks_t> contracts_and_acks;
    if (!table_meta_client->get_status(
            table_id, interruptor, nullptr, &contracts_and_acks)) {
        return false;
    }

    multi_table_manager_bcard_t::timestamp_t latest_timestamp;
    latest_timestamp.epoch.timestamp = 0;
    latest_timestamp.epoch.id = nil_uuid();
    latest_timestamp.log_index = 0;
    for (const auto &peer : contracts_and_acks) {
        boost::optional<server_id_t> server_id =
            server_config_client->get_server_id_for_peer_id(peer.first);
        if (static_cast<bool>(server_id)) {
            auto pair = contracts_and_acks_out->insert(
                std::make_pair(server_id.get(), std::move(peer.second)));
            contracts_out->insert(
                pair.first->second.contracts.begin(),
                pair.first->second.contracts.end());
            if (pair.first->second.timestamp.supersedes(latest_timestamp)) {
                *latest_contracts_server_id_out = pair.first->first;
                latest_timestamp = pair.first->second.timestamp;
            }
        }
    }

    return !contracts_and_acks_out->empty();
}

ql::datum_t convert_table_status_shard_to_datum(
        const table_config_t::shard_t &shard,
        const region_map_t<server_acks_t> &regions,
        const std::map<server_id_t, contracts_and_contract_acks_t> &contracts_and_acks,
        const std::map<
                contract_id_t,
                std::reference_wrapper<const std::pair<region_t, contract_t> >
            > &contracts,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        table_readiness_t *readiness_out) {
    ql::datum_object_builder_t builder;

    bool has_quorum = true;
    bool has_primary_replica = true;
    bool has_outdated_reader = true;
    bool has_unfinished = false;

    std::set<server_id_t> primary_replicas;
    std::map<server_id_t, std::set<std::string> > replicas;
    for (const auto &region : regions) {
        const contract_t &latest_contract =
            contracts.at(region.second.latest_contract_id).get().second;

        ack_counter_t ack_counter(latest_contract);
        bool region_has_primary_replica = false;
        bool region_has_outdated_reader = false;

        for (const auto &ack : region.second.acks) {
            switch (ack.second.second.state) {
                case contract_ack_t::state_t::primary_need_branch:
                    has_unfinished = true;
                    replicas[ack.first].insert("waiting_for_quorum");
                    break;
                case contract_ack_t::state_t::secondary_need_primary:
                    region_has_outdated_reader = true;
                    has_unfinished = true;
                    replicas[ack.first].insert("waiting_for_primary_replica");
                    break;
                case contract_ack_t::state_t::primary_in_progress:
                case contract_ack_t::state_t::primary_ready:
                    region_has_primary_replica = true;
                    primary_replicas.insert(ack.first);
                    replicas[ack.first].insert("ready");
                    break;
                case contract_ack_t::state_t::secondary_backfilling:
                    has_unfinished = true;
                    replicas[ack.first].insert("backfilling");
                    break;
                case contract_ack_t::state_t::secondary_streaming:
                    {
                        const boost::optional<contract_t::primary_t> &region_primary =
                            contracts.at(ack.second.first).get().second.primary;
                        if (static_cast<bool>(latest_contract.primary) &&
                                latest_contract.primary == region_primary) {
                            region_has_outdated_reader = true;
                            replicas[ack.first].insert("ready");
                        } else {
                            has_unfinished = true;
                            replicas[ack.first].insert("transitioning");
                        }
                    }
                    break;
                case contract_ack_t::state_t::nothing:
                    if (replicas.find(ack.first) == replicas.end()) {
                        replicas[ack.first] = {};
                    }
                    break;
            }

            ack_counter.note_ack(ack.first);
        }

        for (const auto &replica : latest_contract.replicas) {
            if (replicas.find(replica) == replicas.end()) {
                has_unfinished = true;
                replicas[replica].insert(
                    contracts_and_acks.find(replica) == contracts_and_acks.end()
                        ? "disconnected" : "transitioning");
            }
        }
        for (const auto &replica : shard.replicas) {
            if (replicas.find(replica) == replicas.end()) {
                has_unfinished = true;
                replicas[replica].insert(
                    contracts_and_acks.find(replica) == contracts_and_acks.end()
                        ? "disconnected" : "transitioning");
            }
        }

        has_quorum &= ack_counter.is_safe();
        has_primary_replica &= region_has_primary_replica;
        has_outdated_reader &= region_has_outdated_reader;
    }

    if (readiness_out != nullptr) {
        if (has_primary_replica) {
            if (has_quorum) {
                if (!has_unfinished) {
                    *readiness_out = table_readiness_t::finished;
                } else {
                    *readiness_out = table_readiness_t::writes;
                }
            } else {
                *readiness_out = table_readiness_t::reads;
            }
        } else {
            if (has_outdated_reader) {
                *readiness_out = table_readiness_t::outdated_reads;
            } else {
                *readiness_out = table_readiness_t::unavailable;
            }
        }
    }

    ql::datum_array_builder_t primary_replicas_builder(
        ql::configured_limits_t::unlimited);
    for (const auto &primary_replica : primary_replicas) {
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
    builder.overwrite(
        "primary_replicas", std::move(primary_replicas_builder).to_datum());

    ql::datum_array_builder_t replicas_builder(ql::configured_limits_t::unlimited);
    for (const auto &replica : replicas) {
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
            replica_builder.overwrite(
                "states", convert_set_to_datum<std::string>(
                    &convert_string_to_datum, replica.second));
            replicas_builder.add(std::move(replica_builder).to_datum());
        }
    }
    builder.overwrite("replicas", std::move(replicas_builder).to_datum());

    return std::move(builder).to_datum();
}

bool convert_table_status_to_datum(
        const namespace_id_t &table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_and_shards_t &config_and_shards,
        signal_t *interruptor,
        admin_identifier_format_t identifier_format,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        ql::datum_t *datum_out,
        std::string *error_out,
        table_readiness_t *readiness_out) {
    /* Note that `contracts` and `latest_contracts` will contain references into
       `contracts_and_acks`, thus this must remain in scope for them to be valid! */
    std::map<server_id_t, contracts_and_contract_acks_t> contracts_and_acks;
    std::map<
            contract_id_t,
            std::reference_wrapper<const std::pair<region_t, contract_t> >
        > contracts;
    server_id_t latest_contracts_server_id;
    if (!get_contracts_and_acks(
            table_id,
            interruptor,
            table_meta_client,
            server_config_client,
            &contracts_and_acks,
            &contracts,
            &latest_contracts_server_id)) {
        if (error_out != nullptr) {
            *error_out = strprintf(
                "Lost contact with the server(s) hosting table `%s.%s`.",
                uuid_to_str(config_and_shards.config.database).c_str(),
                uuid_to_str(table_id).c_str());
        }
        return false;
    }
    const std::map<contract_id_t, std::pair<region_t, contract_t> > &latest_contracts =
        contracts_and_acks.at(latest_contracts_server_id).contracts;

    std::vector<std::pair<region_t, server_acks_t> > regions_and_values;
    regions_and_values.reserve(latest_contracts.size());
    for (const auto &latest_contract : latest_contracts) {
        server_acks_t server_acks;
        server_acks.latest_contract_id = latest_contract.first;
        server_acks.acks = {};
        regions_and_values.emplace_back(
            std::make_pair(latest_contract.second.first, std::move(server_acks)));
    }
    region_map_t<server_acks_t> regions(
        regions_and_values.begin(), regions_and_values.end());

    for (const auto &server : contracts_and_acks) {
        for (const auto &contract_ack : server.second.contract_acks) {
            auto contract_it = contracts.find(contract_ack.first);
            if (contract_it == contracts.end()) {
                /* When the executor is being reset we may receive acknowledgements for
                   contracts that are no longer in the set of all contracts. Ignoring
                   these will at worse result in a pessimistic status, which is fine
                   when this function is being used as part of `table_wait`. */
                continue;
            }
            region_map_t<server_acks_t> masked_regions =
                regions.mask(contract_it->second.get().first);
            for (auto &masked_region : masked_regions) {
                masked_region.second.acks.insert(
                    std::make_pair(server.first, contract_ack));
            }
            regions.update(masked_regions);
        }
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(config_and_shards.config.name));

    table_readiness_t readiness = table_readiness_t::finished;
    ql::datum_array_builder_t shards_builder(ql::configured_limits_t::unlimited);
    for (size_t i = 0; i < config_and_shards.shard_scheme.num_shards(); ++i) {
        region_t region(config_and_shards.shard_scheme.get_shard_range(i));
        table_readiness_t shard_readiness;
        shards_builder.add(
            convert_table_status_shard_to_datum(
                config_and_shards.config.shards.at(i),
                regions.mask(region),
                contracts_and_acks,
                contracts,
                identifier_format,
                server_config_client,
                &shard_readiness));
        readiness = std::min(readiness, shard_readiness);
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

    if (readiness_out != nullptr) {
        *readiness_out = readiness;
    }
    if (datum_out != nullptr) {
        *datum_out = std::move(builder).to_datum();
    }
    return true;
}

bool table_status_artificial_table_backend_t::format_row(
        namespace_id_t table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_and_shards_t &config_and_shards,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    assert_thread();
    return convert_table_status_to_datum(
        table_id,
        db_name_or_uuid,
        config_and_shards,
        interruptor,
        identifier_format,
        table_meta_client,
        server_config_client,
        row_out,
        error_out,
        nullptr);
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

    table_config_and_shards_t config_and_shards;
    if (!backend->table_meta_client->get_config(
            table_id, interruptor, &config_and_shards)) {
        /* We couldn't get the `config_and_shards` for this table due to an unknown
           reason, the best option we have at this point is to report it deleted. */
        return table_wait_result_t::DELETED;
    }

    cluster_semilattice_metadata_t metadata = backend->semilattice_view->get();

    bool immediate = true;
    /* Start with `initial_poll_ms`, then double the waiting period after each attempt up
       to a maximum of `max_poll_ms`. */
    uint32_t current_poll_ms = initial_poll_ms;
    while (true) {
        ql::datum_t db_name_or_uuid;
        name_string_t db_name;
        if (!convert_table_id_to_datums(
                table_id,
                backend->identifier_format,
                metadata,
                backend->table_meta_client,
                nullptr,
                nullptr,
                &db_name_or_uuid,
                &db_name) || db_name.str() == "__deleted_database__") {
            // Either the database or the table was deleted.
            return table_wait_result_t::DELETED;
        }

        ql::datum_t status;
        table_readiness_t readiness;
        bool table_status_result = convert_table_status_to_datum(
            table_id,
            db_name_or_uuid,
            config_and_shards,
            interruptor,
            backend->identifier_format,
            backend->table_meta_client,
            backend->server_config_client,
            &status,
            nullptr,
            &readiness);

        if (table_status_result && readiness >= wait_readiness) {
            if (status_out != nullptr) {
                *status_out = std::move(status);
            }
            return immediate
                ? table_wait_result_t::IMMEDIATE
                : table_wait_result_t::WAITED;
        } else {
            immediate = false;
            nap(current_poll_ms, interruptor);
            current_poll_ms = std::min(max_poll_ms, current_poll_ms * 2u);
        }
    }
}
