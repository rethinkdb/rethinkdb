// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_HPP_

#include "clustering/immediate_consistency/metadata.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/query_routing/metadata.hpp"
#include "store_view.hpp"

class backfill_throttler_t;
class io_backender_t;

class contract_execution_bcard_t {
public:
    broadcaster_business_card_t broadcaster;
    replier_business_card_t replier;
    peer_id_t peer;
};

class execution_t {
public:
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

