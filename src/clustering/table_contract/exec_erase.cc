// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/exec_erase.hpp"

erase_execution_t::erase_execution_t(
        const execution_t::context_t *_context,
        const region_t &_region,
        store_view_t *_store,
        perfmon_collection_t *_perfmon_collection,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb) :
    execution_t(_context, _region, _store, _perfmon_collection)
{
    update_contract(c, ack_cb);
    coro_t::spawn_sometime(std::bind(&erase_execution_t::run, this, drainer.lock()));
}

void erase_execution_t::update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb) {
    guarantee(c.replicas.count(context->server_id) == 0);
    ack_cb(contract_ack_t(contract_ack_t::state_t::nothing));
}

void erase_execution_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        /* RSI(raft): This function needs to work even if `store->home_thread` is not
        equal to our home thread. */

        store->reset_data(
            binary_blob_t(version_t::zero()),
            region,
            write_durability_t::HARD,
            keepalive.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}


