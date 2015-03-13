// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_client.hpp"

remote_replicator_client_t::remote_replicator_client_t(
        const base_path_t &base_path,
        io_backender_t *io_backender,
        backfill_throttler_t *backfill_throttler,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &_branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        perfmon_collection_t *backfill_stats_parent,
        order_source_t *order_source,
        signal_t *interruptor,
        double *backfill_progress_out   /* can be null */
        ) THROWS_ONLY(interrupted_exc_t) :

    store_(store),
    uuid_(generate_uuid()),
    perfmon_collection_membership_(
        backfill_stats_parent,
        &perfmon_collection,
        "backfill-serialization-" + uuid_to_str(uuid_)),
    /* TODO: Put the file in the data directory, not here */
    write_queue_(
        io_backender,
        serializer_filepath_t(base_path, "backfill-serialization-" + uuid_to_str(uuid_)),
        &perfmon_collection_),
    write_queue_semaphore_(
        SEMAPHORE_NO_LIMIT,
        WRITE_QUEUE_SEMAPHORE_TRICKLE_FRACTION),
    write_async_mailbox_(mailbox_manager,
        std::bind(&listener_t::on_write_async, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6)),
    write_sync_mailbox_(mailbox_manager,
        std::bind(&listener_t::on_write_sync, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7)),
    read_mailbox_(mailbox_manager,
        std::bind(&listener_t::on_read, this,
            ph::_1, ph::_2, ph::_3, ph::_4))
{
    /* Subscribe to the stream of writes coming from the primary */
    guarantee(remote_replicator_server_bcard.branch_id == branch_id);
    remote_replicator_client_intro_t intro;
    {
        remote_replicator_client_bcard_t::intro_mailbox_t intro_mailbox(
            mailbox_manager,
            [&](signal_t *, const remote_replica_transmitter_intro_t &i) {
                intro = i;
                write_queue_entry_enforcer_.init(
                    new timestamp_enforcer_t(intro.streaming_begin_timestamp));
                registered_.pulse();
            });
        remote_replica_client_bcard_t our_bcard(
            intro_mailbox_.get_address(),
            write_async_mailbox_.get_address(),
            server_id_);
        registrant_.init(new registrant_t<remote_replica_client_bcard_t>(
            mailbox_manager, remote_replicator_server_bcard.registrar, our_bcard));
        wait_interruptible(&registered_, interruptor);
    }

    /* OK, now we're streaming writes into our queue, but not applying them to the B-tree
    yet. */

    /* Block until the backfiller is at least as up-to-date as the point where we started
    streaming writes from the primary */
    {
        cond_t backfiller_is_up_to_date;
        mailbox_t<void()> ack_mbox(
            mailbox_manager,
            [&](signal_t *) { backfiller_is_up_to_date.pulse(); });
        send(mailbox_manager, replica_bcard.synchronize_mailbox, 
            intro.streaming_begin_timestamp, ack_mbox.get_address());
        wait_interruptible(&backfiller_is_up_to_date, interruptor);
    }

    /* Backfill from the backfiller */
    {
        guarantee(replica_bcard.branch_id == branch_id);
        peer_id_t peer = replica_bcard.backfiller_bcard.backfill_mailbox.get_peer();
        backfill_throttler_t::lock_t throttler_lock(
            backfill_throttler, peer, interruptor);
        backfillee(mailbox_manager,
                   branch_history_manager,
                   store_,
                   store_->get_region(),
                   replica_bcard.backfiller_bcard,
                   interruptor,
                   backfill_progress_out);
    } // Release throttler_lock

    /* Figure out where the backfill left us at */
    state_timestamp_t backfill_end_timestamp;
    {
        read_token_t read_token2;
        svs_->new_read_token(&read_token2);
        region_map_t<binary_blob_t> backfill_end_point_blob;
        svs_->do_get_metainfo(
            order_source->check_in("listener_t(B)").with_read_mode(),
            &read_token2, interruptor, &backfill_end_point_blob);
        region_map_t<version_t> backfill_end_point =
            to_version_map(backfill_end_point_blob);
        guarantee(backfill_end_point.get_domain() == svs_->get_region());

        bool first_chunk = true;
        for (const auto &chunk : backfill_end_point) {
            guarantee(chunk.second.branch == branch_id_);
            if (first_chunk) {
                backfill_end_timestamp = chunk.second.timestamp;
                first_chunk = false;
            } else {
                guarantee(backfill_end_timestamp == chunk.second.timestamp);
            }
        }
    }

    guarantee(backfill_end_timestamp >= intro.streaming_begin_timestamp);

    /* Construct the `replica_t` so that we can perform writes */
    replica.init(new replica_t(
        mailbox_manager,
        store,
        branch_history_manager,
        branch_id,
        backfill_end_timestamp));

    /* Start performing the queued-up writes */
    write_queue_coro_pool_callback_.init(
        new std_function_callback_t<write_queue_entry_t>(
            std::bind(&listener_t::perform_enqueued_write, this,
                ph::_1, backfill_end_timestamp, ph::_2)));
    write_queue_coro_pool_.init(new coro_pool_t<write_queue_entry_t>(
            WRITE_QUEUE_CORO_POOL_SIZE, &write_queue_,
            write_queue_coro_pool_callback_.get()));
    write_queue_semaphore_.set_capacity(WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY);

    /* Wait for the write queue to drain completely */
    if (write_queue_.size() <= WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY) {
        write_queue_has_drained_.pulse_if_not_already_pulsed();
    }
    wait_interruptible(&write_queue_has_drained_, interruptor);

    /* Tell the primary that it's OK to send us reads and synchronous writes */
    send(mailbox_manager, intro.ready_mailbox);
}


void remote_replicator_client_t::on_write_async(
        signal_t *interruptor,
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token)
        THROWS_NOTHING {
    wait_interruptible(&registered_, interruptor);
    write_queue_entrance_enforcer_->wait_all_before(timestamp, interruptor);
    write_queue_semaphore_.co_lock_interruptible(interruptor);
    write_queue_.push(write_queue_entry_t(write, timestamp, order_token));
    write_queue_entrance_enforcer_->complete(timestamp);
}

void remote_replicator_client_t::perform_enqueued_write(
        const write_queue_entry_t &qe,
        state_timestamp_t backfill_end_timestamp,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    write_queue_semaphore_.unlock();
    if (write_queue_.size() <= WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY) {
        write_queue_has_drained_.pulse_if_not_already_pulsed();
    }

    write_response_t dummy;
    replica_->do_write(
        qe.write, qe.timestamp, qe.order_token, write_durability_t::SOFT,
        interruptor, &dummy);
}

void remote_replicator_client_t::on_write_sync(
        signal_t *interruptor,
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        mailbox_addr_t<void(write_response_t)> ack_addr)
        THROWS_NOTHING {
    /* We aren't inserting into the queue ourselves, so there's no need to wait on
    `write_queue_entrance_enforcer_`. But a call to `on_write_async()` could
    hypothetically come later, so we need to tell the `write_queue_entrance_enforcer_`
    not to wait for us. */
    write_queue_entrance_enforcer_->complete(timestamp);

    write_response_t response;
    replica_->do_write(
        write, timestamp, order_token, durability,
        interruptor, &response);
    send(ack_addr, response);
}

void remote_replicator_client_t::on_read(
        signal_t *interruptor,
        const read_t &read,
        state_timestamp_t min_timestamp,
        mailbox_addr_t<void(read_response_t)> ack_addr)
        THROWS_NOTHING {
    read_response_t response;
    replica_->do_read(read, min_timestamp, interruptor, &response);
    send(ack_addr, response);
}

