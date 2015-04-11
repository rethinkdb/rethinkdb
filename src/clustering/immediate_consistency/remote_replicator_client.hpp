// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_CLIENT_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_CLIENT_HPP_

#include <queue>

#include "clustering/generic/registrant.hpp"
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"
#include "clustering/immediate_consistency/replica.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "concurrency/semaphore.hpp"

class backfill_throttler_t;

/* `remote_replicator_client_t` contacts a `remote_replicator_server_t` on another server
to sign up for writes to a given shard, and then applies them to a `store_t` on the same
server. It also performs a backfill to initialize the state of the `store_t`, and handles
the transition between the backfill and the streaming writes.

There is one `remote_replicator_client_t` on each secondary replica server of each shard.
`secondary_execution_t` constructs it. */

class remote_replicator_client_t {
private:
    class backfill_end_timestamps_t;

public:
    /* Here's how the backfill works:

    1. We sign up for a stream of writes from the `remote_replicator_server_t`. Initially
        we store the writes in `write_queue_`.
    2. We start backfilling from the `replica_t`. We ensure that the `replica_t` is on
        the same branch as the `remote_replicator_server_t`, and that the backfill's end
        timestamp is greater than or equal to the stream's begin timestamp.
    3. When `write_queue_` reaches a certain size, we stop the backfill. We perform all
        the queued writes that affect the region that we backfilled, and discard all
        others.
    4. Go back to step 1, except that incoming writes that apply to the region that was
        backfilled are performed immediately instead of being queued.
    5. Repeat steps 1-4 until the whole region has been backfilled.

    The `remote_replicator_client_t` constructor blocks until this entire process is
    complete. */

    remote_replicator_client_t(
        backfill_throttler_t *backfill_throttler,
        const backfill_config_t &backfill_config,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

private:
    class queue_entry_t {
    public:
        /* `has_write` is `false` if the write has no effect on the region that is being
        queued. We still create a queue entry for such writes because we need to update
        the metainfo. */
        bool has_write;
        write_t write;
        state_timestamp_t timestamp;
        order_token_t order_token;
    };
    typedef std::function<void(queue_entry_t &&, cond_t *)> queue_function_t;

    /* `apply_write_or_metainfo()` is a helper function for `drain_stream_queue()` and
    `on_write_async()`. It applies the given write to the store and updates the metainfo;
    but if `has_write` is `false`, it only updates the metainfo. */
    static void apply_write_or_metainfo(
            store_view_t *store,
            const branch_id_t &branch_id,
            const region_t &region,
            bool has_write,
            const write_t &write,
            state_timestamp_t timestamp,
            write_token_t *token,
            order_token_t order_token,
            signal_t *interruptor);

    /* `drain_stream_queue()` is a helper function for the constructor. It applies the
    queue entries in `queue` to `store`. When the queue is empty, it calls
    `on_queue_empty()`; if the queue is still empty after `on_queue_empty()` returns,
    then `drain_stream_queue()` returns. */
    static void drain_stream_queue(
            store_view_t *store,
            const branch_id_t &branch_id,
            const region_t &region,
            std::queue<queue_entry_t> *queue,
            const backfill_end_timestamps_t &bets,
            const std::function<void(signal_t *)> &on_queue_empty,
            const std::function<void(signal_t *)> &on_finished_one_entry,
            signal_t *interruptor);

    /* `on_write_async()`, `on_write_sync()`, and `on_read()` are mailbox callbacks for
    `write_async_mailbox_`, `write_sync_mailbox_`, and `read_mailbox_`. */
    void on_write_async(
            signal_t *interruptor,
            write_t &&write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            const mailbox_t<void()>::address_t &ack_addr)
        THROWS_NOTHING;
    void on_write_sync(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_durability_t durability,
            const mailbox_t<void(write_response_t)>::address_t &ack_addr)
        THROWS_NOTHING;
    void on_read(
            signal_t *interruptor,
            const read_t &read,
            state_timestamp_t min_timestamp,
            const mailbox_t<void(read_response_t)>::address_t &ack_addr)
        THROWS_NOTHING;

    mailbox_manager_t *const mailbox_manager_;
    store_view_t *const store_;
    branch_id_t const branch_id_;

    /* `timestamp_enforcer_` is used to order writes as they arrive. `replica_` will do
    its own ordering of writes, so `timestamp_enforcer_` is only important during the
    constructor before `replica_` has been created. */
    scoped_ptr_t<timestamp_enforcer_t> timestamp_enforcer_;

    /* `region_*_`, `queue_fun_`, and `queue_order_checkpoint_` are only used during the
    constructor. Specifically, the constructor uses them to control what
    `on_write_async()` does with writes that arrive during the backfill. See
    `on_write_async()` for more information. */
    region_t region_streaming_, region_queueing_, region_discarding_;
    queue_function_t *queue_fun_;
    order_checkpoint_t queue_order_checkpoint_;

    /* `replica_` is created at the end of the constructor, once the backfill is over. */
    scoped_ptr_t<replica_t> replica_;

    /* Read access to `rwlock_` is required when reading `region_*_`, `queue_fun_`,
    or `replica_`, or when calling `timestamp_enforcer_->complete()`. Write access to
    `rwlock_` is required when writing to `region_*_`, `queue_fun_`, or `replica_`. */
    rwlock_t rwlock_;

    /* `registered` is pulsed once we've gotten the initial message from the
    `remote_replicator_server_t`. `timestamp_enforcer_` won't be initialized until
    `registered_` is pulsed. */
    cond_t registered_;

    remote_replicator_client_bcard_t::write_async_mailbox_t write_async_mailbox_;
    remote_replicator_client_bcard_t::write_sync_mailbox_t write_sync_mailbox_;
    remote_replicator_client_bcard_t::read_mailbox_t read_mailbox_;

    /* We use `registrant_` to subscribe to a stream of reads and writes from the
    dispatcher via the `remote_replicator_server_t`. */
    scoped_ptr_t<registrant_t<remote_replicator_client_bcard_t> > registrant_;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_HPP_ */

