// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/contract_metadata.hpp"

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
        }
        table_raft_state_t *state;
    } visitor;
    visitor.state = this;
    boost::apply_visitor(visitor, change.v);
}

/* RSI(raft): This should be `SINCE_v1_N`, where `N` is the version number at which Raft
is released */
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(
    contract_t::primary_t, server, hand_over);
RDB_IMPL_SERIALIZABLE_5_SINCE_v1_16(
    contract_t, replicas, voters, temp_voters, primary, branch);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(
    table_raft_state_t::change_t::set_table_config_t, new_config);
RDB_IMPL_SERIALIZABLE_4_SINCE_v1_16(
    table_raft_state_t::change_t::new_contracts_t,
    remove_contracts, add_contracts, remove_branches, add_branches);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(table_raft_state_t::change_t, v);
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(table_raft_state_t, config, member_ids);

