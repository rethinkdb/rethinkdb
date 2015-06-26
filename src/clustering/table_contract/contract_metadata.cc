// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/contract_metadata.hpp"

#include "boost_utils.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "stl_utils.hpp"

#ifndef NDEBUG
void contract_t::sanity_check() const {
    if (static_cast<bool>(primary)) {
        guarantee(replicas.count(primary->server) == 1);
        if (static_cast<bool>(primary->hand_over) && !primary->hand_over->is_nil()) {
            guarantee(replicas.count(*primary->hand_over) == 1);
        }
    }
    for (const server_id_t &s : voters) {
        guarantee(replicas.count(s) == 1);
    }
}
#endif /* NDEBUG */

RDB_IMPL_EQUALITY_COMPARABLE_2(
    contract_t::primary_t, server, hand_over);
RDB_IMPL_EQUALITY_COMPARABLE_5(
    contract_t, replicas, voters, temp_voters, primary, after_emergency_repair);

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    contract_t::primary_t, server, hand_over);
RDB_IMPL_SERIALIZABLE_5_SINCE_v2_1(
    contract_t, replicas, voters, temp_voters, primary, after_emergency_repair);

#ifndef NDEBUG
void contract_ack_t::sanity_check(
        const server_id_t &server,
        const contract_id_t &contract_id,
        const table_raft_state_t &raft_state) const {
    const region_t &region = raft_state.contracts.at(contract_id).first;
    const contract_t &contract = raft_state.contracts.at(contract_id).second;

    guarantee(contract.replicas.count(server) == 1,
        "A server sent an ack for a contract that it wasn't a replica for.");

    bool ack_says_primary = state == state_t::primary_need_branch ||
        state == state_t::primary_in_progress || state == state_t::primary_ready;
    bool contract_says_primary = static_cast<bool>(contract.primary) &&
        contract.primary->server == server;
    guarantee(ack_says_primary == contract_says_primary,
        "The contract says a server should be primary, but it sent a non-primary ack.");

    guarantee((state == state_t::primary_need_branch) == static_cast<bool>(branch),
        "branch_id should be present iff state is primary_need_branch");
    guarantee((state == state_t::secondary_need_primary) == static_cast<bool>(version),
        "version should be present iff state is secondary_need_primary");
    guarantee(!static_cast<bool>(version) || version->get_domain() == region,
        "version has wrong region");

    bool is_voter = contract.voters.count(server) == 1 ||
        (static_cast<bool>(contract.temp_voters) &&
            contract.temp_voters->count(server) == 1);
    if (!contract.after_emergency_repair && is_voter) {
        try {
            if (state == state_t::primary_need_branch) {
                branch_history_combiner_t combiner(
                    &raft_state.branch_history, &branch_history);
                version_t branch_initial_version(
                    *branch,
                    branch_history.get_branch(*branch).initial_timestamp);
                raft_state.current_branches.visit(region,
                [&](const region_t &subregion, const branch_id_t &cur_branch) {
                    version_find_branch_common(
                        &combiner, branch_initial_version, cur_branch, subregion);
                });
            } else if (state == state_t::secondary_need_primary) {
                branch_history_combiner_t combiner(
                    &raft_state.branch_history, &branch_history);
                version->visit(region,
                [&](const region_t &subregion, const version_t &subversion) {
                    raft_state.current_branches.visit(subregion,
                    [&](const region_t &subsubregion, const branch_id_t &cur_branch) {
                        version_find_branch_common(
                            &combiner, subversion, cur_branch, subsubregion);
                    });
                });
            }
        } catch (const missing_branch_exc_t &) {
            crash("Branch history is missing pieces");
        }
    }
}
#endif /* NDEBUG */

RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
    contract_ack_t, state, version, branch, branch_history);
RDB_IMPL_EQUALITY_COMPARABLE_4(
    contract_ack_t, state, version, branch, branch_history);

