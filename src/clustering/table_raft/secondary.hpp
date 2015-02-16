// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_SECONDARY_HPP_
#define CLUSTERING_TABLE_RAFT_SECONDARY_HPP_

namespace table_raft {

class secondary_t {
public:
    secondary_t(
        const server_id_t &sid,
        store_view_t *s,
        branch_history_manager_t *bhm,
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb,
        watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *
            primary_bcards);
    void update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);

private:
    void run(auto_drainer_t::lock_t);
    void send_ack(const contract_ack_t &ca);

    server_id_t const server_id;
    store_view_t *const store;
    branch_history_manager_t *const branch_history_manager;
    region_t const region;
    watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *const
        primary_bcards;
    server_id_t const primary;
    branch_id_t const branch;

    std::function<void(const contract_ack_t &)> ack_cb;
    boost::optional<contract_ack_t> last_ack;

    auto_drainer_t drainer;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_SECONDARY_HPP_ */

