// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_RAFT_PRIMARY_HPP_
#define CLUSTERING_TABLE_RAFT_PRIMARY_HPP_

#include "clustering/immediate_consistency/metadata.hpp"
#include "clustering/query_routing/primary_query_server.hpp"
#include "clustering/table_raft/state.hpp"
#include "containers/counted.hpp"

class broadcaster_t;
class io_backender_t;
class listener_t;

namespace table_raft {

class primary_bcard_t {
public:
    broadcaster_business_card_t broadcaster;
    replier_business_card_t replier;
    peer_id_t peer;
};

class primary_t : private primary_query_server_t::query_callback_t {
public:
    primary_t(
        const server_id_t &sid,
        mailbox_manager_t *const mailbox_manager,
        store_view_t *s,
        branch_history_manager_t *bhm,
        const region_t &r,
        perfmon_collection_t *pms,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb,
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t>
            *primary_bcards,
        watchable_map_var_t<uuid_u, table_query_bcard_t> *table_query_bcards,
        const base_path_t &base_path,
        io_backender_t *io_backender);
    void update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);

private:
    /* `contract_info_t` stores a contract, its ack callback, and a condition variable
    indicating if it's obsolete. The reason this is in a struct is because we sometimes
    need to reason about old contracts, so we may keep multiple versions around. */
    class contract_info_t : public single_threaded_countable_t<contract_info_t> {
    public:
        contract_info_t(
                const contract_t &c,
                const std::function<void(contract_ack_t)> &acb) :
            contract(c), ack_cb(acb) { }
        contract_t contract;
        std::function<void(contract_ack_t)> ack_cb;
        cond_t obsolete;
    };

    /* `ack_counter_t` computes whether a write is guaranteed to be preserved
    even after a failover or other reconfiguration. */
    class ack_counter_t {
    public:
        ack_counter_t(counted_t<contract_info_t> c) :
            contract(c), primary_ack(false), voter_acks(0), temp_voter_acks(0) { }
        void note_ack(const server_id_t &server) {
            primary_ack |= (server == contract->contract.primary->server);
            voter_acks += contract->contract.voters.count(server);
            if (static_cast<bool>(contract->contract.temp_voters)) {
                temp_voter_acks += contract->contract.temp_voters->count(server);
            }
        }
        bool is_safe() const {
            return primary_ack &&
                voter_acks * 2 > contract->contract.voters.size() &&
                (!static_cast<bool>(contract->contract.temp_voters) ||
                    temp_voter_acks * 2 > contract->contract.temp_voters->size());
        }
    private:
        counted_t<contract_info_t> contract;
        bool primary_ack;
        size_t voter_acks, temp_voter_acks;
    };

    /* This is started in a coroutine when the `primary_t` is created. It sets up the
    broadcaster, listener, etc. */
    void run(auto_drainer_t::lock_t keepalive);

    /* These override virtual methods on `master_t::query_callback_t`. They get called
    when we receive queries over the network. */
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

    /* These are helper functions for `update_contract()`. They check when it's safe to
    ack a contract and then ack the contract. */
    static bool is_contract_ackable(
        counted_t<contract_info_t> contract,
        const std::set<server_id_t> &servers);
    void sync_and_ack_contract(
        counted_t<contract_info_t> contract,
        auto_drainer_t::lock_t keepalive);

    server_id_t const server_id;
    mailbox_manager_t *const mailbox_manager;
    store_view_t *const store;
    branch_history_manager_t *const branch_history_manager;
    region_t const region;
    perfmon_collection_t *const perfmons;
    watchable_map_var_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t>
        *const primary_bcards;
    watchable_map_var_t<uuid_u, table_query_bcard_t> *const table_query_bcards;
    base_path_t const base_path;
    io_backender_t *const io_backender;
    branch_id_t const our_branch_id;

    /* `latest_contract` stores the latest contract we've received, along with its ack
    callback. `latest_ack` stores the latest contract ack we've sent. */
    counted_t<contract_info_t> latest_contract;
    boost::optional<contract_ack_t> latest_ack;

    /* `branch_registered` is pulsed once the leader has committed our branch ID to the
    Raft state. */
    cond_t branch_registered;

    /* `our_broadcaster` stores the pointer to the `broadcaster_t` and `master_t` we
    constructed. `run()` pulses it after it finishes constructing them. */
    promise_t<broadcaster_t *> our_broadcaster;

    auto_drainer_t drainer;
};

} /* namespace table_raft */

#endif /* CLUSTERING_TABLE_RAFT_PRIMARY_HPP_ */

