// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/calculate_status.hpp"

#include <algorithm>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/table_contract/exec_primary.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

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
        server_id_t *latest_contracts_server_id_out,
        server_name_map_t *server_names_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    std::map<peer_id_t, contracts_and_contract_acks_t> contracts_and_acks;
    table_meta_client->get_status(table_id, interruptor, nullptr, &contracts_and_acks);

    multi_table_manager_bcard_t::timestamp_t latest_timestamp;
    latest_timestamp.epoch.timestamp = 0;
    latest_timestamp.epoch.id = nil_uuid();
    latest_timestamp.log_index = 0;
    for (const auto &peer : contracts_and_acks) {
        /* Determine the server ID that goes with this peer */
        boost::optional<server_id_t> server_id =
            server_config_client->get_peer_to_server_map()->get_key(peer.first);
        if (!static_cast<bool>(server_id)) {
            /* This can only happen because of a race condition. */
            continue;
        }

        /* Look up the name of the server in question. We need to do this separately
        because it's hypothetically possible for us to get contract acks from a server
        that doesn't appear in any contract, and we need to make sure that every server
        ID that appears in `contracts_and_acks_out` also appears in `server_names_out`.
        */
        bool found_name = false;
        server_config_client->get_server_config_map()->read_key(*server_id,
            [&](const server_config_versioned_t *config) {
                if (config != nullptr) {
                    server_names_out->names.insert(std::make_pair(*server_id,
                        std::make_pair(config->version, config->config.name)));
                    found_name = true;
                }
            });
        if (!found_name) {
            /* This can only happen because of a race condition. */
            continue;
        }

        auto pair = contracts_and_acks_out->insert(
            std::make_pair(server_id.get(), std::move(peer.second)));
        contracts_out->insert(
            pair.first->second.contracts.begin(),
            pair.first->second.contracts.end());
        server_names_out->names.insert(
            pair.first->second.server_names.names.begin(),
            pair.first->second.server_names.names.end());
        if (pair.first->second.timestamp.supersedes(latest_timestamp)) {
            *latest_contracts_server_id_out = pair.first->first;
            latest_timestamp = pair.first->second.timestamp;
        }
    }

    return !contracts_and_acks_out->empty();
}

struct region_acks_t {
    bool operator==(const region_acks_t &other) const {
        return latest_contract_id == other.latest_contract_id && acks == other.acks;
    }

    contract_id_t latest_contract_id;
    std::map<server_id_t, std::pair<contract_id_t, contract_ack_t> > acks;
};