void table_raft_state_t::apply_change(const table_raft_state_t::change_t &change) {
    class visitor_t : public boost::static_visitor<void> {
    public:
        void operator()(const change_t::set_table_config_t &set_config_change) {
            state->config = set_config_change.new_config;
        }
        void operator()(const change_t::new_contracts_t &new_contracts_change) {
            for (const contract_id_t &cid : new_contracts_change.remove_contracts) {
                state->contracts.erase(cid);
            }
            state->contracts.insert(
                new_contracts_change.add_contracts.begin(),
                new_contracts_change.add_contracts.end());
            for (const branch_id_t &bid : new_contracts_change.remove_branches) {
                state->branch_history.branches.erase(bid);
            }
            state->branch_history.branches.insert(
                new_contracts_change.add_branches.branches.begin(),
                new_contracts_change.add_branches.branches.end());
            for (const auto &region_branch :
                new_contracts_change.register_current_branches) {
                state->current_branches.update(region_branch.first, region_branch.second);
            }
            for (const server_id_t &sid : new_contracts_change.remove_server_names) {
                state->server_names.names.erase(sid);
            }
            state->server_names.names.insert(
                new_contracts_change.add_server_names.names.begin(),
                new_contracts_change.add_server_names.names.end());
        }
        void operator()(const change_t::new_member_ids_t &new_member_ids_change) {
            for (const server_id_t &sid : new_member_ids_change.remove_member_ids) {
                state->member_ids.erase(sid);
            }
            state->member_ids.insert(
                new_member_ids_change.add_member_ids.begin(),
                new_member_ids_change.add_member_ids.end());
        }
        void operator()(const change_t::new_server_names_t &new_server_names) {
            for (const auto &pair : new_server_names.config_and_shards.names) {
                auto name_it = state->config.server_names.names.find(pair.first);
                guarantee(name_it != state->config.server_names.names.end());
                name_it->second = pair.second;
            }
            for (const auto &pair : new_server_names.raft_state.names) {
                auto name_it = state->server_names.names.find(pair.first);
                guarantee(name_it != state->server_names.names.end());
                name_it->second = pair.second;
            }
        }
        table_raft_state_t *state;
    } visitor;
    visitor.state = this;
    boost::apply_visitor(visitor, change.v);
    DEBUG_ONLY_CODE(sanity_check());
}

#ifndef NDEBUG
void table_raft_state_t::sanity_check() const {
    std::set<server_id_t> all_replicas;
    for (const auto &pair : contracts) {
        pair.second.second.sanity_check();
        for (const server_id_t &replica : pair.second.second.replicas) {
            all_replicas.insert(replica);
            guarantee(server_names.names.count(replica) == 1);
        }
    }
    for (const auto &pair : server_names.names) {
        guarantee(all_replicas.count(pair.first) == 1);
    }
}
#endif /* NDEBUG */

RDB_IMPL_EQUALITY_COMPARABLE_6(
    table_raft_state_t, config, contracts, branch_history, current_branches,
    member_ids, server_names);

RDB_IMPL_SERIALIZABLE_1_SINCE_v2_1(
    table_raft_state_t::change_t::set_table_config_t, new_config);
RDB_IMPL_SERIALIZABLE_7_SINCE_v2_1(
    table_raft_state_t::change_t::new_contracts_t,
    remove_contracts, add_contracts, remove_branches, add_branches,
    register_current_branches, remove_server_names, add_server_names);
RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    table_raft_state_t::change_t::new_member_ids_t,
    remove_member_ids, add_member_ids);
RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    table_raft_state_t::change_t::new_server_names_t, raft_state, config_and_shards);
RDB_IMPL_SERIALIZABLE_1_SINCE_v2_1(
    table_raft_state_t::change_t, v);
RDB_IMPL_SERIALIZABLE_6_SINCE_v2_1(
    table_raft_state_t, config, contracts, branch_history, current_branches,
    member_ids, server_names);

