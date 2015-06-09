// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_COORDINATOR_CALCULATE_MISC_HPP_
#define CLUSTERING_TABLE_CONTRACT_COORDINATOR_CALCULATE_MISC_HPP_

#include "clustering/table_contract/contract_metadata.hpp"

void calculate_branch_history(
        const table_raft_state_t &old_state,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks,
        const std::set<contract_id_t> &remove_contracts,
        const std::map<contract_id_t, std::pair<region_t, contract_t> > &add_contracts,
        const std::map<region_t, branch_id_t> &register_current_branches,
        std::set<branch_id_t> *remove_branches_out,
        branch_history_t *add_branches_out);

void calculate_server_names(
        const table_raft_state_t &old_state,
        const std::set<contract_id_t> &remove_contracts,
        const std::map<contract_id_t, std::pair<region_t, contract_t> > &add_contracts,
        std::set<server_id_t> *remove_server_names_out,
        server_name_map_t *add_server_names_out);

void calculate_member_ids_and_raft_config(
        const raft_member_t<table_raft_state_t>::state_and_config_t &sc,
        std::set<server_id_t> *remove_member_ids_out,
        std::map<server_id_t, raft_member_id_t> *add_member_ids_out,
        raft_config_t *new_config_out);

#endif /* CLUSTERING_TABLE_CONTRACT_COORDINATOR_CALCULATE_MISC_HPP_ */

