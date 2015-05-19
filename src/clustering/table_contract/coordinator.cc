// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/coordinator.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "logger.hpp"

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
    bool operator==(const contract_ack_frag_t &x) const {
        return state == x.state && version == x.version && branch == x.branch;
    }
    bool operator!=(const contract_ack_frag_t &x) const {
        return !(*this == x);
    }

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
        return ack.version->map_multi(region,
            [&](const region_t &reg, const version_t &vers) {
                region_map_t<version_t> points_on_canonical_branch =
                    version_find_branch_common(&combined_branch_history,
                        vers, branch, reg);
                return points_on_canonical_branch.map(reg,
                    [&](const version_t &common_vers) {
                        base_frag.version = boost::make_optional(common_vers.timestamp);
                        return base_frag;
                    });
            });
    }
}

/* `invisible_to_majority_of_set()` returns `true` if `target` definitely cannot be seen
by a majority of the servers in `judges`. If we can't see one of the servers in `judges`,
we'll assume it can see `target` to reduce spurious failoves. */
bool invisible_to_majority_of_set(
        const server_id_t &target,
        const std::set<server_id_t> &judges,
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t> *
            connections_map) {
    size_t count = 0;
    for (const server_id_t &s : judges) {
        if (static_cast<bool>(connections_map->get_key(std::make_pair(s, target))) ||
                !static_cast<bool>(connections_map->get_key(std::make_pair(s, s)))) {
            ++count;
        }
    }
    return !(count > judges.size() / 2); 
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
        const std::map<server_id_t, contract_ack_frag_t> &acks,
        /* This `watchable_map_t` will have an entry for (X, Y) if we can see server X
        and server X can see server Y. */
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t> *
            connections_map,
        /* We'll print log messages of the form `<log prefix>: <message>`, unless
        `log_prefix` is empty, in which case we won't print anything. */
        const std::string &log_prefix) {

    contract_t new_c = old_c;

    /* If there are new servers in `config.replicas`, add them to `c.replicas` */
    new_c.replicas.insert(config.replicas.begin(), config.replicas.end());

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
            if (!log_prefix.empty()) {
                logINF("%s: Beginning replica set change.", log_prefix.c_str());
            }
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
            if (!log_prefix.empty()) {
                logINF("%s: Committed replica set change.", log_prefix.c_str());
            }
        }
    }

    /* `visible_voters` includes all members of `voters` and `temp_voters` which could be
    visible to a majority of `voters` (and `temp_voters`, if `temp_voters` exists). Note
    that if the coordinator can't see server X, it will assume server X can see every
    other server; this reduces spurious failovers when the coordinator loses contact with
    other servers. */
    std::set<server_id_t> visible_voters;
    for (const server_id_t &server : new_c.replicas) {
        if (new_c.voters.count(server) == 0 &&
                (!static_cast<bool>(new_c.temp_voters) ||
                    new_c.temp_voters->count(server) == 0)) {
            continue;
        }
        if (invisible_to_majority_of_set(server, new_c.voters, connections_map)) {
            continue;
        }
        if (static_cast<bool>(new_c.temp_voters)) {
            if (invisible_to_majority_of_set(
                    server, *new_c.temp_voters, connections_map)) {
                continue;
            }
        }
        visible_voters.insert(server);
    }

    /* If a server was removed from `config.replicas` and `c.voters` but it's still in
    `c.replicas`, then remove it. And if it's primary, then make it not be primary. */
    bool should_kill_primary = false;
    for (const server_id_t &server : old_c.replicas) {
        if (config.replicas.count(server) == 0 &&
                new_c.voters.count(server) == 0 &&
                (!static_cast<bool>(new_c.temp_voters) ||
                    new_c.temp_voters->count(server) == 0)) {
            new_c.replicas.erase(server);
            if (static_cast<bool>(old_c.primary) && old_c.primary->server == server) {
                /* Actual killing happens further down */
                should_kill_primary = true;
                if (!log_prefix.empty()) {
                    logINF("%s: Stopping server %s as primary because it's no longer a "
                        "voter.", log_prefix.c_str(), uuid_to_str(server).c_str());
                }
            }
        }
    }

    /* If we don't have a primary, choose a primary. Servers are not eligible to be a
    primary unless they are carrying every acked write. There will be at least one
    eligible server if and only if we have reports from a majority of `new_c.voters`.

    In addition, we must choose `config.primary_replica` if it is eligible. If
    `config.primary_replica` has not sent an ack, we must wait for the failover timeout
    to elapse before electing a different replica. This is to make sure that we won't
    elect the wrong replica simply because the user's designated primary took a little
    longer to send the ack. */
    if (!static_cast<bool>(old_c.primary)) {
        /* We have an invariant that every acked write must be on the path from the root
        of the branch history to `old_c.branch`. So we project each voter's state onto
        that path, then sort them by position along the path. Any voter that is at least
        as up to date, according to that metric, as more than half of the voters
        (including itself) is eligible. We also take into account whether a server is
        visible to its peers when deciding which server to select. */

        /* First, collect the states from the servers, and sort them by how up-to-date
        they are. Note that we use the server ID as a secondary sorting key. This mean we
        tend to pick the same server if we run the algorithm twice; this helps to reduce
        unnecessary fragmentation. */
        std::vector<std::pair<state_timestamp_t, server_id_t> > sorted_candidates;
        for (const server_id_t &server : new_c.voters) {
            if (acks.count(server) == 1 && acks.at(server).state ==
                    contract_ack_t::state_t::secondary_need_primary) {
                sorted_candidates.push_back(
                    std::make_pair(*(acks.at(server).version), server));
            }
        }
        std::sort(sorted_candidates.begin(), sorted_candidates.end());

        /* Second, determine which servers are eligible to become primary on the basis of
        their data and their visibility to their peers. */
        std::vector<server_id_t> eligible_candidates;
        for (size_t i = 0; i < sorted_candidates.size(); ++i) {
            server_id_t server = sorted_candidates[i].second;
            /* If the server is not visible to more than half of its peers, then it is
            not eligible to be primary */
            if (visible_voters.count(server) == 0) {
                continue;
            }
            /* `up_to_date_count` is the number of servers that `server` is at least as
            up-to-date as. We know `server` must be at least as up-to-date as itself and
            all of the servers that are earlier in the list. */
            size_t up_to_date_count = i + 1;
            /* If there are several servers with the same timestamp, they will appear
            together in the list. So `server` may be at least as up-to-date as some of
            the servers that appear after it in the list. */
            while (up_to_date_count < sorted_candidates.size() &&
                    sorted_candidates[up_to_date_count].first ==
                        sorted_candidates[i].first) {
                ++up_to_date_count;
            }
            /* OK, now `up_to_date_count` is the number of servers that this server is
            at least as up-to-date as. */
            if (up_to_date_count > new_c.voters.size() / 2) {
                eligible_candidates.push_back(server);
            }
        }

        /* OK, now we can pick a primary. */
        auto it = std::find(eligible_candidates.begin(), eligible_candidates.end(),
                config.primary_replica);
        if (it != eligible_candidates.end()) {
            /* The user's designated primary is eligible, so use it. */
            contract_t::primary_t p;
            p.server = config.primary_replica;
            new_c.primary = boost::make_optional(p);
        } else if (!eligible_candidates.empty()) {
            /* The user's designated primary is ineligible. We have to decide if we'll
            wait for the user's designated primary to become eligible, or use one of the
            other eligible candidates. */
            if (!config.primary_replica.is_nil() &&
                    visible_voters.count(config.primary_replica) == 1 &&
                    acks.count(config.primary_replica) == 0) {
                /* The user's designated primary is visible to a majority of its peers,
                and the only reason it was disqualified is because we haven't seen an ack
                from it yet. So we'll wait for it to send in an ack rather than electing
                a different primary. */
            } else {
                /* We won't wait for it. */
                contract_t::primary_t p;
                /* `eligible_candidates` is ordered by how up-to-date they are */
                p.server = eligible_candidates.back();
                new_c.primary = boost::make_optional(p);
            }
        }

        if (static_cast<bool>(new_c.primary)) {
            if (!log_prefix.empty()) {
                logINF("%s: Selected server %s as primary.", log_prefix.c_str(),
                    uuid_to_str(new_c.primary->server).c_str());
            }
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
        the worst that will happen is we'll lose availability. */
        if (visible_voters.count(old_c.primary->server) == 0) {
            should_kill_primary = true;
            if (!log_prefix.empty()) {
                logINF("%s: Stopping server %s as primary because a majority of voters "
                    "cannot reach it.", log_prefix.c_str(),
                    uuid_to_str(old_c.primary->server).c_str());
            }
        }

        if (should_kill_primary) {
            new_c.primary = boost::none;
        } else if (old_c.primary->server != config.primary_replica) {
            /* The old primary is still a valid replica, but it isn't equal to
            `config.primary_replica`. So we have to do a hand-over to ensure that after
            we kill the primary, `config.primary_replica` will be a valid candidate. */

            if (old_c.primary->hand_over !=
                    boost::make_optional(config.primary_replica)) {
                /* We haven't started the hand-over yet, or we're in the middle of a
                hand-over to a different primary. */
                if (acks.count(config.primary_replica) == 1 &&
                        acks.at(config.primary_replica).state ==
                            contract_ack_t::state_t::secondary_streaming &&
                        visible_voters.count(config.primary_replica) == 1) {
                    /* The new primary is ready, so begin the hand-over. */
                    new_c.primary->hand_over =
                        boost::make_optional(config.primary_replica);
                    if (!log_prefix.empty()) {
                        logINF("%s: Handing over primary from %s to %s to match table "
                            "config.", log_prefix.c_str(),
                            uuid_to_str(old_c.primary->server).c_str(),
                            uuid_to_str(config.primary_replica).c_str());
                    }
                } else {
                    /* We're not ready to switch to the new primary yet. */
                    if (static_cast<bool>(old_c.primary->hand_over)) {
                        /* We were in the middle of a hand over to a different primary,
                        and then the user changed `config.primary_replica`. But the new
                        primary isn't ready yet, so cancel the old hand-over. (This is
                        very uncommon.) */
                        new_c.primary->hand_over = boost::none;
                    }
                }
            } else {
                /* We're already in the process of handing over to the new primary. */
                if (acks.count(old_c.primary->server) == 1 &&
                        acks.at(old_c.primary->server).state ==
                            contract_ack_t::state_t::primary_ready) {
                    /* The hand over is complete. Now it's safe to stop the old primary.
                    The new primary will be started later, after a majority of the
                    replicas acknowledge that they are no longer listening for writes
                    from the old primary. */
                    new_c.primary = boost::none;
                } else if (visible_voters.count(config.primary_replica) == 0) {
                    /* Something went wrong with the new primary before the hand-over was
                    complete. So abort the hand-over. */
                    new_c.primary->hand_over = boost::none;
                }
            }
        } else {
            if (static_cast<bool>(old_c.primary->hand_over)) {
                /* We were in the middle of a hand over, but then the user changed
                `config.primary_replica` back to what it was before. (This is very
                uncommon.) */
                new_c.primary->hand_over = boost::none;
            }
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
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *connections_map,
        const std::string &log_prefix,
        std::set<contract_id_t> *remove_contracts_out,
        std::map<contract_id_t, std::pair<region_t, contract_t> > *add_contracts_out) {

    ASSERT_FINITE_CORO_WAITING;

    std::vector<region_t> new_contract_region_vector;
    std::vector<contract_t> new_contract_vector;

    /* We want to break the key-space into sub-regions small enough that the contract,
    table config, and ack versions are all constant across the sub-region. First we
    iterate over all contracts: */
    for (const std::pair<contract_id_t, std::pair<region_t, contract_t> > &cpair :
            old_state.contracts) {
        /* Next iterate over all shards of the table config and find the ones that
        overlap the contract in question: */
        for (size_t shard_index = 0; shard_index < old_state.config.config.shards.size();
                ++shard_index) {
            region_t region = region_intersection(
                cpair.second.first,
                region_t(old_state.config.shard_scheme.get_shard_range(shard_index)));
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
                frags.visit(region,
                [&](const region_t &reg, const contract_ack_frag_t &frag) {
                    frags_by_server.visit_mutable(reg,
                    [&](const region_t &,
                            std::map<server_id_t, contract_ack_frag_t> *acks_map) {
                        auto res = acks_map->insert(std::make_pair(key.first, frag));
                        guarantee(res.second);
                    });
                });
            });
            size_t subshard_index = 0;
            frags_by_server.visit(region,
            [&](const region_t &reg,
                    const std::map<server_id_t, contract_ack_frag_t> &acks_map) {
                /* We've finally collected all the inputs to `calculate_contract()` and
                broken the key space into regions across which the inputs are
                homogeneous. So now we can actually call it. */

                /* Compute a shard identifier for logging, of the form:
                    "shard <user shard>.<subshard>.<hash shard>"
                This relies on the fact that `visit()` goes first in subshard order and
                then in hash shard order; it increments `subshard_index` whenever it
                finds a region with `reg.beg` equal to zero. */
                std::string log_subprefix;
                if (!log_prefix.empty()) {
                    log_subprefix = strprintf("%s: shard %zu.%zu.%d",
                        log_prefix.c_str(),
                        shard_index, subshard_index, get_cpu_shard_approx_number(reg));
                    if (reg.end == HASH_REGION_HASH_SIZE) {
                        ++subshard_index;
                    }
                }

                contract_t new_contract = calculate_contract(
                    cpair.second.second,
                    old_state.config.config.shards[shard_index],
                    acks_map,
                    connections_map,
                    log_subprefix);
                new_contract_region_vector.push_back(reg);
                new_contract_vector.push_back(new_contract);
            });
        }
    }

    /* Put the new contracts into a `region_map_t` to coalesce adjacent regions that have
    identical contracts */
    region_map_t<contract_t> new_contract_region_map =
        region_map_t<contract_t>::from_unordered_fragments(
            std::move(new_contract_region_vector), std::move(new_contract_vector));

    /* Slice the new contracts by CPU shard, so that no contract spans more than one CPU
    shard */
    std::map<region_t, contract_t> new_contract_map;
    for (size_t cpu = 0; cpu < CPU_SHARDING_FACTOR; ++cpu) {
        region_t cpu_region = cpu_sharding_subspace(cpu);
        new_contract_region_map.visit(cpu_region,
        [&](const region_t &reg, const contract_t &contract) {
            guarantee(reg.beg == cpu_region.beg && reg.end == cpu_region.end);
            new_contract_map.insert(std::make_pair(reg, contract));
        });
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

/* `calculate_server_names()` figures out what changes need to be made to the server
names stored in the Raft state. When a contract is added that refers to a new server, it
will copy the server name from the table config; when the last contract that refers to a
server is removed, it will delete hte server name. */
void calculate_server_names(
        const table_raft_state_t &old_state,
        const std::set<contract_id_t> &remove_contracts,
        const std::map<contract_id_t, std::pair<region_t, contract_t> > &add_contracts,
        std::set<server_id_t> *remove_server_names_out,
        std::map<server_id_t, std::pair<uint64_t, name_string_t> >
            *add_server_names_out) {
    std::set<server_id_t> all_replicas;
    for (const auto &pair : old_state.contracts) {
        if (remove_contracts.count(pair.first) == 0) {
            all_replicas.insert(
                pair.second.second.replicas.begin(),
                pair.second.second.replicas.end());
        }
    }
    for (const auto &pair : add_contracts) {
        all_replicas.insert(
            pair.second.second.replicas.begin(),
            pair.second.second.replicas.end());
        for (const server_id_t &server : pair.second.second.replicas) {
            if (old_state.server_names.count(server) == 0) {
                add_server_names_out->insert(std:::make_pair(server,
                    old_state.config.server_names.at(server)));
            }
        }
    }
    for (const auto &pair : old_state.server_names) {
        if (all_replicas.count(pair.first) == 0) {
            remove_server_names_out->insert(pair.first);
        }
    }
}

/* `calculate_member_ids_and_raft_config()` figures out when servers need to be added to
or removed from the `table_raft_state_t::member_ids` map or the Raft configuration. The
goals are as follows:
- A server ought to be in `member_ids` and the Raft configuration if it appears in the
    current `table_config_t` or in a contract.
- If it is a voter in any contract, it should be a voter in the Raft configuration;
    otherwise, it should be a non-voting member.

There is constraint on how we approach those goals: Every server in the Raft
configuration must also be in `member_ids`. So we add new servers to `member_ids` before
adding them to the Raft configuration, and we remove obsolete servers from the Raft
configuration before removing them from `member_ids`. Note that `calculate_member_ids()`
assumes that if it returns changes for both `member_ids` and the Raft configuration, the
`member_ids` changes will be applied first. */
void calculate_member_ids_and_raft_config(
        const raft_member_t<table_raft_state_t>::state_and_config_t &sc,
        std::set<server_id_t> *remove_member_ids_out,
        std::map<server_id_t, raft_member_id_t> *add_member_ids_out,
        raft_config_t *new_config_out) {
    /* Assemble a set of all of the servers that ought to be in `member_ids`. */
    std::set<server_id_t> members_goal;
    for (const table_config_t::shard_t &shard : sc.state.config.config.shards) {
        members_goal.insert(shard.replicas.begin(), shard.replicas.end());
    }
    for (const auto &pair : sc.state.contracts) {
        members_goal.insert(
            pair.second.second.replicas.begin(), pair.second.second.replicas.end());
    }
    /* Assemble a set of all of the servers that ought to be Raft voters. */
    std::set<server_id_t> voters_goal;
    for (const auto &pair : sc.state.contracts) {
        voters_goal.insert(
            pair.second.second.voters.begin(),
            pair.second.second.voters.end());
    }
    /* Create entries in `add_member_ids_out` for any servers in `goal` that don't
    already have entries in `member_ids` */
    for (const server_id_t &server : members_goal) {
        if (sc.state.member_ids.count(server) == 0) {
            add_member_ids_out->insert(std::make_pair(
                server, raft_member_id_t(generate_uuid())));
        }
    }
    /* For any servers in the current `member_ids` that aren't in `servers`, add an entry
    in `remove_member_ids_out`, unless they are still in the Raft configuration. (If
    they're still in the Raft configuration, we'll remove them soon.) */
    for (const auto &existing : sc.state.member_ids) {
        if (members_goal.count(existing.first) == 0 &&
                !sc.config.is_member(existing.second)) {
            remove_member_ids_out->insert(existing.first);
        }
    }
    /* Set up `new_config_out`. */
    for (const server_id_t &server : members_goal) {
        raft_member_id_t member_id = (sc.state.member_ids.count(server) == 1)
            ? sc.state.member_ids.at(server)
            : add_member_ids_out->at(server);
        if (voters_goal.count(server) == 1) {
            new_config_out->voting_members.insert(member_id);
        } else {
            new_config_out->non_voting_members.insert(member_id);
        }
    }
}

contract_coordinator_t::contract_coordinator_t(
        raft_member_t<table_raft_state_t> *_raft,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *_acks,
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *_connections_map,
        const std::string &_log_prefix) :
    raft(_raft),
    acks(_acks),
    connections_map(_connections_map),
    log_prefix(_log_prefix),
    config_pumper(std::bind(&contract_coordinator_t::pump_configs, this, ph::_1)),
    contract_pumper(std::bind(&contract_coordinator_t::pump_contracts, this, ph::_1)),
    ack_subs(acks, std::bind(&pump_coro_t::notify, &contract_pumper), false),
    connections_map_subs(connections_map,
        std::bind(&pump_coro_t::notify, &contract_pumper), false)
{
    raft->assert_thread();
    /* Do an initial round of pumping, in case there are any changes the previous
    coordinator didn't take care of */
    contract_pumper.notify();
    config_pumper.notify();
}
        

boost::optional<raft_log_index_t> contract_coordinator_t::change_config(
        const std::function<void(table_config_and_shards_t *)> &changer,
        signal_t *interruptor) {
    assert_thread();
    if (!log_prefix.empty()) {
        logINF("%s: Changing table's configuration.", log_prefix.c_str());
    }
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
    contract_pumper.notify();
    config_pumper.notify();
    wait_interruptible(change_token->get_ready_signal(), interruptor);
    if (!change_token->wait()) {
        return boost::none;
    }
    return boost::make_optional(log_index);
}

void contract_coordinator_t::pump_contracts(signal_t *interruptor) {
    assert_thread();

    /* Wait a little while to give changes time to accumulate, because
    `calculate_all_contracts()` needs to examine every shard of the table even if
    nothing about them has changed, and that might be expensive. */
    nap(200, interruptor);

    /* Now we'll apply changes to Raft. We keep trying in a loop in case it
    doesn't work at first. */
    while (true) {
        /* Wait until the Raft member is likely to accept changes */
        raft->get_readiness_for_change()->run_until_satisfied(
            [](bool is_ready) { return is_ready; }, interruptor);

        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);

        /* Call `include_latest_notifications()` right before we read the Raft state.
        This is because any changes to the Raft state that happened up to this point will
        be included in this round of pumping, but any changes after this point must be
        run in their own round. */
        contract_pumper.include_latest_notifications();

        /* Calculate the proposed change */
        table_raft_state_t::change_t::new_contracts_t change;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
            calculate_all_contracts(
                state->state, acks, connections_map, log_prefix,
                &change.remove_contracts, &change.add_contracts);
            calculate_branch_history(
                state->state, acks,
                change.remove_contracts, change.add_contracts,
                &change.remove_branches, &change.add_branches);
            calculate_server_names(
                state->state, change.remove_contracts, change.add_contracts,
                &change.remove_server_names, &change.add_server_names);
        });

        /* Apply the change, unless it's a no-op */
        bool change_ok;
        if (!change.remove_contracts.empty() ||
                !change.add_contracts.empty() ||
                !change.remove_branches.empty() ||
                !change.add_branches.branches.empty()) {
            cond_t non_interruptor;
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_change(
                    &change_lock, table_raft_state_t::change_t(change),
                    &non_interruptor);

            /* `pump_configs()` sometimes makes changes in reaction to changes in
            contracts, so wake it up. */
            config_pumper.notify();

            change_ok = change_token.has();

            /* Note that we don't wait on the change token. We know we won't
            start a redundant change because we always compute the change by
            comparing to `get_latest_state()`. */
        } else {
            change_ok = true;
        }

        /* If the change failed, go back to the top of the loop and wait on
        `get_readiness_for_change()` again. */
        if (change_ok) {
            break;
        }
    }
}

