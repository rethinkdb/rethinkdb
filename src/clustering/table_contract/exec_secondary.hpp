// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_SECONDARY_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_SECONDARY_HPP_

#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/exec.hpp"
#include "store_view.hpp"

class backfill_throttler_t;
class io_backender_t;

class secondary_execution_t : public execution_t, public home_thread_mixin_t {
public:
    secondary_execution_t(
        const execution_t::context_t *_context,
        store_view_t *_store,
        perfmon_collection_t *_perfmon_collection,
        const std::function<void(
            const contract_id_t &, const contract_ack_t &)> &ack_cb,
        const contract_id_t &cid,
        const table_raft_state_t &raft_state);
    void update_contract(
        const contract_id_t &cid,
        const table_raft_state_t &raft_state);

private:
    /* `run()` does the actual work of setting up the `listener_t`, etc. */
    void run(auto_drainer_t::lock_t);
    void send_ack(const contract_ack_t &ca);

    /* If we are secondary for a contract for which there is no primary, or if the
    contract's region doesn't match the branch's region, we'll set `connect_to_primary`
    to `false`. In this case, we'll serve outdated reads but won't attempt to find a
    primary in the directory. */
    bool connect_to_primary;

    server_id_t primary;
    branch_id_t branch;

    contract_id_t contract_id;

    /* `last_ack` contains the last ack we've sent via `ack_cb`, if any. */
    boost::optional<contract_ack_t> last_ack;

    /* `drainer` ensures that `run` is stopped before the other member variables are
    destroyed. */
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXEC_SECONDARY_HPP_ */

