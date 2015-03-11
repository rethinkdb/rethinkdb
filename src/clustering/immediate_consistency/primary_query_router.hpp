// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_

class primary_query_router_t : public home_thread_mixin_debug_only_t {
private:
    class incomplete_write_t;
    class incomplete_write_ref_t;

public:
    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {
    public:
        virtual server_id_t get_server_id() = 0;
        virtual void read(
            const read_t &read,
            min_timestamp_token_t token,
            signal_t *interruptor,
            read_response_t *response_out) = 0;
        virtual void write(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            write_durability_t durability,
            signal_t *interruptor,
            write_response_t *response_out) = 0;
        virtual void write_noreply(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            signal_t *interruptor) = 0;

    protected:
        dispatchee_t(
            primary_query_router_t *parent,
            const std::string &perfmon_name,
            state_timestamp_t *first_timestamp_out);
        ~dispatchee_t();

        void set_readable(bool readable);

    private:
        friend class primary_query_router_t;
        primary_query_router_t *parent;
        bool is_readable;
        fifo_enforcer_source_t fifo_source;
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
        friend class broadcaster_t;
        /* This is so that if the write callback is destroyed before `on_end()` is
        called, it will get deregistered. */
        incomplete_write_t *write;
    };

    primary_query_router_t(
        const region_map_t<version_t> &base_version,
        branch_id_t *branch_id_out,
        branch_birth_certificate_t *branch_bc_out);

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
    
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_PRIMARY_QUERY_ROUTER_HPP_ */