void contract_coordinator_t::pump_configs(signal_t *interruptor) {
    assert_thread();

    nap(200, interruptor);

    while (true) {
        /* Wait until the Raft member is likely to accept config changes. This
        isn't actually necessary for changes to `member_ids`, but it's easier to
        just handle `member_ids` changes and Raft configuration changes at the
        same time. */
        raft->get_readiness_for_config_change()->run_until_satisfied(
            [](bool is_ready) { return is_ready; },
            interruptor);

        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);

        /* As in `pump_contracts()`, call `include_latest_notifications()` right before
        we read the Raft state */
        config_pumper.include_latest_notifications();

        /* Calculate changes to `table_raft_state_t::member_ids` */
        table_raft_state_t::change_t::new_member_ids_t member_ids_change;
        boost::optional<raft_config_t> config_change;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
            raft_config_t new_config;
            calculate_member_ids_and_raft_config(
                *sc,
                &member_ids_change.remove_member_ids,
                &member_ids_change.add_member_ids,
                &new_config);
            /* The config almost always won't be a joint consensus, because we
            waited until the `raft_member_t` was ready for config changes. But
            it's still possible so we have to handle that case somehow. Setting
            `config_change` won't change the config in a joint consensus because
            `propose_config_change()` will fail, but at least it will force us to
            go around the loop again. */
            if ((!sc->config.is_joint_consensus() &&
                        new_config != sc->config.config) ||
                    (sc->config.is_joint_consensus() &&
                        new_config != *sc->config.new_config)) {
                config_change = boost::make_optional(new_config);
            }
        });

        bool member_ids_ok;
        if (!member_ids_change.remove_member_ids.empty() ||
                !member_ids_change.add_member_ids.empty()) {
            /* Apply the `member_ids` change */
            cond_t non_interruptor;
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_change(
                    &change_lock,
                    table_raft_state_t::change_t(member_ids_change),
                    &non_interruptor);
            member_ids_ok = change_token.has();
        } else {
            /* The change is a no-op, so don't bother applying it */
            member_ids_ok = true;
        }

        bool config_ok;
        if (!member_ids_ok) {
            /* If the `member_ids` change didn't go through, don't attempt the
            config change. The config change would be unlikely to succeed, but
            more importantly, the config change isn't necessarily valid to apply
            unless it comes after the `member_ids` change. */
            config_ok = false;
        } else if (static_cast<bool>(config_change)) {
            /* Apply the config change */
            cond_t non_interruptor;
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_config_change(
                    &change_lock, *config_change, &non_interruptor);
            config_ok = change_token.has();
        } else {
            /* The config change is a no-op */
            config_ok = true;
        }

        /* If both changes succeeded, break out of the loop. Otherwise, go back
        to the top of the loop and wait for `get_readiness_for_config_change()`
        again. */
        if (member_ids_ok && config_ok) {
            break;
        }
    }
}

