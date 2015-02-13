// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_LEADER_HPP_
#define CLUSTERING_TABLE_RAFT_LEADER_HPP_

namespace table_raft {

class leader_t {
public:
    leader_t(
        raft_member_t<state_t> *raft,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks);

    boost::optional<raft_log_index_t> change_config(
        const std::function<void(table_config_and_shards_t *)> &changer,
        signal_t *interruptor);

private:
    void pump_contracts(auto_drainer_t::lock_t keepalive);

    raft_member_t<state_t> *const raft;
    watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *const acks;

    scoped_ptr_t<cond_t> wake_pump_contracts;

    auto_drainer_t drainer;

    watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>::all_subs_t
        ack_subs;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_LEADER_HPP_ */

