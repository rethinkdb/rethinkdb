// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/erase.hpp"

erase_t::erase_t(
        const server_id_t &sid,
        store_view_t *s,
        UNUSED branch_history_manager_t *bhm,
        const region_t &r,
        const contract_t &contract,
        const std::function<void(contract_ack_t)> &ack_cb) :
    server_id(sid), store(s), region(r)
{
    guarantee(s->get_region() == region);
    guarantee(c.replicas.count(server_id) == 0);
    ack_cb(contract_ack_t(contract_ack_t::state_t::nothing));
    coro_t::spawn_sometime(std::bind(&erase_t::run, this, drainer.lock()));
}

void erase_t::update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &ack_cb) {
    guarantee(c.replicas.count(server_id) == 0);
    ack_cb(contract_ack_t(contract_ack_t::state_t::nothing));
}

void erase_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        store->reset_data(
            binary_blob_t(version_t::zero()),
            region,
            write_durability_t::HARD,
            keepalive.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}

