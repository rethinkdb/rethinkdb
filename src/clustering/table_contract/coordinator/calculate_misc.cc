// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/coordinator/calculate_misc.hpp"

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
        const std::map<region_t, branch_id_t> &register_current_branches,
        std::set<branch_id_t> *remove_branches_out,
        branch_history_t *add_branches_out) {
    /* RSI(raft): This is a totally naive implementation that never prunes branches and
    sometimes adds unnecessary branches. */
    (void)old_state;
    (void)remove_contracts;
    (void)add_contracts;
    (void)register_current_branches;
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
        server_name_map_t *add_server_names_out) {
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
            if (old_state.server_names.names.count(server) == 0) {
                add_server_names_out->names.insert(std::make_pair(server,
                    old_state.config.server_names.names.at(server)));
            }
        }
    }
    for (const auto &pair : old_state.server_names.names) {
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
        members_goal.insert(shard.all_replicas.begin(), shard.all_replicas.end());
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

