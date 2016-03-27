// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_CONTRACT_EXECUTOR_EXEC_PRIMARY_HPP_
#define CLUSTERING_TABLE_CONTRACT_EXECUTOR_EXEC_PRIMARY_HPP_

#include "clustering/query_routing/primary_query_server.hpp"
#include "clustering/table_contract/executor/exec.hpp"
#include "containers/counted.hpp"

class io_backender_t;
class primary_dispatcher_t;

/* `ack_counter_t` computes whether a write is guaranteed to be preserved
even after a failover or other reconfiguration. */
class ack_counter_t {
public:
    explicit ack_counter_t(const contract_t &_contract) :
        contract(_contract), primary_ack(false), voter_acks(0), temp_voter_acks(0) { }
    void note_ack(const server_id_t &server) {
        if (static_cast<bool>(contract.primary)) {
            primary_ack |= (server == contract.primary->server);
        }
        voter_acks += contract.voters.count(server);
        if (static_cast<bool>(contract.temp_voters)) {
            temp_voter_acks += contract.temp_voters->count(server);
        }
    }
    bool is_safe() const {
        return primary_ack &&
            voter_acks * 2 > contract.voters.size() &&
            (!static_cast<bool>(contract.temp_voters) ||
                temp_voter_acks * 2 > contract.temp_voters->size());
    }
private:
    const contract_t &contract;
    bool primary_ack;
    size_t voter_acks, temp_voter_acks;

    DISABLE_COPYING(ack_counter_t);
};

class primary_execution_t :
    public execution_t,
    public home_thread_mixin_t,
    private primary_query_server_t::query_callback_t {
public:
    primary_execution_t(
        const execution_t::context_t *context,
        execution_t::params_t *params,
        const contract_id_t &cid,
        const table_raft_state_t &raft_state);
    ~primary_execution_t();

    void update_contract_or_raft_state(
        const contract_id_t &cid,
        const table_raft_state_t &raft_state);

private:
    class write_callback_t;
    /* `contract_info_t` stores a contract, its ack callback, and a condition variable
    indicating if it's obsolete. The reason this is in a struct is because we sometimes
    need to reason about old contracts, so we may keep multiple versions around. */
    class contract_info_t : public slow_atomic_countable_t<contract_info_t> {
    public:
        contract_info_t(const contract_id_t &_contract_id,
                        const contract_t &_contract,
                        write_durability_t _default_write_durability,
                        write_ack_config_t _write_ack_config) :
                contract_id(_contract_id),
                contract(_contract),
                default_write_durability(_default_write_durability),
                write_ack_config(_write_ack_config) {
        }
        bool equivalent(const contract_info_t &other) const {
            /* This method is called `equivalent` rather than `operator==` to avoid
            confusion, because it doesn't actually compare every member */
            return contract_id == other.contract_id &&
                default_write_durability == other.default_write_durability &&
                write_ack_config == other.write_ack_config;
        }
        contract_id_t contract_id;
        contract_t contract;
        write_durability_t default_write_durability;
        write_ack_config_t write_ack_config;
        cond_t obsolete;
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
        admin_err_t *error_out);
    bool on_read(
        const read_t &request,
        fifo_enforcer_sink_t::exit_read_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out,
        admin_err_t *error_out);

    /* `sync_majority()` is used after a read in 'majority' mode, and will perform a
    `sync` operation across a majority of replicas to make sure what has just been read
    has been committed to disk on a majority of replicas for each shard. */
    bool sync_committed_read(const read_t &read_request,
                             order_token_t order_token,
                             signal_t *interruptor,
                             admin_err_t *error_out);

    /* `update_contract_or_raft_state()` spawns `update_contract_on_store_thread()`
    to deliver the new contract to `store->home_thread()`. It has two jobs:
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

    /* `is_majority_available()` is a helper function to determine whether a majority
    of the replicas are available to acknowledge a read or write. */
    static bool is_majority_available(
        counted_t<contract_info_t> contract,
        primary_dispatcher_t *dispatcher);

    boost::optional<branch_id_t> our_branch_id;

    /* `latest_contract_*` stores the latest contract we've received, along with its ack
    callback. The `home_thread` version should only be accessed on `this->home_thread()`,
    and the `store_thread` version should only be accessed on `store->home_thread()`. */
    counted_t<contract_info_t> latest_contract_home_thread, latest_contract_store_thread;

    /* `latest_ack` stores the latest contract ack we've sent. */
    boost::optional<contract_ack_t> latest_ack;

    /* `update_contract_mutex` is used to order calls to
    `update_contract_on_store_thread()`, so that we don't overwrite a newer contract with
    an older one */
    new_mutex_t update_contract_mutex;

    /* `branch_registered` is pulsed once the coordinator has committed our branch ID to
    the Raft state. */
    cond_t branch_registered;

    /* `our_dispatcher` stores the pointer to the `primary_dispatcher_t` we constructed.
    It will be `nullptr` until the `primary_dispatcher_t` actually exists and has a valid
    replica. */
    primary_dispatcher_t *our_dispatcher;

    /* Anything that might try to use `our_dispatcher` after the `primary_execution_t`
    destructor has begin should hold a lock on `*our_dispatcher_drainer`. (Specifically,
    this applies to `update_contract_on_store_thread()`.) `our_dispatcher` and
    `our_dispatcher_drainer` will always both be null or both be non-null. */
    auto_drainer_t *our_dispatcher_drainer;

    /* `begin_write_mutex_assertion` is used to ensure that we don't ack a contract until
    all past and ongoing writes are safe under the contract's conditions. */
    mutex_assertion_t begin_write_mutex_assertion;

    /* `drainer` ensures that `run()` and `update_contract_on_store_thread()` are
    stopped before the other member variables are destroyed. */
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_TABLE_CONTRACT_EXECUTOR_EXEC_PRIMARY_HPP_ */

