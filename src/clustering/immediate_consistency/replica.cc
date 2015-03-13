// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/replica.hpp"

replica_t::replica_t(
        mailbox_manager_t *_mailbox_manager,
        store_view_t *_store,
        branch_history_manager_t *_bhm,
        const branch_id_t &_branch_id,
        state_timestamp_t _timestamp) :
    mailbox_manager(_mailbox_manager),
    store(_store),
    branch_id(_branch_id),
    start_enforcer(_timestamp),
    end_enforcer(_timestamp),
    backfiller(_mailbox_manager, _bhm, _store)
    { }

void replica_t::do_read(
        const read_t &read,
        state_timestamp_t min_timestamp,
        signal_t *interruptor,
        read_response_t *response_out) {
    assert_thread();
    rassert(region_is_superset(store->get_region(), read.get_region()));
    rassert(!region_is_empty(read.get_region()));

    // Wait until all writes that the read needs to see have completed.
    end_enforcer.wait_all_before(min_timestamp, interruptor);

    // Leave the token empty. We're enforcing ordering ourselves through `end_enforcer`.
    read_token_t read_token;

#ifndef NDEBUG
    trivial_metainfo_checker_callback_t metainfo_checker_callback;
    metainfo_checker_t metainfo_checker(
        &metainfo_checker_callback, svs_->get_region());
#endif

    // Perform the operation
    store->read(
        DEBUG_ONLY(metainfo_checker, )
        read,
        &response,
        &read_token,
        interruptor);
}

void replica_t::do_write(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        signal_t *interruptor,
        read_response_t *response_out) {
    assert_thread();
    rassert(region_is_superset(store->get_region(), write.get_region()));
    rassert(!region_is_empty(write.get_region()));
    order_token.assert_write_mode();

    write_token_t write_token;
    {
        /* Wait until it's our turn to go. */
        start_enforcer.wait_all_before(timestamp, interruptor);

        /* Claim a write token. This determines the order in which writes will be
        processed by the underlying store. */
        store->new_write_token(&write_token);

        /* Notify the next write (if any) that it's their turn to go. */
        start_enforcer.complete(timestamp);
    }

#ifndef NDEBUG
    version_leq_metainfo_checker_callback_t metainfo_checker_callback(timestamp.pred());
    metainfo_checker_t metainfo_checker(&metainfo_checker_callback, svs_->get_region());
#endif

    // Perform the operation
    svs_->write(DEBUG_ONLY(metainfo_checker, )
                region_map_t<binary_blob_t>(
                    svs_->get_region(),
                    binary_blob_t(version_t(branch_id, timestamp))),
                write,
                response,
                durability,
                timestamp,
                order_token,
                &write_token,
                &combined_interruptor);

    /* Notify reads that were waiting for this write that it's OK to go */
    end_enforcer.complete(timestamp);
}

void replica_t::on_synchronize(
        signal_t *interruptor,
        state_timestamp_t timestamp,
        mailbox_t<void()>::address_t ack_addr) {
    end_enforcer.wait_all_before(timestamp, interruptor);
    send(mailbox_manager, ack_addr);
}

