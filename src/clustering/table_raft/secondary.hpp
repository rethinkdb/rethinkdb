// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_SECONDARY_HPP_
#define CLUSTERING_TABLE_RAFT_SECONDARY_HPP_

namespace table_raft {

class secondary_t {
public:
    secondary_t(
        store_view_t *s,
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
    ~secondary_t();   /* may block */
    void update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_SECONDARY_HPP_ */

