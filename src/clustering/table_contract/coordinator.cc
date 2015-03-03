// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/coordinator.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_contract/cpu_sharding.hpp"

/* A `contract_ack_t` is not necessarily homogeneous. It may have different `version_t`s
for different regions, and a region with a single `version_t` may need to be split
further depending on the branch history. Since `calculate_contract()` assumes it's
processing a homogeneous input, we need to break the `contract_ack_t` into homogeneous
pieces. `contract_ack_frag_t` is like a homogeneous version of `contract_ack_t`; in place
of the `region_map_t<version_t>` it has a single `state_timestamp_t`. Use
`break_ack_into_fragments()` to convert a `contract_ack_t` into a
`region_map_t<contract_ack_frag_t>`. */

class contract_ack_frag_t {
public:
    contract_ack_t::state_t state;
    boost::optional<state_timestamp_t> version;
    boost::optional<branch_id_t> branch;
};

region_map_t<contract_ack_frag_t> break_ack_into_fragments(
        const region_t &region,
        const contract_ack_t &ack,
        const branch_id_t &branch,
        const branch_history_reader_t *raft_branch_history) {
    contract_ack_frag_t base_frag;
    base_frag.state = ack.state;
    base_frag.branch = ack.branch;
    if (!static_cast<bool>(ack.version)) {
        return region_map_t<contract_ack_frag_t>(region, base_frag);
    } else {
        branch_history_combiner_t combined_branch_history(
            raft_branch_history, &ack.branch_history);
        std::vector<std::pair<region_t, contract_ack_frag_t> > parts;
        for (const std::pair<region_t, version_t> &vers_pair : *ack.version) {
            region_map_t<version_t> points_on_canonical_branch =
                version_find_branch_common(
                    &combined_branch_history,
                    vers_pair.second,
                    branch,
                    region);
            for (const auto &can_pair : points_on_canonical_branch) {
                base_frag.version = boost::make_optional(can_pair.second.timestamp);
                parts.push_back(std::make_pair(can_pair.first, base_frag));
            }
        }
        return region_map_t<contract_ack_frag_t>(parts.begin(), parts.end());
    }
}

