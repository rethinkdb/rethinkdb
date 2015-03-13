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

class remote_replicator_client_t {
public:
    remote_replicator_client_t(
        const base_path_t &_base_path,
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
        ) THROWS_ONLY(interrupted_exc_t);

private:
    class write_queue_entry_t {
    public:
        write_queue_entry_t() { }
        write_queue_entry_t(const write_t &w, state_timestamp_t ts, order_token_t ot) :
            write(w), timestamp(ts), order_token(ot) { }
        write_t write;
        state_timestamp_t timestamp;
        order_token_t order_token;
        RDB_DECLARE_ME_SERIALIZABLE(write_queue_entry_t);
    };

    /* This has the effect of friending the serialization functions, so that they can
    serialize `write_queue_entry_t` even though it's private. */
    RDB_DECLARE_ME_SERIALIZABLE(write_queue_entry_t);

    void on_write_async(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token)
        THROWS_NOTHING;

    void perform_enqueued_write(
            const write_queue_entry_t &serialized_write,
            state_timestamp_t backfill_end_timestamp,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void on_write_sync(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_durability_t durability,
            mailbox_addr_t<void(write_response_t)> ack_addr)
        THROWS_NOTHING;

    void on_read(
            signal_t *interruptor,
            const read_t &read,
            state_timestamp_t min_timestamp,
            mailbox_addr_t<void(read_response_t)> ack_addr)
        THROWS_NOTHING;

    mailbox_manager_t *const mailbox_manager_;
    store_view_t *const store_;

    // This uuid exists solely as a temporary used to be passed to
    // uuid_to_str for perfmon_collection initialization and the
    // backfill queue file name
    const uuid_u uuid_;

    perfmon_collection_t perfmon_collection_;
    perfmon_membership_t perfmon_collection_membership_;

    scoped_ptr_t<replica_t> replica_;

    cond_t registered_;

    scoped_ptr_t<timestamp_enforcer_t> write_queue_entrance_enforcer_;
    disk_backed_queue_wrapper_t<write_queue_entry_t> write_queue_;
    scoped_ptr_t<std_function_callback_t<write_queue_entry_t> >
        write_queue_coro_pool_callback_;
    adjustable_semaphore_t write_queue_semaphore_;
    cond_t write_queue_has_drained_;

    /* Destroying `write_queue_coro_pool` will stop any invocations of
    `perform_enqueued_write()`. We mustn't access any member variables defined
    below `write_queue_coro_pool` from within `perform_enqueued_write()`,
    because their destructors might have been called. */
    scoped_ptr_t<coro_pool_t<write_queue_entry_t> > write_queue_coro_pool_;

    remote_replicator_client_bcard_t::write_async_mailbox_t write_async_mailbox_;
    remote_replicator_client_bcard_t::write_sync_mailbox_t write_sync_mailbox_;
    remote_replicator_client_bcard_t::read_mailbox_t read_mailbox_;

    scoped_ptr_t<registrant_t<remote_replicator_client_bcard_t> > registrant_;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_HPP_ */

