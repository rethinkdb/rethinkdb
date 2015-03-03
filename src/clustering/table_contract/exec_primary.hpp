// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXEC_PRIMARY_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXEC_PRIMARY_HPP_

#include "clustering/query_routing/primary_query_server.hpp"
#include "clustering/table_contract/exec.hpp"
#include "containers/counted.hpp"

class broadcaster_t;
class io_backender_t;
class listener_t;

class primary_execution_t :
    public execution_t,
    public home_thread_mixin_t,
    private primary_query_server_t::query_callback_t {
public:
    primary_execution_t(
        const execution_t::context_t *context,
        const region_t &region,
        store_view_t *store,
        perfmon_collection_t *perfmon_collection,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);
    void update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &ack_cb);

private:
    /* `contract_info_t` stores a contract, its ack callback, and a condition variable
    indicating if it's obsolete. The reason this is in a struct is because we sometimes
    need to reason about old contracts, so we may keep multiple versions around. */
    class contract_info_t : public slow_atomic_countable_t<contract_info_t> {
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
    when we receive queries over the network. Warning: They are run on the store's home
    thread, which is not necessarily our home thread. */
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

    /* `update_contract()` spawns `update_contract_on_store_thread()` to deliver the new
    contract to `store->home_thread()`. Its has two jobs:
    1. It sets `latest_contract_store_thread` to the new contract
    2. If the broadcaster has been created, it waits until it's safe to ack
        `primary_ready`, and then does so. */
    void update_contract_on_store_thread(
        counted_t<contract_info_t> contract,
        auto_drainer_t::lock_t keepalive,
        new_mutex_in_line_t *mutex_in_line_ptr);

    /* `sync_contract_with_replicas()` blocks until it's safe to ack `primary_ready` for
    the given contract. It does this by sending a sync write to all of the replicas and
    waiting until enough of them respond; this relies on the principle that if a replica
    acknowledges the sync write, it must have also acknowledged every write initiated
    before the sync write. If the first sync write fails, it will try repeatedly until it
    succeeds or is interrupted. */
    void sync_contract_with_replicas(
        counted_t<contract_info_t> contract,
        signal_t *interruptor);

    /* `is_contract_ackable()` is a helper function for `sync_contract_with_replicas()`
    that returns `true` if it's safe to ack `primary_ready` for the given contract, given
    that all the replicas in `servers` have performed the sync write. */
    static bool is_contract_ackable(
        counted_t<contract_info_t> contract,
        const std::set<server_id_t> &servers);

    branch_id_t const our_branch_id;

    /* `latest_contract_*` stores the latest contract we've received, along with its ack
    callback. The `home_thread` version should only be accessed on `this->home_thread()`,
    and the `store_thread` version should only be accessed on `store->home_thread()`. */
    counted_t<contract_info_t> latest_contract_home_thread, latest_contract_store_thread;

    /* `latest_ack` stores the latest contract ack we've sent. */
    boost::optional<contract_ack_t> latest_ack;

    /* `mutex` is used to order calls to `update_contract_on_store_thread()`, so that we
    don't overwrite a newer contract with an older one */
    new_mutex_t mutex;

    /* `branch_registered` is pulsed once the coordinator has committed our branch ID to
    the Raft state. */
    cond_t branch_registered;

    /* `our_broadcaster` stores the pointer to the `broadcaster_t` we constructed. It
    will be `nullptr` until the `broadcaster_t` actually exists. */
    broadcaster_t * our_broadcaster;

    /* `drainer` ensures that `run` is stopped before the other member variables are
    destroyed. */
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXEC_PRIMARY_HPP_ */