/* `calculate_contract()` calculates a new contract for a region. Whenever any of the
inputs changes, the coordinator will call `update_contract()` to compute a contract for
each range of keys. The new contract will often be the same as the old, in which case it
doesn't get a new contract ID. */
contract_t calculate_contract(
        /* The old contract that contains this region. */
        const contract_t &old_c,
        /* The user-specified configuration for the shard containing this region. */
        const table_config_t::shard_t &config,
        /* Contract acks from replicas regarding `old_c`. If a replica hasn't sent us an
        ack *specifically* for `old_c`, it won't appear in this map; we don't include
        acks for contracts that were in the same region before `old_c`. */
        const std::map<server_id_t, contract_ack_frag_t> &acks) {

    contract_t new_c = old_c;

    /* If there are new servers in `config.replicas`, add them to `c.replicas` */
    for (const server_id_t &server : config.replicas) {
        if (new_c.replicas.count(server) == 0) {
            new_c.replicas.insert(server);
        }
    }

    /* If there is a mismatch between `config.replicas` and `c.voters`, then correct
    it */
    if (!static_cast<bool>(old_c.temp_voters) && old_c.voters != config.replicas) {
        size_t num_streaming = 0;
        for (const server_id_t &server : config.replicas) {
            auto it = acks.find(server);
            if (it != acks.end() &&
                    (it->second.state == contract_ack_t::state_t::secondary_streaming ||
                    (static_cast<bool>(old_c.primary) &&
                        old_c.primary->server == server))) {
                ++num_streaming;
            }
        }

        /* We don't want to initiate the change until a majority of the new replicas are
        already streaming, or else we'll lose write availability as soon as we set
        `temp_voters`. */
        if (num_streaming > config.replicas.size() / 2) {
            /* OK, we're ready to go */
            new_c.temp_voters = boost::make_optional(config.replicas);
        }
    }

    /* If we already initiated a voter change by setting `temp_voters`, it might be time
    to commit that change by setting `voters` to `temp_voters`. */
    if (static_cast<bool>(old_c.temp_voters)) {
        /* Before we change `voters`, we have to make sure that we'll preserve the
        invariant that every acked write is on a majority of `voters`. This is mostly the
        job of the primary; it will not report `primary_running` unless it is requiring
        acks from a majority of both `voters` and `temp_voters` before acking writes to
        the client, *and* it has ensured that every write that was acked before that
        policy was implemented has been backfilled to a majority of `temp_voters`. So we
        can't switch voters unless the primary reports `primary_running`. */
        if (static_cast<bool>(old_c.primary) &&
                acks.count(old_c.primary->server) == 1 &&
                acks.at(old_c.primary->server).state ==
                    contract_ack_t::state_t::primary_ready) {
            /* OK, it's safe to commit. */
            new_c.voters = *new_c.temp_voters;
            new_c.temp_voters = boost::none;
        }
    }

    /* If a server was removed from `config.replicas` and `c.voters` but it's still in
    `c.replicas`, and it's not primary, then remove it. (If it is primary, it won't be
    for long, because we'll detect this case and switch to another primary.) */
    bool should_kill_primary = false;
    for (const server_id_t &server : old_c.replicas) {
        if (config.replicas.count(server) == 0 &&
                old_c.voters.count(server) == 0 &&
                (!static_cast<bool>(old_c.temp_voters) ||
                    old_c.temp_voters->count(server) == 0)) {
            if (static_cast<bool>(old_c.primary) && old_c.primary->server == server) {
                /* We'll process this case further down. */
                should_kill_primary = true;
            } else {
                new_c.replicas.erase(server);
            }
        }
    }

    /* If we don't have a primary, choose a primary. Servers are not eligible to be a
    primary unless they are carrying every acked write. In addition, we must choose
    `config.primary_replica` if it is eligible. There will be at least one eligible
    server if and only if we have reports from a majority of `new_c.voters`. */
    if (!static_cast<bool>(old_c.primary)) {
        /* We have an invariant that every acked write must be on the path from the root
        of the branch history to `old_c.branch`. So we project each voter's state onto
        that path, then sort them by position along the path. Any voter that is at least
        as up to date, according to that metric, as more than half of the voters
        (including itself) is eligible. */

        /* First, collect and sort the states from the servers. Note that we use the
        server ID as a secondary sorting key. This mean we tend to pick the same server
        if we run the algorithm twice; this helps to reduce unnecessary fragmentation. */
        std::vector<std::pair<state_timestamp_t, server_id_t> > replica_states;
        for (const server_id_t &server : new_c.voters) {
            if (acks.count(server) == 1 && acks.at(server).state ==
                    contract_ack_t::state_t::secondary_need_primary) {
                replica_states.push_back(
                    std::make_pair(*acks.at(server).version, server));
            }
        }
        std::sort(replica_states.begin(), replica_states.end());

        /* Second, select a new one. This loop is a little convoluted; it will set
        `new_primary` to `config.primary_replica` if eligible, otherwise the most
        up-to-date other server if there is one, otherwise nothing. */
        boost::optional<server_id_t> new_primary;
        for (size_t i = new_c.voters.size() / 2; i < replica_states.size(); ++i) {
            new_primary = replica_states[i].second;
            if (replica_states[i].second == config.primary_replica) {
                break;
            }
        }

        /* RSI(raft): If `config.primary_replica` isn't connected or isn't ready, we
        should elect a different primary. If `config.primary_replica` is connected but
        just takes a little bit longer to reply to our contracts than the other replicas,
        we should wait for it to reply to our contracts and then elect it. Under the
        current implementation, we don't wait. This could lead to an awkward loop, where
        we elect the wrong primary, then un-elect it because we realize
        `config.primary_replica` is ready, and then re-elect the wrong primary instead of
        electing `config.primary_replica`. */

        if (static_cast<bool>(new_primary)) {
            contract_t::primary_t p;
            p.server = *new_primary;
            new_c.primary = boost::make_optional(p);
        }
    }

    /* Sometimes we already have a primary, but we need to pick a different one. There
    are three such situations:
    - The existing primary is disconnected
    - The existing primary isn't `config.primary_replica`, and `config.primary_replica`
        is ready to take over the role
    - `config.primary_replica` isn't ready to take over the role, but the existing
        primary isn't even supposed to be a replica anymore.
    In the first situation, we'll simply remove `c.primary`. In the second and third
    situations, we'll first set `c.primary->warm_shutdown` to `true`, and then only
    once the primary acknowledges that, we'll remove `c.primary`. Either way, once the
    replicas acknowledge the contract in which we removed `c.primary`, the logic earlier
    in this function will select a new primary. Note that we can't go straight from the
    old primary to the new one; we need a majority of replicas to promise to stop
    receiving updates from the old primary before it's safe to elect a new one. */
    if (static_cast<bool>(old_c.primary)) {
        /* Note we already checked for the case where the old primary wasn't supposed to
        be a replica. If this is so, then `should_kill_primary` will already be set to
        `true`. */

        /* Check if we need to do an auto-failover. The precise form of this condition
        isn't important for correctness. If we do an auto-failover when the primary isn't
        actually dead, or don't do an auto-failover when the primary is actually dead,
        the worst that will happen is we'll lose availability.

        RSI(raft): Maybe wait until the primary stays down for a certain amount of time
        before failing over. */
        size_t voters_cant_reach_primary = 0;
        for (const server_id_t &server : new_c.voters) {
            if (acks.count(server) == 1 && acks.at(server).state ==
                    contract_ack_t::state_t::secondary_need_primary) {
                ++voters_cant_reach_primary;
            }
        }
        if (voters_cant_reach_primary > new_c.voters.size() / 2) {
            should_kill_primary = true;
        }

        if (should_kill_primary) {
            new_c.primary = boost::none;
        } else if (old_c.primary->server != config.primary_replica &&
                acks.count(config.primary_replica) == 1 &&
                acks.at(config.primary_replica).state ==
                    contract_ack_t::state_t::secondary_streaming) {
            /* The old primary is still a valid replica, but it isn't equal to
            `config.primary_replica`. So we have to do a hand-over to ensure that after
            we kill the primary, `config.primary_replica` will be a valid candidate. */

            if (old_c.primary->hand_over == config.primary_replica &&
                    acks.count(old_c.primary->server) == 1 &&
                    acks.at(old_c.primary->server).state ==
                        contract_ack_t::state_t::primary_ready) {
                /* We already did the hand over. Now it's safe to stop the old primary.
                */
                new_c.primary = boost::none;
            } else {
                new_c.primary->hand_over = boost::make_optional(config.primary_replica);
            }
        } else {
            /* We're sticking with the current primary, so `hand_over` should be empty.
            In the unlikely event that we were in the middle of a hand-over and then
            changed our minds, it might not be empty, so we clear it manually. */
            new_c.primary->hand_over = boost::none;
        }
    }

    /* Register a branch if a primary is asking us to */
    if (static_cast<bool>(old_c.primary) &&
            static_cast<bool>(new_c.primary) &&
            old_c.primary->server == new_c.primary->server &&
            acks.count(old_c.primary->server) == 1 &&
            acks.at(old_c.primary->server).state ==
                contract_ack_t::state_t::primary_need_branch) {
        new_c.branch = *acks.at(old_c.primary->server).branch;
    }

    return new_c;
}

