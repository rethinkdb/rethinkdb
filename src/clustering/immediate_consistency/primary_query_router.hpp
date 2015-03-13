// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_

class primary_query_router_t : public home_thread_mixin_debug_only_t {
private:
    class incomplete_write_t;
    class incomplete_write_ref_t;

public:
    class replica_t {
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
    };

    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {
    public:
        dispatchee_t(
            primary_query_router_t *parent,
            replica_t *replica,
            const server_id_t &server_id,
            double priority,
            state_timestamp_t *first_timestamp_out);
        ~dispatchee_t();

        void set_readable(bool readable);

    private:
        friend class primary_query_router_t;

        primary_query_router_t *const parent;
        replica_t *const replica;
        server_id_t server_id;
        double priority;

        bool is_readable;

        perfmon_counter_t queue_count;
        perfmon_membership_t queue_count_membership;

        fifo_enforcer_source_t fifo_source;
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
        friend class broadcaster_t;
        /* This is so that if the write callback is destroyed before `on_end()` is
        called, it will get deregistered. */
        incomplete_write_t *write;
    };

    primary_query_router_t(
        perfmon_collection_t *parent_perfmon_collection,
        const region_map_t<version_t> &base_version);
    primary_query_router_t(const primary_query_router_t &) = delete;
    primary_query_router_t &operator=(const primary_query_router_t &) = delete;

    branch_id_t get_branch_id() {
        return branch_id;
    }
    branch_birth_certificate_t get_branch_birth_certificate() {
        return branch_bc;
    }

    void read(
        const read_t &r,
        read_response_t *response,
        fifo_enforcer_sink_t::exit_read_t *lock,
        order_token_t tok,
        signal_t *interruptor)
        THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t);

    /* Unlike `read()`, `spawn_write()` returns as soon as the write has begun and
    replies asynchronously via a callback. It will not block. If the `write_callback_t`
    is destroyed while the write is still in progress, its destructor will automatically
    deregister it so that no segfaults will happen. */
    void spawn_write(
        const write_t &w,
        order_token_t tok,
        write_callback_t *cb);

    clone_ptr_t<watchable_t<std::set<server_id_t> > > get_readable_dispatchees() {
        return readable_dispatchees_as_set.get_watchable();
    }

private:
    /* Reads need to pick a single readable mirror to perform the operation. Writes need
    to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. You must hold `mutex` and pass in
    `proof` of the mutex acquisition. */
    void pick_a_readable_dispatchee(
        dispatchee_t **dispatchee_out,
        mutex_assertion_t::acq_t *proof,
        auto_drainer_t::lock_t *lock_out)
        THROWS_ONLY(cannot_perform_query_exc_t);

    void background_write(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token) THROWS_NOTHING;
    void background_writeread(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token, write_durability_t durability) THROWS_NOTHING;
    void end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING;

    /* This recomputes `readable_dispatchees_as_set()` from `readable_dispatchees()` */
    void refresh_readable_dispatchees_as_set();

    /* This function sanity-checks `incomplete_writes`, `current_timestamp`,
    and `newest_complete_timestamp`. It mostly exists as a form of executable
    documentation. */
    void sanity_check();

    branch_id_t branch_id;
    branch_birth_certificate_t branch_bc;

    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_membership;

    /* If a write has begun, but some mirror might not have completed it yet,
    then it goes in `incomplete_writes`. The idea is that a new mirror that
    connects will use the union of a backfill and `incomplete_writes` as its
    data, and that will guarantee it gets at least one copy of every write.
    See the member function `sanity_check()` for a description of the
    relationship between `incomplete_writes`, `current_timestamp`, and
    `newest_complete_timestamp`. */

    /* `mutex` is held by new writes and reads being created, by writes
    finishing, and by dispatchees joining, leaving, or upgrading. It protects
    `incomplete_writes`, `current_timestamp`, `newest_complete_timestamp`,
    `order_checkpoint`, `dispatchees`, and `readable_dispatchees`. */
    mutex_assertion_t mutex;

    std::list<boost::shared_ptr<incomplete_write_t> > incomplete_writes;
    state_timestamp_t current_timestamp, newest_complete_timestamp;
    order_checkpoint_t order_checkpoint;

    /* Once we ack a write, we must make sure that every read that's initiated
    after that will see the result of the write. We use this timestamp to keep
    track of the most recent acked write and produce `min_timestamp_token_t`s from
    it whenever we send a read to a listener. */
    state_timestamp_t most_recent_acked_write_timestamp;

    std::map<dispatchee_t *, auto_drainer_t::lock_t> dispatchees;
    intrusive_list_t<dispatchee_t> readable_dispatchees;

    /* This is just a set that contains the peer ID of each dispatchee in
    `readable_dispatchees`. We store it separately so we can expose it to code that needs
    to know which replicas are available. */
    watchable_variable_t<std::set<server_id_t> > readable_dispatchees_as_set;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_ */

