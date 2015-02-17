// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_ERASE_HPP_
#define CLUSTERING_TABLE_RAFT_ERASE_HPP_

namespace table_raft {

class erase_t {
public:
    erase_t(
        const server_id_t &server_id,
        store_view_t *s,
        UNUSED branch_history_manager_t *bhm,
        const region_t &r,
        UNUSED perfmon_collection_t *perfmons,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
    void update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);

private:
    void run(auto_drainer_t::lock_t);

    server_id_t const server_id;
    store_view_t *const store;
    region_t const r;
    auto_drainer_t drainer;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_ERASE_HPP_ */