/* `calculate_all_contracts()` is sort of like `calculate_contract()` except that it
applies to the whole set of contracts instead of to a single contract. It takes the
inputs that `calculate_contract()` needs, but in sharded form; then breaks the key space
into small enough chunks that the inputs are homogeneous across each chunk; then calls
`calculate_contract()` on each chunk.

The output is in the form of a diff instead of a set of new contracts. We need a diff to
put in the `table_raft_state_t::change_t::new_contracts_t`, and we need to compute the
diff anyway in order to reuse contract IDs for contracts that haven't changed, so it
makes sense to combine those two diff processes. */
void calculate_all_contracts(
        const table_raft_state_t &old_state,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks,
        std::set<contract_id_t> *remove_contracts_out,
        std::map<contract_id_t, std::pair<region_t, contract_t> > *add_contracts_out) {

    ASSERT_FINITE_CORO_WAITING;

    std::vector<std::pair<region_t, contract_t> > new_contract_vector;

    /* We want to break the key-space into sub-regions small enough that the contract,
    table config, and ack versions are all constant across the sub-region. First we
    iterate over all contracts: */
    for (const std::pair<contract_id_t, std::pair<region_t, contract_t> > &cpair :
            old_state.contracts) {
        /* Next iterate over all shards of the table config and find the ones that
        overlap the contract in question: */
        for (size_t si = 0; si < old_state.config.config.shards.size(); ++si) {
            region_t region = region_intersection(
                cpair.second.first,
                region_t(old_state.config.shard_scheme.get_shard_range(si)));
            if (region_is_empty(region)) {
                continue;
            }
            /* Now collect the acks for this contract into `ack_frags`. `ack_frags` is
            homogeneous at first and then it gets fragmented as we iterate over `acks`.
            */
            region_map_t<std::map<server_id_t, contract_ack_frag_t> > frags_by_server(
                region);
            acks->read_all([&](
                    const std::pair<server_id_t, contract_id_t> &key,
                    const contract_ack_t *value) {
                if (key.second != cpair.first) {
                    return;
                }
                region_map_t<contract_ack_frag_t> frags = break_ack_into_fragments(
                    region, *value, cpair.second.second.branch,
                    &old_state.branch_history);
                for (const auto &fpair : frags) {
                    auto part_of_frags_by_server = frags_by_server.mask(fpair.first);
                    for (auto &&fspair : part_of_frags_by_server) {
                        fspair.second.insert(std::make_pair(key.first, fpair.second));
                    }
                    frags_by_server.update(part_of_frags_by_server);
                }
            });
            for (const auto &apair : frags_by_server) {
                /* We've finally collected all the inputs to `calculate_contract()` and
                broken the key space into regions across which the inputs are
                homogeneous. So now we can actually call it. */
                contract_t new_contract = calculate_contract(
                    cpair.second.second,
                    old_state.config.config.shards[si],
                    apair.second);
                new_contract_vector.push_back(std::make_pair(apair.first, new_contract));
            }
        }
    }

    /* Put the new contracts into a `region_map_t` to coalesce adjacent regions that have
    identical contracts */
    region_map_t<contract_t> new_contract_region_map(
        new_contract_vector.begin(), new_contract_vector.end());

    /* Slice the new contracts by CPU shard, so that no contract spans more than one CPU
    shard */
    std::map<region_t, contract_t> new_contract_map;
    for (size_t cpu = 0; cpu < CPU_SHARDING_FACTOR; ++cpu) {
        region_t cpu_region = cpu_sharding_subspace(cpu);
        for (const auto &pair : new_contract_region_map.mask(cpu_region)) {
            guarantee(pair.first.beg == cpu_region.beg &&
                pair.first.end == cpu_region.end);
            new_contract_map.insert(pair);
        }
    }

    /* Diff the new contracts against the old contracts */
    for (const auto &cpair : old_state.contracts) {
        auto it = new_contract_map.find(cpair.second.first);
        if (it != new_contract_map.end() && it->second == cpair.second.second) {
            /* The contract was unchanged. Remove it from `new_contract_map` to signal
            that we don't need to assign it a new ID. */
            new_contract_map.erase(it);
        } else {
            /* The contract was changed. So delete the old one. */
            remove_contracts_out->insert(cpair.first);
        }
    }
    for (const auto &pair : new_contract_map) {
        /* The contracts remaining in `new_contract_map` are actually new; whatever
        contracts used to cover their region have been deleted. So assign them contract
        IDs and export them. */
        add_contracts_out->insert(std::make_pair(generate_uuid(), pair));
    }
}

