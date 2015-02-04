// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_RAFT_FOLLOWER_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_RAFT_FOLLOWER_HPP_

namespace table_raft {

class follower_t {
public:
private:
    class ongoing_primary_t {
    public:
        boost::optional<branch_id_t> branch;
        scoped_ptr_t<broadcaster_t> broadcaster;
        scoped_ptr_t<listener_t> listener;
        scoped_ptr_t<replier_t> replier;
    };

    class ongoing_secondary_t {
    public:
        boost::optional<branch_id_t> branch;
        scoped_ptr_t<listener_t> listener;
        scoped_ptr_t<replier_t> replier;
    };

    void execute_contract_primary(
        const region_t &region,
        const contract_id_t &cid,
        const contract_t &c,
        store_t *store,
        ongoing_primary_t *ongoing,
        signal_t *interruptor);

    void execute_contract_secondary(
        const region_t &region,
        const contract_id_t &cid,
        const contract_t &c,
        store_t *store,
        ongoing_secondary_t *ongoing,
        signal_t *interruptor);

    void execute_contract_nothing(
        const region_t &region,
        const contact_id_t &cid,
        const contract_t &c,
        store_t *store,
        signal_t *interruptor);
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_RAFT_FOLLOWER_HPP_ */

