// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_client.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/backfillee.hpp"
#include "store_view.hpp"

/* `WRITE_QUEUE_CORO_POOL_SIZE` is the number of coroutines that will be used when
draining the write queue after completing a backfill. */
#define WRITE_QUEUE_CORO_POOL_SIZE 64

/* When we have caught up to the primary replica to within
`WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY` elements, then we consider ourselves to be
up-to-date. */
#define WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY 5

/* When we are draining the write queue, we allow new objects to be added to the write
queue at (this rate) * (the rate at which objects are being popped). If this number is
high, then the backfill will take longer but the cluster will serve queries at a high
rate during the backfill. If this number is low, then the backfill will be faster but the
cluster will only serve queries slowly during the backfill. */
#define WRITE_QUEUE_SEMAPHORE_TRICKLE_FRACTION 0.5

class remote_replicator_client_t::write_queue_entry_t {
public:
    write_queue_entry_t() { }
    write_queue_entry_t(const write_t &w, state_timestamp_t ts, order_token_t ot) :
        write(w), timestamp(ts), order_token(ot) { }
    write_t write;
    state_timestamp_t timestamp;
    order_token_t order_token;
    RDB_MAKE_ME_SERIALIZABLE_3(write_queue_entry_t, write, timestamp, order_token);
};

remote_replicator_client_t::remote_replicator_client_t(
        const base_path_t &base_path,
        io_backender_t *io_backender,
        backfill_throttler_t *backfill_throttler,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        perfmon_collection_t *backfill_stats_parent,
        order_source_t *order_source,
        signal_t *interruptor,
        double *backfill_progress_out   /* can be null */
        ) THROWS_ONLY(interrupted_exc_t) :

    mailbox_manager_(mailbox_manager),
    store_(store),
    uuid_(generate_uuid()),
    perfmon_collection_membership_(
        backfill_stats_parent,
        &perfmon_collection_,
        "backfill-serialization-" + uuid_to_str(uuid_)),
    write_queue_(new disk_backed_queue_wrapper_t<write_queue_entry_t>(
        io_backender,
        serializer_filepath_t(base_path, "backfill-serialization-" + uuid_to_str(uuid_)),
        &perfmon_collection_)),
    write_queue_semaphore_(
        SEMAPHORE_NO_LIMIT,
        WRITE_QUEUE_SEMAPHORE_TRICKLE_FRACTION),
    write_async_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_write_async, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5)),
    write_sync_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_write_sync, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6)),
    read_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_read, this,
            ph::_1, ph::_2, ph::_3, ph::_4))
{
    /* Subscribe to the stream of writes coming from the primary */
    guarantee(remote_replicator_server_bcard.branch == branch_id);
    remote_replicator_client_intro_t intro;
    {
        remote_replicator_client_bcard_t::intro_mailbox_t intro_mailbox(
            mailbox_manager,
            [&](signal_t *, const remote_replicator_client_intro_t &i) {
                intro = i;
                write_queue_entrance_enforcer_.init(
                    new timestamp_enforcer_t(intro.streaming_begin_timestamp));
                registered_.pulse();
            });
        remote_replicator_client_bcard_t our_bcard {
            server_id,
            intro_mailbox.get_address(),
            write_async_mailbox_.get_address(),
            write_sync_mailbox_.get_address(),
            read_mailbox_.get_address() };
        registrant_.init(new registrant_t<remote_replicator_client_bcard_t>(
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
        store_->new_read_token(&read_token2);
        region_map_t<binary_blob_t> backfill_end_point_blob;
        store_->do_get_metainfo(
            order_source->check_in("listener_t(B)").with_read_mode(),
            &read_token2, interruptor, &backfill_end_point_blob);
        region_map_t<version_t> backfill_end_point =
            to_version_map(backfill_end_point_blob);
        guarantee(backfill_end_point.get_domain() == store_->get_region());

        bool first_chunk = true;
        for (const auto &chunk : backfill_end_point) {
            guarantee(chunk.second.branch == branch_id);
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
    replica_.init(new replica_t(
        mailbox_manager,
        store,
        branch_history_manager,
        branch_id,
        backfill_end_timestamp));

    /* Start performing the queued-up writes */
    write_queue_coro_pool_callback_.init(
        new std_function_callback_t<write_queue_entry_t>(
            std::bind(&remote_replicator_client_t::perform_enqueued_write, this,
                ph::_1, backfill_end_timestamp, ph::_2)));
    write_queue_coro_pool_.init(new coro_pool_t<write_queue_entry_t>(
            WRITE_QUEUE_CORO_POOL_SIZE, write_queue_.get(),
            write_queue_coro_pool_callback_.get()));
    write_queue_semaphore_.set_capacity(WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY);

    /* Wait for the write queue to drain completely */
    if (write_queue_->size() <= WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY) {
        write_queue_has_drained_.pulse_if_not_already_pulsed();
    }
    wait_interruptible(&write_queue_has_drained_, interruptor);

    /* Tell the primary that it's OK to send us reads and synchronous writes */
    send(mailbox_manager, intro.ready_mailbox);
}

remote_replicator_client_t::~remote_replicator_client_t() {
    /* This is declared in the `.cc` file because we need to be able to see the full
    definition of `write_queue_entry_t` in the destructor. */
}

void remote_replicator_client_t::on_write_async(
        signal_t *interruptor,
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        const mailbox_t<void()>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    wait_interruptible(&registered_, interruptor);
    write_queue_entrance_enforcer_->wait_all_before(timestamp.pred(), interruptor);
    write_queue_semaphore_.co_lock_interruptible(interruptor);
    write_queue_->push(write_queue_entry_t(write, timestamp, order_token));
    write_queue_entrance_enforcer_->complete(timestamp);
    send(mailbox_manager_, ack_addr);
}

void remote_replicator_client_t::perform_enqueued_write(
        const write_queue_entry_t &qe,
        state_timestamp_t backfill_end_timestamp,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    write_queue_semaphore_.unlock();
    if (write_queue_->size() <= WRITE_QUEUE_SEMAPHORE_LONG_TERM_CAPACITY) {
        write_queue_has_drained_.pulse_if_not_already_pulsed();
    }

    if (qe.timestamp <= backfill_end_timestamp) {
        return;
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
        const mailbox_t<void(write_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    /* We aren't inserting into the queue ourselves, so there's no need to wait on
    `write_queue_entrance_enforcer_`. But a call to `on_write_async()` could
    hypothetically come later, so we need to tell the `write_queue_entrance_enforcer_`
    not to wait for us. */
    write_queue_entrance_enforcer_->complete(timestamp);

    write_response_t response;
    replica_->do_write(
        write, timestamp, order_token, durability,
        interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

void remote_replicator_client_t::on_read(
        signal_t *interruptor,
        const read_t &read,
        state_timestamp_t min_timestamp,
        const mailbox_t<void(read_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    read_response_t response;
    replica_->do_read(read, min_timestamp, interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