/* `calculate_branch_history()` figures out what changes need to be made to the branch
history stored in the Raft state. In practice this means two things:
  - When a new primary asks us to register a branch, we copy it and relevant ancestors
    into the branch history.
  - When all of the replicas are known to be descended from a certain point in the branch
    history, we prune the history leading up to that point, since it's no longer needed.
*/
void calculate_branch_history(
        const table_raft_state_t &old_state,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks,
        const std::set<contract_id_t> &remove_contracts,
        const std::map<contract_id_t, std::pair<region_t, contract_t> > &add_contracts,
        std::set<branch_id_t> *remove_branches_out,
        branch_history_t *add_branches_out) {
    /* RSI(raft): This is a totally naive implementation that never prunes branches and
    sometimes adds unnecessary branches. */
    (void)old_state;
    (void)remove_contracts;
    (void)add_contracts;
    (void)remove_branches_out;
    acks->read_all([&](
            const std::pair<server_id_t, contract_id_t> &,
            const contract_ack_t *ack) {
        if (static_cast<bool>(ack->branch)) {
            ack->branch_history.export_branch_history(*ack->branch, add_branches_out);
        }
    });
}

contract_coordinator_t::contract_coordinator_t(
        raft_member_t<table_raft_state_t> *_raft,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *_acks) :
    raft(_raft), acks(_acks), wake_pump_contracts(new cond_t),
    ack_subs(acks,
        [this](const std::pair<server_id_t, contract_id_t> &, const contract_ack_t *)
            { wake_pump_contracts->pulse_if_not_already_pulsed(); },
        false)
{
    /* Do an initial round of pumping, in case there are any changes the previous
    coordinator didn't take care of */
    wake_pump_contracts->pulse_if_not_already_pulsed();
}
        

