// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_COORDINATOR_CALCULATE_CONTRACTS_HPP_
#define CLUSTERING_TABLE_CONTRACT_COORDINATOR_CALCULATE_CONTRACTS_HPP_

#include "clustering/table_contract/contract_metadata.hpp"
#include "concurrency/watchable_map.hpp"

void calculate_all_contracts(
        const table_raft_state_t &old_state,
        const std::map<contract_id_t, std::map<server_id_t, contract_ack_t> > &acks,
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *connections_map,
        const std::string &log_prefix,
        std::set<contract_id_t> *remove_contracts_out,
        std::map<contract_id_t, std::pair<region_t, contract_t> > *add_contracts_out,
        std::map<region_t, branch_id_t> *register_current_branches_out);

#endif /* CLUSTERING_TABLE_CONTRACT_COORDINATOR_CALCULATE_CONTRACTS_HPP_ */

