// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EMERGENCY_REPAIR_HPP_
#define CLUSTERING_TABLE_CONTRACT_EMERGENCY_REPAIR_HPP_

/* `calculate_emergency_repair()` is the brains behind
`table_meta_client_t::emergency_repair()`. It takes as input the table's current state
and a set of servers that are being abandoned. If it finds anything that can be fixed
without data loss, it fixes it and sets `*quorum_loss_found_out = true`. If it finds
anything that can only be fixed via data loss, it sets `*data_loss_found_out = true`.
`new_state_out` will be the state after fixing any simple errors, and also fixing data
loss errors but only if `allow_data_loss` is `true`. */
void calculate_emergency_repair(
    const table_raft_state_t &old_state,
    const std::set<server_id_t> &dead_servers,
    bool allow_data_loss,
    table_raft_state_t *new_state_out,
    bool *quorum_loss_found_out,
    bool *data_loss_found_out);

#endif /* CLUSTERING_TABLE_CONTRACT_EMERGENCY_REPAIR_HPP_ */

