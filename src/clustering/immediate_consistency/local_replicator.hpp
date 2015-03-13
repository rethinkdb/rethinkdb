// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_LOCAL_REPLICATOR_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_LOCAL_REPLICATOR_HPP_

class local_replicator_t : public primary_query_router_t::replica_t {
public:
    local_replicator_t(
        const server_id_t &server_id,
        primary_query_router_t *primary,
        store_view_t *store,
        branch_history_manager_t *bhm);

    void do_read(
        const read_t &read,
        state_timestamp_t min_timestamp,
        signal_t *interruptor,
        read_response_t *response_out);

    void do_write_sync(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        signal_t *interruptor,
        write_response_t *response_out);

    void do_write_async(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        signal_t *interruptor);

private:
    void background_write(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        auto_drainer_t::lock_t keepalive);

    store_view_t *const store;
    branch_id_t const branch_id;

    replica_t replica;

    /* Destructor order matters: We need to destroy `dispatchee` before `drainer` because
    `dispatchee` calls `do_write_async()` which acquires `drainer`. We need to destroy
    `drainer` before the other members variables because destroying `drainer` stops
    `background_write()`, which accesses the other members. */

    auto_drainer_t drainer;

    scoped_ptr_t<primary_query_router_t::dispatchee_t> dispatchee;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_LOCAL_REPLICA_HPP_ */

