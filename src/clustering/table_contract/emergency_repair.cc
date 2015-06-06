// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/emergency_repair.hpp"

/* Returns `true` if `servers` is a subset of `dead`. */
bool all_dead(
        const std::set<server_id_t> &servers, const std::set<server_id_t> &dead) {
    for (const server_id_t &server : servers) {
        if (dead.count(server) == 0) {
            return false;
        }
    }
    return true;
}

/* Returns `true` if half or more of the servers in `servers` are also in `dead`. */
bool quorum_dead(
        const std::set<server_id_t> &servers, const std::set<server_id_t> &dead) {
    size_t alive = 0;
    for (const server_id_t &server : servers) {
        if (dead.count(server) == 0) {
            ++alive;
        }
    }
    return alive * 2 <= servers.size();
}

void calculate_emergency_repair(
        const table_raft_state_t &old_state,
        const std::set<server_id_t> &dead_servers,
        bool allow_erase,
        table_raft_state_t *new_state_out,
        bool *rollback_found_out,
        bool *erase_found_out) {
    *rollback_found_out = false;
    *erase_found_out = false;

    /* Pick the server we'll use as a replacement for shards that we end up erasing */
    server_id_t erase_replacement = nil_uuid();
    for (const auto &pair : old_state.member_ids) {
        if (dead_servers.count(pair.first) == 0) {
            erase_replacement = pair.first;
            break;
        }
    }
    guarantee(!erase_replacement.is_nil(), "calculate_emergency_repair() should not be "
        "called if all servers are dead");

    /* Calculate the new contracts. This is the guts of the repair operation. */
    new_state_out->contracts.clear();
    for (const auto &pair : old_state.contracts) {
        contract_t contract = pair.second.second;
        if (all_dead(contract.replicas, dead_servers)) {
            *erase_found_out = true;
            if (allow_erase) {
                /* Discard all previous replicas, and set up a new empty replica on
                `erase_replacement`. */
                contract = contract_t();
                contract.replicas.insert(erase_replacement);
                contract.voters.insert(erase_replacement);
            }
        } else if (quorum_dead(contract.voters, dead_servers) ||
                (static_cast<bool>(contract.temp_voters) &&
                    quorum_dead(*contract.temp_voters, dead_servers))) {
            *rollback_found_out = true;
            /* Remove the primary (a new one will be elected) */
            contract.primary = boost::none;
            /* Convert temp voters into voters */
            if (static_cast<bool>(contract.temp_voters)) {
                contract.voters.insert(
                    contract.temp_voters->begin(), contract.temp_voters->end());
                contract.temp_voters = boost::none;
            }
            /* Forcibly demote all dead voters */
            for (const server_id_t &server : dead_servers) {
                contract.voters.erase(server);
            }
            /* If all the voters were dead, then promote all living non-voters to voter
            */
            if (contract.voters.empty()) {
                for (const server_id_t &server : contract.replicas) {
                    if (dead_servers.count(server) == 0) {
                        contract.voters.insert(server);
                    }
                }
            }
        }
        /* We generate a new contract ID for the new contract, whether or not it's
        identical to the old contract. In practice this doesn't matter since contract IDs
        from the old epoch will never be compared to contract IDs from the new epoch. */
        new_state_out->contracts.insert(std::make_pair(
            generate_uuid(),
            std::make_pair(pair.second.first, contract)));
    }

    /* Calculate the new config to reflect the new contracts as closely as possible. The
    details here aren't critical for correctness purposes. */
    {
        /* Copy all the simple fields of the old config onto the new config */
        new_state_out->config.config.basic = old_state.config.config.basic;
        new_state_out->config.config.sindexes = old_state.config.config.sindexes;
        new_state_out->config.config.write_ack_config =
            old_state.config.config.write_ack_config;
        new_state_out->config.config.durability = old_state.config.config.durability;

        /* We first calculate all the voting and nonvoting replicas for each range in a
        `range_map_t`. */
        range_map_t<key_range_t::right_bound_t, table_config_t::shard_t> config(
            key_range_t::right_bound_t(store_key_t::min()),
            key_range_t::right_bound_t::make_unbounded(),
            table_config_t::shard_t());
        for (const auto &pair : new_state_out->contracts) {
            config.visit_mutable(
                key_range_t::right_bound_t(pair.second.first.inner.left),
                pair.second.first.inner.right,
                [&](const key_range_t::right_bound_t &,
                        const key_range_t::right_bound_t &,
                        table_config_t::shard_t *shard) {
                    for (const server_id_t &server : pair.second.second.replicas) {
                        if (pair.second.second.voters.count(server) == 1) {
                            shard->all_replicas.insert(server);
                            shard->nonvoting_replicas.erase(server);
                        } else {
                            if (shard->all_replicas.count(server) == 0) {
                                shard->all_replicas.insert(server);
                                shard->nonvoting_replicas.insert(server);
                            }
                        }
                    }
                });
        }

        /* Assign primaries. We keep the old primary if possible, otherwise we choose an
        arbitrary server. */
        for (size_t i = 0; i < old_state.config.config.shards.size(); ++i) {
            const table_config_t::shard_t &old_shard = old_state.config.config.shards[i];
            key_range_t shard_range = old_state.config.shard_scheme.get_shard_range(i);
            config.visit_mutable(
                key_range_t::right_bound_t(shard_range.left),
                shard_range.right,
                [&](const key_range_t::right_bound_t &,
                        const key_range_t::right_bound_t &,
                        table_config_t::shard_t *shard) {
                    /* Check if `old_shard.primary_replica` is eligible to be a primary
                    under the new config */
                    if (shard->voting_replicas().count(old_shard.primary_replica) == 1) {
                        /* It's eligible, so keep it */
                        shard->primary_replica = old_shard.primary_replica;
                    } else {
                        /* It's ineligible, so pick the first eligible server */
                        shard->primary_replica = *shard->voting_replicas().begin();
                    }
                });
        }

        /* Convert each section of the `range_map_t` into a new user-level shard. */
        config.visit(
            key_range_t::right_bound_t(store_key_t::min()),
            key_range_t::right_bound_t::make_unbounded(),
            [&](const key_range_t::right_bound_t &,
                    const key_range_t::right_bound_t &right,
                    const table_config_t::shard_t &shard) {
                new_state_out->config.config.shards.push_back(shard);
                if (!right.unbounded) {
                    new_state_out->config.shard_scheme.split_points
                        .push_back(right.key());
                }
            });
        guarantee(new_state_out->config.config.shards.size() ==
            new_state_out->config.shard_scheme.num_shards());

        /* Copy over server names for all servers that are in the new config, into the
        server names map for the config. (Note that there are two separate server name
        maps, one for the config and one for the contracts.) */
        for (const table_config_t::shard_t &shard :
                new_state_out->config.config.shards) {
            for (const server_id_t &server : shard.all_replicas) {
                /* Since the new config is derived from the new contracts which are
                derived from the old contracts, every server present in the new config
                must appear in `old_state.server_names`. */
                new_state_out->config.server_names.names.insert(
                    std::make_pair(server, old_state.server_names.names.at(server)));
            }
        }
    }

    /* Copy over the branch history without modification. In theory we could do some
    pruning here, but there's not much that we could prune and so it's easier to just let
    the branch history GC do it. */
    new_state_out->branch_history = old_state.branch_history;

    /* Copy over the current branch map as well. Where the new contracts specify
    a primary that doesn't have the current branch due to the hard override, the
    current branch will be reset for its range once it registers its new branch. */
    new_state_out->current_branches = old_state.current_branches;

    /* Find all the servers that appear in the new contracts, and put entries for those
    servers in `member_ids` and `server_names` */
    for (const auto &pair : new_state_out->contracts) {
        for (const server_id_t &server : pair.second.second.replicas) {
            if (new_state_out->member_ids.count(server) == 0) {
                /* We generate a new member ID for every server. This shouldn't matter
                because member IDs from the new epoch should never be compared to member
                IDs from the old epoch, but it slightly reduces the risk of bugs. */
                new_state_out->member_ids.insert(
                    std::make_pair(server, raft_member_id_t(generate_uuid())));

                /* Since the new contracts are derived from the old contracts, every
                server present in the new contracts must appear in
                `old_state.server_names`. */
                new_state_out->server_names.names.insert(
                    std::make_pair(server, old_state.server_names.names.at(server)));
            }
        }
    }
}

