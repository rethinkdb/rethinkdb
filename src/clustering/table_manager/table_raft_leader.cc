// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_raft_leader.hpp"

namespace table_raft {

/* `calculate_contract()` calculates a new contract for a region. Whenever any of the
inputs changes, the leader will call `update_contract()` to compute a contract for each
range of keys. The new contract will often be the same as the old, in which case it
doesn't get a new contract ID. */
contract_t calculate_contract(
        /* The region that we're computing a contract for. This region will never contain
        any split points. */
        const region_t &region,
        /* The old contract that contains this region. */
        const contract_t &old_c,
        /* The user-specified configuration for the shard containing this region. */
        const table_config_t::shard_t &config,
        /* Contract acks from replicas regarding `old_c`. If a replica hasn't sent us an
        ack *specifically* for `old_c`, it won't appear in this map. */
        const std::map<server_id_t, contract_ack_t::state_t> &ack_states,
        const std::map<server_id_t, version_t> &ack_versions,
        /* Creates a new branch, starting from the given version, to be hosted on the
        given server. `update_contract()` should call this at most once. If several calls
        to `update_contract()` in the same batch all call `branch_maker()` for contiguous
        regions, it will combine all the calls into the same branch. */
        const std::function<branch_id_t(server_id_t, version_t)> &branch_maker) {

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
            auto it = ack_states.find(server);
            if (it != ack_states.end() &&
                    (it->second == contract_ack_t::secondary_streaming ||
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
                ack_states.count(old_c.primary->server) == 1 &&
                ack_states.at(old_c.primary->server) == contract_ack_t::primary_ready) {
            /* OK, it's safe to commit. */
            new_c.voters = *new_c.temp_voters;
            new_c.temp_voters = boost::none;
        }
    }

    /* If a server was removed from `config.replicas` and `c.voters` but it's still in
    `c.replicas`, and it's not primary, then remove it. (If it is primary, it won't be
    for long, because we'll detect this case and switch to another primary.) */
    bool should_warm_shutdown_primary = false;
    for (const server_id_t &server : old_c.replicas) {
        if (config.replicas.count(server) == 0 &&
                old_c.voters.count(server) == 0 &&
                (!static_cast<bool>(old_c.temp_voters) ||
                    old_c.temp_voters->count(server) == 0)) {
            if (static_cast<bool>(old_c.primary) && old_c.primary->server == server) {
                /* We'll process this case further down */
                should_warm_shutdown_primary = true;
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
            if (ack_states.count(server) == 1 && ack_states.at(server) ==
                    contract_ack_t::secondary_need_primary) {
                state_timestamp_t timestamp = version_project_to_branch(
                    branch_history, ack_versions.at(server), old_c.branch, region);
                replica_states.insert(std::make_pair(timestamp, server));
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
            p.warm_shutdown = false;
            p.warm_shutdown_for = nil_uuid();
            new_c.primary = boost::make_optional(p);
            new_c.branch = branch_maker(*new_primary, ack_versions.at(*new_primary));
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
        /* Check if we need to do an auto-failover. The precise form of this condition
        isn't important for correctness. If we do an auto-failover when the primary isn't
        actually dead, or don't do an auto-failover when the primary is actually dead,
        the worst that will happen is we'll lose availability. */
        size_t voters_cant_reach_primary = 0;
        for (const server_id_t &server : new_c.voters) {
            if (ack_states.count(server) == 1 && ack_states.at(server) ==
                    contract_ack::secondary_need_primary) {
                ++voters_cant_reach_primary;
            }
        }
        bool should_kill_primary = voters_cant_reach_primary > new_c.voters.size() / 2;

        /* If the current primary isn't supposed to be a replica anymore, we'll already
        have set `should_warm_shutdown_primary` to `true` earlier. Now we also check for
        the case where it's supposed to be a replica but another replica is supposed to
        be primary. */
        server_id_t warm_shutdown_for = nil_uuid();
        if (old_c.primary->server != config.primary_replica &&
                ack_states.count(config.primary_replica) == 1 &&
                ack_states.at(config.primary_replica) ==
                    contract_ack_t::secondary_streaming) {
            should_warm_shutdown_primary = true;
            warm_shutdown_for = config.primary_replica;
        }

        if (should_kill_primary) {
            new_c.primary = boost::none;
        } else if (should_warm_shutdown_primary) {
            /* If the primary already finished the warm shutdown, for the desired server,
            then kill it. Otherwise, start the warm shutdown. */
            if (old_c.primary->warm_shutdown == true &&
                    old_c.primary->warm_shutdown_for = warm_shutdown_for &&
                    ack_states.count(old_c.primary->server) == 1 &&
                    ack_states.at(old_c.primary->server) ==
                        contract_ack_t::state_t::primary_ready) {
                new_c.primary = boost::none;
            } else {
                new_c.primary->warm_shutdown = true;
                new_c.primary->warm_shutdown_for = warm_shutdown_for;
            }
        } else {
            /* Usually this will be a no-op. However, it's possible that we'll start a
            warm shutdown but then the config will change before we finish, and this
            aborts the warm shutdown. */
            new_c.primary->warm_shutdown = false;
            new_c.primary->warm_shutdown_for = nil_uuid();

            /* We might need to issue the primary a new branch ID */
            if (ack_states.count(old_c.primary->server) == 1 &&
                    ack_states.at(old_c.primary->server) ==
                        contract_ack_t::primary_need_new_branch) {
                new_c.branch = branch_maker(
                    old_c.primary->server,
                    ack_versions.at(old_c.primary->server));
            }
        }
    }

    return new_c;
}

} /* namespace table_raft */

