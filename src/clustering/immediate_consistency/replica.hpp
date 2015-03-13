// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REPLICA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REPLICA_HPP_

class replica_t {
public:
    replica_t(
        store_view_t *store,
        state_timestamp_t timestamp);

    void do_read(
        const read_t &read,
        min_timestamp_token_t token,
        signal_t *interruptor,
        read_response_t *response_out);

    /* Warning: If you interrupt `do_write()`, the `replica_t` will be left in an
    undefined state, and you should destroy the `replica_t` soon after. */
    void do_write(
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        signal_t *interruptor,
        read_response_t *response_out);

private:
    store_view_t *const store;

    /* A timestamp is completed in `start_enforcer` when the corresponding write has
    acquired a token from the store, and in `end_enforcer` when the corresponding write
    has completed. */
    timestamp_enforcer_t start_enforcer, end_enforcer;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REPLICA_HPP_ */

