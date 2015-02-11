// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_PRIMARY_HPP_
#define CLUSTERING_TABLE_RAFT_PRIMARY_HPP_

namespace table_raft {

class primary_bcard_t {
public:
    broadcaster_business_card_t broadcaster;
    replier_business_card_t replier;
    peer_id_t peer;
};

class primary_t {
public:
    primary_t(
        const server_id_t &sid,
        store_view_t *s,
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t>
            *primary_bcards,
        const region_t &r,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);
    void update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);

private:
    class ack_condition_t :
            public single_threaded_countable_t,
            public ack_checker_t {
    public:
        /* This describes the conditions under which incoming writes are acked to the
        client. If `hand_over` is true, writes are forbidden. Otherwise, writes are acked
        when a majority of `voters` and `temp_voters` (if present) ack them. */
        bool hand_over;
        std::set<server_id_t> voters;
        boost::optional<std::set<server_id_t> > temp_voters;
    };

    void run(auto_drainer_t::lock_t keepalive);
    void send_ack(const contract_ack_t &ca);
    bool on_write(
        const write_t &request,
        fifo_enforcer_sink_t::exit_write_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        write_response_t *response_out,
        std::string *error_out);
    bool on_read(
        const read_t &request,
        fifo_enforcer_sink_t::exit_read_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out,
        std::string *error_out);

    server_id_t const server_id;
    store_view_t *const store;
    watchable_map_var_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t>
        *primary_bcards;
    region_t const region;

    /* `last_ack` stores the contract ack that we most recently sent. `ack_cb` stores the
    callback we can use to ack the most recent contract. */
    boost::optional<contract_ack_t> last_ack;
    std::function<void(const contract_ack_t &)> ack_cb;

    /* `original_branch_id` is the branch ID that was in the contract we were started
    with. We always need to request a new branch ID when we're first started; storing
    `original_branch_id` is how we detect if a new branch ID has been assigned. */
    branch_id_t const original_branch_id;

    /* `our_branch_id` stores the branch ID for the branch we're accepting writes for.
    `update_contract()` pulses it when we are issued a branch ID by the leader. */
    promise_t<branch_id_t> our_branch_id;

    /* `our_broadcaster` stores the pointer to the `broadcaster_t` and `master_t` we
    constructed. `run()` pulses it after it finishes constructing them. */
    promise_t<broadcaster_t *> our_broadcaster;

    /* We update this every time a new contract comes in */
    counted_t<ack_condition_t> ack_condition;

    auto_drainer_t drainer;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_PRIMARY_HPP_ */

