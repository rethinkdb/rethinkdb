// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXECUTOR_EXEC_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXECUTOR_EXEC_HPP_

#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/query_routing/metadata.hpp"
#include "store_view.hpp"

class backfill_progress_tracker_t;
class backfill_throttler_t;
class io_backender_t;

/* `contract_execution_bcard_t`s are passed around between the `contract_executor_t`s for
the same table on different servers. They allow servers to request backfills from one
another and subscribe to receive queries. */
class contract_execution_bcard_t {
public:
    remote_replicator_server_bcard_t remote_replicator_server;
    replica_bcard_t replica;
    peer_id_t peer;
};

RDB_DECLARE_SERIALIZABLE(contract_execution_bcard_t);

/* `execution_t` is a base class for `primary_execution_t`, `secondary_execution_t`, and
`erase_execution_t`. */
class execution_t {
public:
    /* There is one `context_t` for each `contract_executor_t`; it holds the global
    and per-table objects that the `execution_t`s will need. */
    class context_t {
    public:
        server_id_t server_id;
        mailbox_manager_t *mailbox_manager;
        branch_history_manager_t *branch_history_manager;
        base_path_t base_path;
        io_backender_t *io_backender;
        backfill_progress_tracker_t *backfill_progress_tracker;
        backfill_throttler_t *backfill_throttler;
        watchable_map_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t> *remote_contract_execution_bcards;
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t> *local_contract_execution_bcards;
        watchable_map_var_t<uuid_u, table_query_bcard_t> *local_table_query_bcards;
    };

    /* There is one `params` for each `execution_t`; it holds information that's specific
    to the particular execution, and also provides callbacks for the execution to send
    events back. */
    class params_t {
    public:
        virtual perfmon_collection_t *get_perfmon_collection() = 0;
        virtual store_view_t *get_store() = 0;

        /* Sends the given contract ack for the given contract ID. The contract ID should
        be an ID that has been passed to the constructor or
        `update_contract_or_raft_state()`. */
        virtual void send_ack(const contract_id_t &, const contract_ack_t &) = 0;

        /* Once the execution has finished its backfill/erase/whatever, so that the
        metainfo is on a particular branch and will stay there indefinitely, it calls
        `enable_gc()` with that branch. `enable_gc()` must not be called more than once.
        */
        virtual void enable_gc(const branch_id_t &) = 0;

    protected:
        virtual ~params_t() { }
    };

    execution_t(const context_t *_context, params_t *_params) :
        context(_context), params(_params), store(params->get_store()),
        region(store->get_region()) { }

    /* Note: Subclass destructors may block. */
    virtual ~execution_t() { }

    virtual void update_contract_or_raft_state(
        const contract_id_t &cid,
        const table_raft_state_t &raft_state) = 0;

protected:
    context_t const *const context;
    params_t *const params;
    store_view_t *const store;
    region_t const region;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXECUTOR_EXEC_HPP_ */

