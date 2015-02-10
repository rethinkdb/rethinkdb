// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_LEADER_HPP_
#define CLUSTERING_TABLE_RAFT_LEADER_HPP_

namespace table_raft {

class leader_t {
public:
    leader_t(
        raft_member_t<state_t> *raft,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *acks);

private:
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_LEADER_HPP_ */

