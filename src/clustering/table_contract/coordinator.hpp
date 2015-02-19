// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_COORDINATOR_HPP_
#define CLUSTERING_TABLE_CONTRACT_COORDINATOR_HPP_

#include "clustering/generic/raft_core.hpp"
#include "clustering/table_contract/contract_metadata.hpp"

class contract_coordinator_t {
public:
    contract_coordinator_t(
        raft_member_t<table_raft_state_t> *raft,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks);

    boost::optional<raft_log_index_t> change_config(
        const std::function<void(table_config_and_shards_t *)> &changer,
        signal_t *interruptor);

private:
    void pump_contracts(auto_drainer_t::lock_t keepalive);

    raft_member_t<table_raft_state_t> *const raft;
    watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *const acks;

    scoped_ptr_t<cond_t> wake_pump_contracts;

    auto_drainer_t drainer;

    watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>::all_subs_t
        ack_subs;
};

#endif /* CLUSTERING_TABLE_CONTRACT_COORDINATOR_HPP_ */

