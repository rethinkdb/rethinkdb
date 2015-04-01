// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_DISPATCHER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_DISPATCHER_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/protocol.hpp"

/* The job of the `primary_dispatcher_t` is:
- Take in reads and writes
- Assign timestamps to the reads and writes
- Distribute the reads and writes to the replicas
- Collect the results and return them to the caller

It takes in reads and writes via its `read()` and `spawn_write()` methods. Replicas
register for reads and writes by subclassing `dispatchee_t` and constructing a
`dispatchee_registration_t` object.

There is one `primary_dispatcher_t` for each shard; it's located on the primary
replica server. `primary_execution_t` constructs it. */

class primary_dispatcher_t : public home_thread_mixin_debug_only_t {
private:
    class incomplete_write_t;

public:
    /* `dispatchee_t` represents a replica from the `primary_dispatcher_t`'s point of
    view. The subclass might run the reads and writes directly, or it might send them
    over the network to a different server. */
    class dispatchee_t {
    public:
        virtual void do_read(
            const read_t &read,
            state_timestamp_t min_timestamp,
            signal_t *interruptor,
            read_response_t *response_out) = 0;

        /* `do_write_sync()` blocks until the write has been performed with the given
        durability level, and it produces a `write_response_t`. `do_write_async()` makes
        no promises about when it returns and it does not return a response. However,
        `do_write_async()` may block in order to limit the rate at which the
        `primary_dispatcher_t` sends writes. */
        virtual void do_write_sync(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_durability_t durability,
            signal_t *interruptor,
            write_response_t *response_out) = 0;
        virtual void do_write_async(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            signal_t *interruptor) = 0;
    protected:
        virtual ~dispatchee_t() { }
    };

    /* `dispatchee_registration_t` is a sentry object that signs the dispatchee up for
    reads and writes. */
    class dispatchee_registration_t {
    public:
        dispatchee_registration_t(
            primary_dispatcher_t *parent,
            dispatchee_t *dispatchee,
            /* `server_id` is used for reporting acks to the `write_callback_t`. */
            const server_id_t &server_id,
            /* `priority` is used for deciding which of several dispatchees should handle
            a read. The local dispatchee has priority 2.0 and remote dispatchees have
            priority 1.0. */
            double priority,
            /* `*first_timestamp_out` will be set to whatever was the latest timestamp
            value when the `dispatchee_registration_t` was constructed. */
            state_timestamp_t *first_timestamp_out);

        /* Destroying the `dispatchee_registration_t` will interrupt any running read or
        write operations on this dispatchee. */
        ~dispatchee_registration_t();

        /* Initially, dispatchees will only receive `do_write_async()` calls. But after
        `mark_ready()` is called, the `primary_dispatcher_t` may also send them reads and
        `do_write_sync()` calls. */
        void mark_ready();

    private:
        friend class primary_dispatcher_t;

        primary_dispatcher_t *const parent;
        dispatchee_t *const dispatchee;
        server_id_t const server_id;
        double const priority;

        bool is_ready;

        perfmon_counter_t queue_count;
        perfmon_membership_t queue_count_membership;

        /* TODO: Maybe this should be a queue of `incomplete_write_t`s instead of a queue
        of `std::function`s. */
        unlimited_fifo_queue_t<std::function<void()> > background_write_queue;
        calling_callback_t background_write_caller;
        coro_pool_t<std::function<void()> > background_write_workers;

        /* This is the timestamp of the latest write for which a `do_write_sync()` call
        has completed. We use this to pick the most up-to-date dispatchee to send reads
        to. */
        state_timestamp_t latest_acked_write;

        auto_drainer_t drainer;
    };

    class write_callback_t {
    public:
        write_callback_t();
        /* `get_default_write_durability()` returns the write durability that this write
        should use if the write itself didn't specify. */
        virtual write_durability_t get_default_write_durability() = 0;
        /* Every time the write is acked, `on_ack()` is called. When no more replicas
        will ack the write, `on_end()` is called. */
        virtual void on_ack(const server_id_t &, write_response_t &&) = 0;
        virtual void on_end() = 0;

    protected:
        virtual ~write_callback_t();

    private:
        friend class primary_dispatcher_t;
        /* This is so that if the write callback is destroyed before `on_end()` is
        called, it will get deregistered. */
        incomplete_write_t *write;
    };

    /* There is a 1:1 relationship between `primary_dispatcher_t`s and branches. When the
    `primary_dispatcher_t` is constructed, it constructs a branch using the given region
    map as the origin. You can get the branch's ID and birth certificate via the
    `get_branch_id()` and `get_branch_birth_certificate()` methods. */

    primary_dispatcher_t(
        perfmon_collection_t *parent_perfmon_collection,
        const region_map_t<version_t> &base_version);

    branch_id_t get_branch_id() {
        return branch_id;
    }
    branch_birth_certificate_t get_branch_birth_certificate() {
        return branch_bc;
    }

    /* `read()` performs the given read, blocking until the read is complete. */
    void read(
        const read_t &r,
        fifo_enforcer_sink_t::exit_read_t *lock,
        order_token_t tok,
        signal_t *interruptor,
        read_response_t *response_out)
        THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t);

    /* Unlike `read()`, `spawn_write()` returns as soon as the write has begun and
    replies asynchronously via a callback. It will not block. If the `write_callback_t`
    is destroyed while the write is still in progress, its destructor will automatically
    deregister it so that no segfaults will happen. */
    void spawn_write(
        const write_t &w,
        order_token_t tok,
        write_callback_t *cb);

    clone_ptr_t<watchable_t<std::set<server_id_t> > > get_ready_dispatchees() {
        return ready_dispatchees_as_set.get_watchable();
    }

private:
    /* `incomplete_write_t` bundles all of the information related to a given write into
    a single struct. When it is destroyed, it calls `on_end()` on the callback. */
    class incomplete_write_t : public single_threaded_countable_t<incomplete_write_t> {
    public:
        incomplete_write_t(
            const write_t &w, state_timestamp_t ts, order_token_t ot,
            write_durability_t durability, write_callback_t *cb);
        ~incomplete_write_t();
        write_t write;
        state_timestamp_t timestamp;
        order_token_t order_token;
        write_durability_t durability;
        write_callback_t *callback;
    };

    void background_write(
        dispatchee_registration_t *dispatchee,
        auto_drainer_t::lock_t dispatchee_lock,
        counted_t<incomplete_write_t> write) THROWS_NOTHING;

    branch_id_t branch_id;
    branch_birth_certificate_t branch_bc;

    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_membership;

    mutex_assertion_t mutex;

    state_timestamp_t current_timestamp;
    order_checkpoint_t order_checkpoint;

    /* Once we ack a write, we must make sure that every read that's initiated after that
    will see the result of the write. We use this timestamp to keep track of the most
    recent acked write and produce `min_timestamp_token_t`s from it whenever we send a
    read to a listener. */
    state_timestamp_t most_recent_acked_write_timestamp;

    std::map<dispatchee_registration_t *, auto_drainer_t::lock_t> dispatchees;

    /* This is just a set that contains the peer ID of each dispatchee in `dispatchees`
    that's readable. We store it separately so we can expose it to code that needs to
    know which replicas are available. */
    watchable_variable_t<std::set<server_id_t> > ready_dispatchees_as_set;

    DISABLE_COPYING(primary_dispatcher_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_ */

