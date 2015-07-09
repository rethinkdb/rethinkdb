// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_LOCAL_REPLICATOR_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_LOCAL_REPLICATOR_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/replica.hpp"
#include "store_view.hpp"

/* `local_replicator_t` receives writes from a `primary_dispatcher_t` on the same server
and applies them directly to a `store_t` on the same server. It doesn't receive a
backfill at startup; instead, it asserts that the store's state is equal to the starting
state of the `primary_dispatcher_t`'s branch, and then sets the metainfo to point at the
starting state of the branch. So it must be created before any writes have happened on
the `primary_dispatcher_t`.

There is one `local_replicator_t` on the primary replica server of each shard.
`primary_execution_t` constructs it. */

class local_replicator_t : public primary_dispatcher_t::dispatchee_t {
public:
    local_replicator_t(
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,
        primary_dispatcher_t *primary,
        store_view_t *store,
        branch_history_manager_t *bhm,
        signal_t *interruptor);

    /* This destructor can block */
    ~local_replicator_t();

    replica_bcard_t get_replica_bcard() {
        return replica.get_replica_bcard();
    }

    bool is_primary() const {
        return true;
    }

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

    void do_dummy_write(
        signal_t *interruptor,
        write_response_t *response_out);

private:
    store_view_t *const store;
    branch_id_t const branch_id;

    replica_t replica;

    scoped_ptr_t<primary_dispatcher_t::dispatchee_registration_t> registration;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_LOCAL_REPLICA_HPP_ */

