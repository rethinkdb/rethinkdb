// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_SECONDARY_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_SECONDARY_HPP_

#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/exec.hpp"
#include "store_view.hpp"

class backfill_throttler_t;
class io_backender_t;

class secondary_execution_t : public execution_t {
public:
    secondary_execution_t(
        const execution_t::context_t *_context,
        const region_t &_region,
        store_view_t *_store,
        perfmon_collection_t *_perfmon_collection,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);
    void update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);

private:
    /* `run()` does the actual work of setting up the `listener_t`, etc. */
    void run(auto_drainer_t::lock_t);
    void send_ack(const contract_ack_t &ca);

    server_id_t const primary;
    branch_id_t const branch;

    /* `ack_cb` contains the last callback we got from either the constructor or from
    `update_contract()`. `last_ack` contains the last ack we've sent via `ack_cb`, if
    any. */
    std::function<void(const contract_ack_t &)> ack_cb;
    boost::optional<contract_ack_t> last_ack;

    /* `drainer` ensures that `run` is stopped before the other member variables are
    destroyed. */
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXEC_SECONDARY_HPP_ */

