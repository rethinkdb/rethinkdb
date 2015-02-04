// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_raft_follower.hpp"

void follower_t::execute_contract(
        const region_t &region,
        const contract_id_t &cid,
        const contract_t &c,
        store_t *store,
        ongoing_t *ongoing,
        signal_t *interruptor) {
    if (static_cast<bool>(c.primary) && server_id == c.primary->server) {
        
    } else if (c.replicas.count(server_id) != 0) {
        
    } else {
        ongoing->replier.reset();
        ongoing->listener.reset();
        ongoing->broadcaster.reset();
        ongoing->branch = boost::none;
        store->reset_data(binary_blob_t(version_t::zero()),
            region, write_durability_t::HARD, interruptor);
        ack(contract_ack_t::nothing, boost::none, boost::none);
    }
}

void follower_t::execute_contract_primary(
        const region_t &region,
        const contract_id_t &cid,
        const contract_t &c,
        store_t *store,
        ongoing_primary_t *ongoing,
        signal_t *interruptor) {
}

void follower_t::execute_contract_secondary(
        const region_t &region,
        const contract_id_t &cid,
        const contract_t &c,
        store_t *store,
        ongoing_secondary_t *ongoing,
        signal_t *interruptor) {
    boost::optional<branch_id_t> branch =
        static_cast<bool>(c.primary) ? boost::make_optional(c.branch) : boost::none;
    if (ongoing->branch != branch) {
        ongoing->replier.reset();
        ongoing->listener.reset();
    }
    ongoing->branch = branch;
    if (static_cast<bool>(branch)) {
        if (!ongoing->listener.has()) {
            broadcaster_business_card_t bcard = find_broadcaster(c.branch, interruptor);
            ongoing->listener.init(new listener_t(
                ...,
                bcard,
                ...));
        }
        if (!ongoing->replier.has()) {
            ongoing->replier.init(new replier_t(
                ...));
        }
        wait_interruptible(
            ongoing->listener->get_broadcaster_lost_signal(),
            interruptor);
        ongoing->replier.reset();
        ongoing->listener.reset();
    }
}

void follower_t::execute_contract_nothing(
        const region_t &region,
        const contact_id_t &cid,
        UNUSED const contract_t &c,
        store_t *store,
        signal_t *interruptor) {
    store->reset_data(binary_blob_t(version_t::zero()),
        region, write_durability_t::HARD, interruptor);
    ack(contract_ack_t::nothing, boost::none, boost::none);
    interruptor->wait_lazily_unordered();
    throw interrupted_exc_t();
}
