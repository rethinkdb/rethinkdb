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

class backfill_progress_tracker_t;
class backfill_throttler_t;

/* `remote_replicator_client_t` contacts a `remote_replicator_server_t` on another server
to sign up for writes to a given shard, and then applies them to a `store_t` on the same
server. It also performs a backfill to initialize the state of the `store_t`, and handles
the transition between the backfill and the streaming writes.

There is one `remote_replicator_client_t` on each secondary replica server of each shard.
`secondary_execution_t` constructs it. */

class remote_replicator_client_t {
public:
    remote_replicator_client_t(
        backfill_throttler_t *backfill_throttler,
        const backfill_config_t &backfill_config,
        backfill_progress_tracker_t *backfill_progress_tracker,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,
        const server_id_t &primary_server_id,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    ~remote_replicator_client_t();

private:
    class timestamp_range_tracker_t;

    /* `on_write_async()`, `on_write_sync()`, and `on_read()` are mailbox callbacks for
    `write_async_mailbox_`, `write_sync_mailbox_`, and `read_mailbox_`. */
    void on_write_async(
            signal_t *interruptor,
            write_t &&write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            const mailbox_t<void()>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t);

    void on_write_sync(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_durability_t durability,
            const mailbox_t<void(write_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t);

    void on_read(
            signal_t *interruptor,
            const read_t &read,
            state_timestamp_t min_timestamp,
            const mailbox_t<void(read_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t);

    mailbox_manager_t *const mailbox_manager_;
    store_view_t *const store_;
    region_t const region_;   /* same as `store_->get_region()` */
    branch_id_t const branch_id_;

    /* During the constructor, we alternate between `PAUSED` and `BACKFILLING`. When the
    constructor is done, we switch to `STREAMING` mode and stay there. */
    enum class backfill_mode_t {
        /* We haven't backfilled completely, but we aren't currently backfilling either.
        Usually this means we're waiting for the backfill throttler. However, we will
        still accept streaming writes in the regions we've already backfilled. */
        PAUSED,
        /* We're actively receiving backfill items over the network. Whenever the
        backfill reaches a given timestamp, we'll also apply all writes at or before that
        timestamp; this is mediated by `backfilling_thresholds_`. */
        BACKFILLING,
        /* We finished backfilling and we're just applying writes as they arrive. */
        STREAMING
        };
    backfill_mode_t mode_;

    /* `timestamp_range_tracker_t` is essentially a `region_map_t<state_timestamp_t>`,
    but in a different format and optimized for this specific use case. The domain of
    `tracker_` is the region that has been backfilled thus far; the values are equal to
    the current timestamps in the B-tree metainfo. `tracker_` exists only during the
    backfill; it gets destroyed after the backfill is over. */
    scoped_ptr_t<timestamp_range_tracker_t> tracker_;

    /* Returns `true` if the next write can be applied now, instead of having to wait for
    the backfill to make more progress. */
    bool next_write_can_proceed(mutex_assertion_t::acq_t *mutex_acq);

    /* If the next write cannot proceed, it will set `next_write_waiter_` and wait for it
    to be pulsed. */
    cond_t *next_write_waiter_;

    /* `timestamp_enforcer_` is used to order writes as they arrive. `replica_` will do
    its own ordering of writes, so `timestamp_enforcer_` is only important during the
    constructor before `replica_` has been created. */
    scoped_ptr_t<timestamp_enforcer_t> timestamp_enforcer_;

    /* `replica_` is created at the end of the constructor, once the backfill is over. */
    scoped_ptr_t<replica_t> replica_;

    mutex_assertion_t mutex_assertion_;

    rwlock_t rwlock_;

    remote_replicator_client_bcard_t::write_async_mailbox_t write_async_mailbox_;
    remote_replicator_client_bcard_t::write_sync_mailbox_t write_sync_mailbox_;
    remote_replicator_client_bcard_t::read_mailbox_t read_mailbox_;

    /* We use `registrant_` to subscribe to a stream of reads and writes from the
    dispatcher via the `remote_replicator_server_t`. */
    scoped_ptr_t<registrant_t<remote_replicator_client_bcard_t> > registrant_;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_HPP_ */