boost::optional<raft_log_index_t> contract_coordinator_t::change_config(
        const std::function<void(table_config_and_shards_t *)> &changer,
        signal_t *interruptor) {
    scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t> change_token;
    raft_log_index_t log_index;
    {
        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);
        table_raft_state_t::change_t::set_table_config_t change;
        bool is_noop;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
            change.new_config = state->state.config;
            changer(&change.new_config);
            log_index = state->log_index;
            is_noop = (change.new_config == state->state.config);
        });
        if (is_noop) {
            return boost::make_optional(log_index);
        }
        /* We don't want to actually interrupt `propose_change()` unless the
        `raft_member_t` is about to be destroyed, because doing so will leave the
        `raft_member_t` in a bad state. If our `auto_drainer_t` is destroyed, that could
        be because we're shutting down or it could be because we're just changing state;
        we can't tell the difference. So to be on the safe side, we never interrupt
        `propose_change()`. */
        cond_t non_interruptor;
        change_token = raft->propose_change(
            &change_lock, table_raft_state_t::change_t(change), &non_interruptor);
        log_index += 1;
    }
    if (!change_token.has()) {
        return boost::none;
    }
    wake_pump_contracts->pulse_if_not_already_pulsed();
    wait_interruptible(change_token->get_ready_signal(), interruptor);
    if (!change_token->wait()) {
        return boost::none;
    }
    return boost::make_optional(log_index);
}

void contract_coordinator_t::pump_contracts(auto_drainer_t::lock_t keepalive) {
    try {
        while (!keepalive.get_drain_signal()->is_pulsed()) {
            /* Wait until something changes that requires us to update the state */
            wait_interruptible(wake_pump_contracts.get(), keepalive.get_drain_signal());

            /* Wait a little longer to give changes time to accumulate, because
            `calculate_all_contracts()` needs to examine every shard of the table even if
            nothing about them has changed, and that might be expensive. */
            signal_timer_t buffer_timer;
            buffer_timer.start(200);
            wait_interruptible(&buffer_timer, keepalive.get_drain_signal());

            /* Now we'll apply changes to Raft. We keep trying in a loop in case it
            doesn't work at first. */
            do {
                /* Wait until the Raft member is likely to accept changes */
                raft->get_readiness_for_change()->run_until_satisfied(
                    [](bool is_ready) { return is_ready; },
                    keepalive.get_drain_signal());

                raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft,
                    keepalive.get_drain_signal());

                /* Reset `wake_pump_contracts` here. Any changes in the inputs that
                happened up to this point will be included in this round of
                `calculate_all_contracts()`, but any later changes might not be. */
                wake_pump_contracts = make_scoped<cond_t>(); 

                /* Calculate the proposed change */
                table_raft_state_t::change_t::new_contracts_t change;
                raft->get_latest_state()->apply_read(
                [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
                    calculate_all_contracts(state->state, acks,
                        &change.remove_contracts, &change.add_contracts);
                    calculate_branch_history(state->state, acks,
                        change.remove_contracts, change.add_contracts,
                        &change.remove_branches, &change.add_branches);
                });

                /* Apply the change, unless it's a no-op */
                if (!change.remove_contracts.empty() ||
                        !change.add_contracts.empty() ||
                        !change.remove_branches.empty() ||
                        !change.add_branches.branches.empty()) {
                    cond_t non_interruptor;
                    scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                        change_token = raft->propose_change(
                            &change_lock, table_raft_state_t::change_t(change),
                            &non_interruptor);

                    /* If the change failed, go back to the top of the loop and wait on
                    `get_readiness_for_change()` again. But if the change succeeded,
                    don't bother waiting on it; we won't start a redundant change because
                    we always compute the change by comparing to `get_latest_state()`. */
                    if (!change_token.has()) {
                        continue;
                    }
                }
            } while (false);
        }
    } catch (const interrupted_exc_t &) {
        /* We're shutting down or no longer Raft leader for some reason. */
    }
}


