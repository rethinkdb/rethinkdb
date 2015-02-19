// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_ERASE_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_ERASE_HPP_

#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/exec.hpp"
#include "store_view.hpp"

class erase_execution_t : public execution_t {
public:
    erase_execution_t(
        const execution_t::context_t *context,
        const region_t &region,
        store_view_t *store,
        perfmon_collection_t *perfmon_collection,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);
    void update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);

private:
    void run(auto_drainer_t::lock_t);
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXEC_ERASE_HPP_ */

