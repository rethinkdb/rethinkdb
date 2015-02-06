// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_SECONDARY_HPP_
#define CLUSTERING_TABLE_RAFT_SECONDARY_HPP_

namespace table_raft {

class secondary_t {
public:
    secondary_t(
        const server_id_t &sid,
        store_view_t *s,
        watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *
            primary_bcards,
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);
    void update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb);

private:
    void run(auto_drainer_t::lock_t);
    void send_ack(const contract_ack_t &ca);

    server_id_t const server_id;
    store_view_t *const store;
    watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *const
        primary_bcards;
    region_t const region;
    server_id_t const primary;
    branch_id_t const branch;

    std::function<void(const contract_ack_t &)> ack_cb;
    boost::optional<contract_ack_t> last_ack;

    auto_drainer_t drainer;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_SECONDARY_HPP_ */

