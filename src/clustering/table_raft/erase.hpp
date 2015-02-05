// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_ERASE_HPP_
#define CLUSTERING_TABLE_RAFT_ERASE_HPP_

namespace table_raft {

class erase_t {
public:
    erase_t(
        store_view_t *s,
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
    ~erase_t();   /* may block */
    void update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);

private:
    void do_erase(auto_drainer_t::lock_t);

    store_view_t *store;
    region_t r;
    auto_drainer_t drainer;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_ERASE_HPP_ */

