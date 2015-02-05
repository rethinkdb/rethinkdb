// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_PRIMARY_HPP_
#define CLUSTERING_TABLE_RAFT_PRIMARY_HPP_

namespace table_raft {

class primary_t {
public:
    primary_t(
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
    ~primary_t();   /* may block */
    void update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_PRIMARY_HPP_ */

