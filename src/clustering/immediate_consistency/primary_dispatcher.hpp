// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_DISPATCHER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_DISPATCHER_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/protocol.hpp"

class primary_dispatcher_t : public home_thread_mixin_debug_only_t {
private:
    class incomplete_write_t;

public:
    class dispatchee_t {
    public:
        virtual void do_read(
            const read_t &read,
            state_timestamp_t min_timestamp,
            signal_t *interruptor,
            read_response_t *response_out) = 0;
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

    class dispatchee_registration_t {
    public:
        dispatchee_registration_t(
            primary_dispatcher_t *parent,
            dispatchee_t *dispatchee,
            const server_id_t &server_id,
            double priority,
            state_timestamp_t *first_timestamp_out);
        ~dispatchee_registration_t();

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

        unlimited_fifo_queue_t<std::function<void()> > background_write_queue;
        calling_callback_t background_write_caller;
        coro_pool_t<std::function<void()> > background_write_workers;

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

    primary_dispatcher_t(
        perfmon_collection_t *parent_perfmon_collection,
        const region_map_t<version_t> &base_version);
    DISABLE_COPYING(primary_dispatcher_t);

    branch_id_t get_branch_id() {
        return branch_id;
    }
    branch_birth_certificate_t get_branch_birth_certificate() {
        return branch_bc;
    }

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

    /* Once we ack a write, we must make sure that every read that's initiated
    after that will see the result of the write. We use this timestamp to keep
    track of the most recent acked write and produce `min_timestamp_token_t`s from
    it whenever we send a read to a listener. */
    state_timestamp_t most_recent_acked_write_timestamp;

    std::map<dispatchee_registration_t *, auto_drainer_t::lock_t> dispatchees;

    /* This is just a set that contains the peer ID of each dispatchee in
    `ready_dispatchees`. We store it separately so we can expose it to code that needs
    to know which replicas are available. */
    watchable_variable_t<std::set<server_id_t> > ready_dispatchees_as_set;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_ */

