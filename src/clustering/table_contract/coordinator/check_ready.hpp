// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_COORDINATOR_HPP_
#define CLUSTERING_TABLE_CONTRACT_COORDINATOR_HPP_

#include "clustering/table_contract/contract_metadata.hpp"

/* Returns `true` if the contracts are consistent with the config, and the acks indicate
that every server is executing its contracts and finished with all backfills, etc. */
bool check_all_replicas_ready(
    const table_raft_state_t &table_state,
    watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks);

#endif /* CLUSTERING_TABLE_CONTRACT_COORDINATOR_HPP_ */

