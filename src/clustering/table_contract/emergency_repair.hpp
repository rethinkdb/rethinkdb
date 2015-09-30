// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EMERGENCY_REPAIR_HPP_
#define CLUSTERING_TABLE_CONTRACT_EMERGENCY_REPAIR_HPP_

#include "clustering/table_contract/contract_metadata.hpp"

/* `calculate_emergency_repair()` is the brains behind
`table_meta_client_t::emergency_repair()`. It takes as input the table's current state
and a set of servers that are being abandoned. If it finds anything that can be fixed via
a rollback, it fixes it and sets `*rollback_found_out = true`. If it finds anything that
can only be fixed via an erase, it sets `*erase_found_out = true`, but doesn't actually
fix it unless `allow_erase` is `true`. */
void calculate_emergency_repair(
    const table_raft_state_t &old_state,
    const std::set<server_id_t> &dead_servers,
    emergency_repair_mode_t mode,
    table_raft_state_t *new_state_out,
    bool *rollback_found_out,
    bool *erase_found_out);

#endif /* CLUSTERING_TABLE_CONTRACT_EMERGENCY_REPAIR_HPP_ */

