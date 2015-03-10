// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_MANAGER_HPP_

#include "clustering/table_contract/coordinator.hpp"
#include "clustering/table_contract/executor.hpp"
#include "clustering/table_manager/sindex_manager.hpp"
#include "clustering/table_manager/table_metadata.hpp"

/* `table_manager_t` hosts the `raft_member_t` and the `contract_executor_t`. It also
hosts the `contract_coordinator_t` if we are the Raft leader. */
class table_manager_t :
    private raft_storage_interface_t<table_raft_state_t>
{
public:
    table_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        backfill_throttler_t *_backfill_throttler,
        table_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
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

    clone_ptr_t<watchable_t<table_manager_bcard_t> > get_table_manager_bcard() {
        return table_manager_bcard.get_watchable();
    }

    watchable_map_t<uuid_u, table_query_bcard_t> *get_table_query_bcards() {
        return contract_executor.get_local_table_query_bcards();
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

    mailbox_manager_t * const mailbox_manager;
    table_persistence_interface_t * const persistence_interface;

    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_membership;

    /* `table_manager_t` monitors `table_manager_directory` and derives three other
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

    watchable_variable_t<table_manager_bcard_t> table_manager_bcard;

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

    /* The `sindex_manager` watches the `table_config_t` and changes the sindexes on
    `multistore_ptr` according to what it sees. */
    sindex_manager_t sindex_manager;

    auto_drainer_t drainer;

    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
        ::all_subs_t table_directory_subs;
    watchable_subscription_t<raft_member_t<table_raft_state_t>::state_and_config_t>
        raft_committed_subs;
    watchable_subscription_t<bool> raft_readiness_subs;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_MANAGER_HPP_ */

