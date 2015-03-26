// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_CLIENT_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_CLIENT_HPP_

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
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    ~remote_replicator_client_t();

private:
    class queue_entry_t {
    public:
        write_t write;
        state_timestamp_t timestamp;
        order_token_t order_token;
    };
    typedef std::function<void(queue_entry_t &&, cond_t *)> queue_function_t;

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

    cond_t registered_;

    scoped_ptr_t<timestamp_enforcer_t> timestamp_enforcer_;
    region_t region_streaming_, region_queueing_, region_discarding_;
    queue_function_t *queue_fun_;

    scoped_ptr_t<replica_t> replica_;

    remote_replicator_client_bcard_t::write_async_mailbox_t write_async_mailbox_;
    remote_replicator_client_bcard_t::write_sync_mailbox_t write_sync_mailbox_;
    remote_replicator_client_bcard_t::read_mailbox_t read_mailbox_;

    scoped_ptr_t<registrant_t<remote_replicator_client_bcard_t> > registrant_;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_HPP_ */

