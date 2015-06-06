// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_ERASE_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_ERASE_HPP_

#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/exec.hpp"
#include "store_view.hpp"

class erase_execution_t : public execution_t, public home_thread_mixin_t {
public:
    erase_execution_t(
        const execution_t::context_t *context,
        store_view_t *store,
        perfmon_collection_t *perfmon_collection,
        const std::function<void(
            const contract_id_t &, const contract_ack_t &)> &ack_cb,
        const contract_id_t &cid,
        const table_raft_state_t &raft_state);
    void update_contract_or_raft_state(
        const contract_id_t &cid,
        const table_raft_state_t &raft_state);

private:
    /* `run()` does the actual work of erasing the data. */
    void run(auto_drainer_t::lock_t);

    /* `drainer` makes sure that `run()` stops before the `erase_execution_t` is
    destroyed. */
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXEC_ERASE_HPP_ */

