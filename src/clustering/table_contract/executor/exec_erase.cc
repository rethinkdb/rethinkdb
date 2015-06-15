// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/executor/exec_erase.hpp"

#include "concurrency/cross_thread_signal.hpp"

erase_execution_t::erase_execution_t(
        const execution_t::context_t *_context,
        store_view_t *_store,
        perfmon_collection_t *_perfmon_collection,
        const std::function<void(
            const contract_id_t &, const contract_ack_t &)> &_ack_cb,
        const contract_id_t &cid,
        const table_raft_state_t &raft_state) :
    execution_t(_context, _store, _perfmon_collection, _ack_cb)
{
    update_contract_or_raft_state(cid, raft_state);
    coro_t::spawn_sometime(std::bind(&erase_execution_t::run, this, drainer.lock()));
}

void erase_execution_t::update_contract_or_raft_state(
        DEBUG_VAR const contract_id_t &cid,
        DEBUG_VAR const table_raft_state_t &raft_state) {
    assert_thread();
    rassert(raft_state.contracts.at(cid).second.replicas.count(context->server_id) == 0);
    rassert(raft_state.contracts.at(cid).first == region);
}

void erase_execution_t::run(auto_drainer_t::lock_t keepalive) {
    assert_thread();
    cross_thread_signal_t interruptor_store_thread(
        keepalive.get_drain_signal(), store->home_thread());
    on_thread_t thread_switcher(store->home_thread());
    try {
        store->reset_data(
            binary_blob_t(version_t::zero()),
            region,
            write_durability_t::HARD,
            &interruptor_store_thread);
    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}


