// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_MANAGER_HPP_

#include "clustering/table_contract/coordinator.hpp"
#include "clustering/table_contract/executor.hpp"
#include "clustering/table_manager/table_metadata.hpp"

class multi_table_manager_t;

/* `table_manager_t` hosts the `raft_member_t` and the `contract_executor_t`. It also
hosts the `contract_coordinator_t` if we are the Raft leader. */
class table_manager_t :
    private raft_storage_interface_t<table_raft_state_t>
{
public:
    table_manager_t(
        multi_table_manager_t *_parent,
        const namespace_id_t &_table_id,
        const multi_table_manager_bcard_t::timestamp_t::epoch_t &_epoch,
        const raft_member_id_t &member_id,
        const raft_persistent_state_t<table_raft_state_t> &initial_state,
        multistore_ptr_t *multistore_ptr);

    /* These are public so that `multi_table_manager_t` can see them */
    const namespace_id_t table_id;
    const multi_table_manager_bcard_t::timestamp_t::epoch_t epoch;
    const raft_member_id_t member_id;

    raft_member_t<table_raft_state_t> *get_raft() {
        return raft.get_raft();
    }

private:
    /* `leader_t` hosts the `contract_coordinator_t`. */
    class leader_t {
    public:
        leader_t(table_manager_t *_parent);
        ~leader_t();

    private:
        void on_set_config(
            signal_t *interruptor,
            const table_config_and_shards_t &new_config_and_shards,
            const mailbox_t<void(
                boost::optional<multi_table_manager_bcard_t::timestamp_t>
                )>::address_t &reply_addr);

        table_manager_t * const parent;
        minidir_read_manager_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>
            contract_ack_read_manager;
        contract_coordinator_t coordinator;
        table_manager_bcard_t::leader_bcard_t::set_config_mailbox_t set_config_mailbox;
    };

    /* This is a `raft_storage_interface_t` method that the `raft_member_t` calls to
    write its state to disk. */
    void write_persistent_state(
        const raft_persistent_state_t<table_raft_state_t> &persistent_state,
        signal_t *interruptor);

    /* This is the callback for `table_directory_subs`. It's responsible for
    maintaining `raft_directory`, `execution_bcard_minidir_directory`, and
    `contract_ack_minidir_directory`. */
    void on_table_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_manager_bcard_t *bcard);

    /* This is the callback for `raft_committed_subs` */
    void on_raft_committed_change();

    /* This is the callback for `raft_readiness_subs` */
    void on_raft_readiness_change();

    multi_table_manager_t * const parent;

    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_membership;

    /* `active_table_t` monitors `table_manager_directory` and derives three other
    `watchable_map_t`s from it:
    - `raft_directory` contains all of the Raft business cards. It is consumed by
        `raft`, which uses it for Raft internal RPCs.
    - `execution_bcard_minidir_directory` contains business cards for the minidirs
        that carry the execution bcards between servers. It is consumed by
        `execution_bcard_write_manager`.
    - `contract_ack_minidir_directory` contains business cards for the minidirs that
        carry the contract acks between servers. It is consumed by
        `contract_ack_write_manager`.
    */
    watchable_map_keyed_var_t<
            peer_id_t,
            raft_member_id_t,
            raft_business_card_t<table_raft_state_t> >
        raft_directory;
    watchable_map_keyed_var_t<
            peer_id_t,
            server_id_t,
            minidir_bcard_t<std::pair<server_id_t, branch_id_t>,
                contract_execution_bcard_t> >
        execution_bcard_minidir_directory;
    watchable_map_keyed_var_t<
            peer_id_t,
            uuid_u,
            minidir_bcard_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> >
        contract_ack_minidir_directory;

    raft_networked_member_t<table_raft_state_t> raft;

    /* `bcard_entry` creates our entry in the directory. It will always be non-empty;
    it's in a `scoped_ptr_t` to make it more convenient to create. */
    scoped_ptr_t<watchable_map_var_t<namespace_id_t, table_manager_bcard_t>::entry_t>
        bcard_entry;

    /* `leader` will be non-empty if we are the Raft leader */
    scoped_ptr_t<leader_t> leader;

    /* `leader_mutex` controls access to `leader`. This is important because creating
    and destroying the `leader_t` may block. */
    new_mutex_t leader_mutex;

    /* The `execution_bcard_read_manager` receives `contract_execution_bcard_t`s from
    `contract_executor_t`s on other servers and passes them to the
    `contract_executor_t` on this server. */
    minidir_read_manager_t<std::pair<server_id_t, branch_id_t>,
        contract_execution_bcard_t> execution_bcard_read_manager;

    /* The `contract_executor_t` creates and destroys `broadcaster_t`s,
    `listener_t`s, etc. to handle queries. */
    contract_executor_t contract_executor;

    /* The `execution_bcard_write_manager` receives `contract_execution_bcard_t`s
    from the `contract_executor` on this server and passes them to the
    `contract_executor_t`s on other servers. */
    minidir_write_manager_t<std::pair<server_id_t, branch_id_t>,
        contract_execution_bcard_t> execution_bcard_write_manager;

    /* The `contract_ack_write_manager` receives `contract_ack_t`s from the
    `contract_executor` and passes them to the `contract_coordinator_t` on the Raft
    leader. */
    minidir_write_manager_t<std::pair<server_id_t, contract_id_t>,
        contract_ack_t> contract_ack_write_manager;

    /* The `table_query_bcard_source` receives `table_query_bcard_t`s from the
    `contract_executor` and sends them on to `table_query_bcard_combiner` on the
    parent. From there they will be sent to other servers' `namespace_repo_t`s. */
    watchable_map_combiner_t<namespace_id_t, uuid_u, table_query_bcard_t>::source_t
        table_query_bcard_source;

    auto_drainer_t drainer;

    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
        ::all_subs_t table_directory_subs;
    watchable_subscription_t<raft_member_t<table_raft_state_t>::state_and_config_t>
        raft_committed_subs;
    watchable_subscription_t<bool> raft_readiness_subs;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_MANAGER_HPP_ */