shard_status_t calculate_shard_status(
        const table_config_t::shard_t &shard,
        const region_map_t<region_acks_t> &regions,
        const std::map<server_id_t, contracts_and_contract_acks_t> &contracts_and_acks,
        const std::map<
                contract_id_t,
                std::reference_wrapper<const std::pair<region_t, contract_t> >
            > &contracts) {
    shard_status_t shard_status;

    /* The status defaults to `table_readiness_t::unavailable`, these are the
       requirements for the increasingly stronger statuses:

       - `table_readiness_t::outdated_reads`, reported as "ready_for_outdated_reads"
         requires that either a primary or secondary is ready.
       - `table_readiness_t::reads`, reported as "ready_for_reads" requires that a
         primary is ready.
       - `table_readiness_t::writes`, reported as "ready_for_writes" requires that a
         primary is ready and that there is a quorum to acknowledge the writes.
       - `table_readiness_t::finished`, reported as "all_replicas_ready" requires a
         primary to be ready, a quorum to acknowledge writes, and that the shard matches
         the configuration with regard to the primary, the set of secondaries, and the
         shard boundaries. */

    bool as_configured = true;
    bool has_quorum = true;
    bool has_primary_replica = true;
    bool has_outdated_reader = true;

    regions.visit(regions.get_domain(),
    [&](const region_t &, const region_acks_t &region_acks) {
        const std::pair<region_t, contract_t> &latest_contract =
            contracts.at(region_acks.latest_contract_id).get();

        /* Verify the shard boundaries match the configuration, important for
           rebalancing. */
        as_configured &= (latest_contract.first.inner == regions.get_domain().inner);

        std::set<server_id_t> region_replicas;
        ack_counter_t ack_counter(latest_contract.second);
        bool region_has_primary_replica = false;
        bool region_has_outdated_reader = false;

        for (const auto &ack : region_acks.acks) {
            server_status_t ack_server_status = server_status_t::DISCONNECTED;
            switch (ack.second.second.state) {
                case contract_ack_t::state_t::primary_need_branch:
                    region_has_outdated_reader = true;
                    ack_server_status = server_status_t::WAITING_FOR_QUORUM;
                    break;
                case contract_ack_t::state_t::secondary_need_primary:
                    region_has_outdated_reader = true;
                    ack_server_status = server_status_t::WAITING_FOR_PRIMARY;
                    break;
                case contract_ack_t::state_t::primary_in_progress:
                case contract_ack_t::state_t::primary_ready:
                    as_configured &= (ack.first == shard.primary_replica);
                    // The primary is part of the replicas in the contract
                    region_replicas.insert(ack.first);
                    ack_counter.note_ack(ack.first);
                    region_has_primary_replica = true;
                    region_has_outdated_reader = true;

                    shard_status.primary_replicas.insert(ack.first);
                    ack_server_status = server_status_t::READY;
                    break;
                case contract_ack_t::state_t::secondary_backfilling:
                    ack_server_status = server_status_t::BACKFILLING;
                    break;
                case contract_ack_t::state_t::secondary_streaming:
                    {
                        const boost::optional<contract_t::primary_t> &region_primary =
                            contracts.at(ack.second.first).get().second.primary;
                        if (static_cast<bool>(latest_contract.second.primary) &&
                                latest_contract.second.primary == region_primary) {
                            region_replicas.insert(ack.first);
                            ack_counter.note_ack(ack.first);
                            region_has_outdated_reader = true;
                            ack_server_status = server_status_t::READY;
                        } else {
                            ack_server_status = server_status_t::TRANSITIONING;
                        }
                    }
                    break;
                case contract_ack_t::state_t::nothing:
                    ack_server_status = server_status_t::NOTHING;
                    break;
            }
            /* The replica's `server_status_t` is initialised to `DISCONNECTED` upon
               construction, taking the maximum of the current and new status to
               aggregate them as explained in the header. */
            shard_status.replicas[ack.first] = std::max(
                shard_status.replicas[ack.first], ack_server_status);
        }

<<<<<<< HEAD
        std::set<server_id_t> replicas;
        replicas.insert(
            latest_contract.replicas.begin(), latest_contract.replicas.end());
        replicas.insert(shard.all_replicas.begin(), shard.all_replicas.end());
        for (const auto &replica : replicas) {
=======
        std::set<server_id_t> contract_and_shard_replicas;
        contract_and_shard_replicas.insert(
            latest_contract.second.replicas.begin(),
            latest_contract.second.replicas.end());
        contract_and_shard_replicas.insert(shard.replicas.begin(), shard.replicas.end());
        for (const auto &replica : contract_and_shard_replicas) {
>>>>>>> Fixes `table_wait` to take the primary, secondaries, and shard boundries into account, as well as attempting to fetch the configuration from the leader first
            if (shard_status.replicas.find(replica) == shard_status.replicas.end()) {
                shard_status.replicas[replica] =
                    contracts_and_acks.find(replica) == contracts_and_acks.end()
                        ? server_status_t::DISCONNECTED
                        : server_status_t::TRANSITIONING;
            }
        }

        // Verify the replicas that are ready exactly match the configuration.
        as_configured &= (region_replicas == shard.replicas);
        has_quorum &= ack_counter.is_safe();
        has_primary_replica &= region_has_primary_replica;
        has_outdated_reader &= region_has_outdated_reader;
    });

    if (has_primary_replica) {
        if (has_quorum) {
            if (as_configured) {
                shard_status.readiness = table_readiness_t::finished;
            } else {
                shard_status.readiness = table_readiness_t::writes;
            }
        } else {
            shard_status.readiness = table_readiness_t::reads;
        }
    } else {
        if (has_outdated_reader) {
            shard_status.readiness = table_readiness_t::outdated_reads;
        } else {
            shard_status.readiness = table_readiness_t::unavailable;
        }
    }

    return shard_status;
}

void calculate_status(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        table_readiness_t *readiness_out,
        std::vector<shard_status_t> *shard_statuses_out,
        server_name_map_t *server_names_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t) {
<<<<<<< HEAD

=======
>>>>>>> Fixes `table_wait` to take the primary, secondaries, and shard boundries into account, as well as attempting to fetch the configuration from the leader first
    /* Note that `contracts` and `latest_contracts` will contain references into
       `contracts_and_acks`, thus this must remain in scope for them to be valid! */
    table_config_and_shards_t config_and_shards;
    std::map<server_id_t, contracts_and_contract_acks_t> contracts_and_acks;
    std::map<
            contract_id_t,
            std::reference_wrapper<const std::pair<region_t, contract_t> >
        > contracts;
    server_id_t latest_contracts_server_id;
    try {
        table_meta_client->get_config(table_id, interruptor, &config_and_shards);
        server_names_out->names.insert(
            config_and_shards.server_names.names.begin(),
            config_and_shards.server_names.names.end());
        get_contracts_and_acks(
            table_id,
            interruptor,
            table_meta_client,
            server_config_client,
            &contracts_and_acks,
            &contracts,
            &latest_contracts_server_id,
            server_names_out);
    } catch (const failed_table_op_exc_t &) {
        if (readiness_out != nullptr) {
            *readiness_out = table_readiness_t::unavailable;
        }
        if (shard_statuses_out != nullptr) {
            shard_statuses_out->clear();
        }
        return;
    }
    const std::map<contract_id_t, std::pair<region_t, contract_t> > &latest_contracts =
        contracts_and_acks.at(latest_contracts_server_id).contracts;

    region_map_t<region_acks_t> regions;
    {
        std::vector<region_t> regions_vec;
        std::vector<region_acks_t> acks_vec;
        regions_vec.reserve(latest_contracts.size());
        acks_vec.reserve(latest_contracts.size());
        std::vector<std::pair<region_t, region_acks_t> > regions_and_values;
        regions_and_values.reserve(latest_contracts.size());
        for (const auto &latest_contract : latest_contracts) {
            regions_vec.push_back(latest_contract.second.first);
            region_acks_t server_acks;
            server_acks.latest_contract_id = latest_contract.first;
            server_acks.acks = {};
            acks_vec.push_back(server_acks);
        }
        regions = region_map_t<region_acks_t>::from_unordered_fragments(
            std::move(regions_vec), std::move(acks_vec));
    }

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
            regions.visit_mutable(
                contract_it->second.get().first,
                [&](const region_t &, region_acks_t *region_acks) {
                    region_acks->acks.insert(std::make_pair(server.first, contract_ack));
                });
        }
    }

    if (readiness_out != nullptr) {
        *readiness_out = table_readiness_t::finished;
    }
    for (size_t i = 0; i < config_and_shards.shard_scheme.num_shards(); ++i) {
        region_t shard_region(config_and_shards.shard_scheme.get_shard_range(i));

        shard_status_t shard_status = calculate_shard_status(
            config_and_shards.config.shards.at(i),
            regions.mask(shard_region),
            contracts_and_acks,
            contracts);

        if (readiness_out != nullptr) {
            *readiness_out = std::min(*readiness_out, shard_status.readiness);
        }
        if (shard_statuses_out != nullptr) {
            shard_statuses_out->emplace_back(std::move(shard_status));
        }
    }
}
