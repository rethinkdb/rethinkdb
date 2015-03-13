// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_HPP_

#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/query_routing/metadata.hpp"
#include "store_view.hpp"

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
        backfill_throttler_t *backfill_throttler;
        watchable_map_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t> *remote_contract_execution_bcards;
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t> *local_contract_execution_bcards;
        watchable_map_var_t<uuid_u, table_query_bcard_t> *local_table_query_bcards;
    };

    /* All subclasses of `execution_t` have the following things in common:
    - A constructor that takes the same parameters as `execution_t` plus a `contract_t`
        and a `std::function<void(const contract_ack_t &)>`. The constructor cannot throw
        exceptions or block.
    - A destructor which may block
    - A method `update_contract` that takes a new `contract_t` and callback.
    */ 
    execution_t(
            const context_t *_context,
            const region_t &_region,
            store_view_t *_store,
            perfmon_collection_t *_perfmon_collection) :
        context(_context), region(_region), store(_store),
        perfmon_collection(_perfmon_collection)
    {
        guarantee(store->get_region() == region);
    }
    virtual ~execution_t() { }
    virtual void update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb) = 0;

protected:
    context_t const *const context;
    region_t const region;
    store_view_t *const store;
    perfmon_collection_t *const perfmon_collection;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXEC_HPP_ */