table_raft_state_t make_new_table_raft_state(
        const table_config_and_shards_t &config) {
    table_raft_state_t state;
    state.config = config;
    for (size_t i = 0; i < config.shard_scheme.num_shards(); ++i) {
        const table_config_t::shard_t &shard_conf = config.config.shards[i];
        contract_t contract;
        contract.replicas = shard_conf.all_replicas;
        contract.voters = shard_conf.voting_replicas();
        if (!shard_conf.primary_replica.is_nil()) {
            contract.primary = boost::make_optional(
                contract_t::primary_t { shard_conf.primary_replica, boost::none });
        }
        contract.after_emergency_repair = false;
        for (size_t j = 0; j < CPU_SHARDING_FACTOR; ++j) {
            region_t region = region_intersection(
                region_t(config.shard_scheme.get_shard_range(i)),
                cpu_sharding_subspace(j));
            state.contracts.insert(std::make_pair(generate_uuid(),
                std::make_pair(region, contract)));
        }
        for (const server_id_t &server_id : shard_conf.all_replicas) {
            if (state.member_ids.count(server_id) == 0) {
                state.member_ids[server_id] = raft_member_id_t(generate_uuid());
            }
        }
    }
    state.server_names = config.server_names;
    DEBUG_ONLY_CODE(state.sanity_check());
    return state;
}

RDB_IMPL_EQUALITY_COMPARABLE_6(table_shard_status_t,
    primary, secondary, need_primary, need_quorum, backfilling, transitioning);
RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(table_shard_status_t,
    primary, secondary, need_primary, need_quorum, backfilling, transitioning);

void debug_print(printf_buffer_t *buf, const contract_t::primary_t &primary) {
    buf->appendf("primary_t { server = ");
    debug_print(buf, primary.server);
    buf->appendf(" hand_over = ");
    debug_print(buf, primary.hand_over);
    buf->appendf(" }");
}

void debug_print(printf_buffer_t *buf, const contract_t &contract) {
    buf->appendf("contract_t { replicas = ");
    debug_print(buf, contract.replicas);
    buf->appendf(" voters = ");
    debug_print(buf, contract.voters);
    buf->appendf(" temp_voters = ");
    debug_print(buf, contract.temp_voters);
    buf->appendf(" primary = ");
    debug_print(buf, contract.primary);
    buf->appendf(" }");
}

const char *contract_ack_state_to_string(contract_ack_t::state_t state) {
    typedef contract_ack_t::state_t state_t;
    switch (state) {
        case state_t::primary_need_branch: return "primary_need_branch";
        case state_t::primary_in_progress: return "primary_in_progress";
        case state_t::primary_ready: return "primary_ready";
        case state_t::secondary_need_primary: return "secondary_need_primary";
        case state_t::secondary_backfilling: return "secondary_backfilling";
        case state_t::secondary_streaming: return "secondary_streaming";
        default: unreachable();
    }
}

void debug_print(printf_buffer_t *buf, const contract_ack_t &ack) {
    buf->appendf("contract_ack_t { state = %s",
        contract_ack_state_to_string(ack.state));
    buf->appendf(" version = ");
    debug_print(buf, ack.version);
    buf->appendf(" branch = ");
    debug_print(buf, ack.branch);
    buf->appendf(" branch_history.size = ");
    debug_print(buf, ack.branch_history.branches.size());
    buf->appendf(" }");
}

void debug_print(printf_buffer_t *buf, const table_raft_state_t &state) {
    buf->appendf("table_raft_state_t { config = ...");
    buf->appendf(" contracts = ");
    debug_print(buf, state.contracts);
    buf->appendf(" branch_history.size = ");
    debug_print(buf, state.branch_history.branches.size());
    buf->appendf(" current_branches = ");
    debug_print(buf, state.current_branches);
    buf->appendf(" member_ids = ");
    debug_print(buf, state.member_ids);
    buf->appendf(" server_names = ");
    debug_print(buf, state.server_names.names);
    buf->appendf(" }");
}

