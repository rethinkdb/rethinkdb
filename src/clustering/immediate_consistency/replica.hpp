// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REPLICA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REPLICA_HPP_

class replica_t {
public:
    replica_t(
        mailbox_manager_t *mailbox_manager,
        store_view_t *store,
        branch_history_manager_t *bhm,
        const branch_id_t &branch_id,
        state_timestamp_t timestamp);

    replica_bcard_t *get_replica_bcard() {
        return replica_bcard_t {
            synchronize_mailbox.get_address(),
            branch_id,
            backfiller.get_business_card()
            };
    }

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
    void on_synchronize(
        signal_t *interruptor,
        state_timestamp_t timestamp,
        mailbox_t<void()>::address_t);

    mailbox_manager_t *const mailbox_manager;
    store_view_t *const store;
    branch_id_t const branch_id;

    /* A timestamp is completed in `start_enforcer` when the corresponding write has
    acquired a token from the store, and in `end_enforcer` when the corresponding write
    has completed. */
    timestamp_enforcer_t start_enforcer, end_enforcer;

    backfiller_t backfiller;

    replica_bcard_t::synchronize_mailbox_t synchronize_mailbox;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REPLICA_HPP_ */

