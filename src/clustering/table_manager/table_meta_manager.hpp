// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_MANAGER_HPP_

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/table_contract/coordinator.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "clustering/table_contract/executor.hpp"
#include "clustering/table_manager/table_metadata.hpp"

/* There is one `table_meta_manager_t` on each server. For tables hosted on this server,
it handles administrative operations: table creation and deletion, adding and removing
this server from the table, and changing the table configuration. It's also responsible
for constructing the `contract_coordinator_t` and `contract_executor_t`, which carry out
the actual business of handling queries.

The `table_meta_manager_t` exposes an "action mailbox", a "get config mailbox", and a
"set config mailbox" via the directory. See `table_metadata.hpp` for the type signatures
of these mailboxes. The `table_meta_client_t` is responsible for translating client
actions into messages to the `table_meta_manager_t`'s mailboxes. However, the
`table_meta_manager_t` will also send messages directly to other `table_meta_manager_t`s'
mailboxes sometimes.

Here are some examples of scenarios involving the `table_meta_manager_t`:

* When the user creates a table, the `table_meta_client_t` generates a UUID for the new
table; an initial epoch ID; an initial configuration for the table; and a
`raft_member_id_t` for each server in the table's configuration. Then it sends a message
to each server's action mailbox with `table_id` set to the new table's UUID; `timestamp`
set to the newly-generated epoch; `is_deletion` set to `false`; `member_id` set to the
new `raft_member_id_t` for that server; and `initial_state` set to the newly-generated
table configuration.

* When the user drops a table, the `table_meta_client_t` sends a message to the action
mailbox of every visible member of the table's Raft cluster. `table_id` is the table
being deleted; `timestamp` is set to the far future; `is_deletion` is `true`; and
`member_id` and `initial_state` are empty.

* When the user changes the table's configuration without adding or removing any servers
(e.g. changing its name), the `table_meta_client_t` will send a message to the set-config
mailbox of the Raft cluster leader. The Raft cluster leader will then initiate a Raft
transaction to change the table's name.

* When the user adds a server to the table's configuration, the `table_meta_client_t`
will send a message to the set-config mailbox of the Raft cluster leader to update the
table's configuration. Next, the `contract_coordinator_t` will initiate Raft transactions
to add the new server to the Raft configuration and generate a `raft_member_id_t` for it.
After the transaction completes, one of the members in the Raft cluster will see that the
new server is present in the Raft configuration, but not acting as a cluster member. So
that member will send a message to the new server's action mailbox with the new server's
`raft_member_id_t`, the current epoch ID, and the new timestamp.

* When the user removes a server from the table's configuration, the same process
happens; after the Raft cluster commits the removal transaction, one of the remaining
members sends a message to its action mailbox with `member_id` and `initial_state` empty,
indicating it's no longer a member of the cluster.

* In the table-creation scenario, suppose that two of the three servers for the table
dropped offline as the table was being created, so they didn't get the messages. When
they come back online, the server that did get the message will forward the message to
the remaining servers' action mailboxes, so the table will finish being created.

* Similarly, if a server sees that a table was dropped, but it sees another server acting
as though the table still exists, it will forward the drop message to that server's
action mailbox. */

class table_meta_persistent_state_t {
public:
    table_meta_manager_bcard_t::timestamp_t::epoch_t epoch;
    raft_member_id_t member_id;
    raft_persistent_state_t<table_raft_state_t> raft_state;
};
RDB_DECLARE_SERIALIZABLE(table_meta_persistent_state_t);

class table_meta_persistence_interface_t {
public:
    virtual void read_all_tables(
        const std::function<void(
            const namespace_id_t &table_id,
            const table_meta_persistent_state_t &state,
            scoped_ptr_t<multistore_ptr_t> &&multistore_ptr)> &callback,
        signal_t *interruptor) = 0;
    virtual void add_table(
        const namespace_id_t &table,
        const table_meta_persistent_state_t &state,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor) = 0;
    virtual void update_table(
        const namespace_id_t &table,
        const table_meta_persistent_state_t &state,
        signal_t *interruptor) = 0;
    virtual void remove_table(
        const namespace_id_t &table,
        signal_t *interruptor) = 0;
protected:
    virtual ~table_meta_persistence_interface_t() { }
};

class table_meta_manager_t {
public:
    table_meta_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_meta_manager_bcard_t>
            *_table_meta_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_bcard_t>
            *_table_meta_directory,
        table_meta_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender);

    table_meta_manager_bcard_t get_table_meta_manager_bcard() {
        table_meta_manager_bcard_t bcard;
        bcard.action_mailbox = action_mailbox.get_address();
        bcard.get_config_mailbox = get_config_mailbox.get_address();
        bcard.server_id = server_id;
        return bcard;
    }

    watchable_map_t<namespace_id_t, table_meta_bcard_t> *
            get_table_meta_bcards() {
        return &table_meta_bcards;
    }

private:
    /* This is a `watchable_map_transform_t` subclass, a helper class for taking the
    `minidir_bcard_t`s from the `table_meta_directory` and putting them into a format
    that the `minidir_write_manager_t` can handle. */
    class execution_bcard_minidir_bcard_finder_t;

    /* The `table_meta_manager_t` has four possible levels of involvement with a table:

    0. We haven't been a replica of the table since we restarted. In this case, we don't
        store anything for the table whatsoever.

    1. We were a replica for the table at some point since we restarted, but we are not
        currently a replica for the table. In this case, we store a `table_t` in our
        `tables` map. We don't have a file on disk for the table or publish a business
        card for the table.

    2. We are currently a replica for the table, but not the Raft leader. In this case,
        we store a `table_t` in our `tables` map, and we store an `active_table_t` in the
        `table_t`. We have a file on disk for the table and we publish a business card
        for the table.

    3. We are a replica and the Raft leader for the table. This is the same as the above
        case but we also have a `leader_table_t` in the `active_table_t`.

    So `table_t`, `active_table_t`, and `leader_table_t` are nested inside each other;
    having any one of them means having all of the later ones. */

    class table_t;
    class active_table_t;
    class leader_table_t;

    /* `leader_table_t` hosts the `contract_coordinator_t`. */
    class leader_table_t {
    public:
        leader_table_t(
            table_meta_manager_t *_parent,
            active_table_t *_active_table);
        ~leader_table_t();

    private:
        void on_set_config(
            signal_t *interruptor,
            const table_config_and_shards_t &new_config_and_shards,
            const mailbox_t<void(boost::optional<table_meta_manager_bcard_t::timestamp_t>
                )>::address_t &reply_addr);

        table_meta_manager_t * const parent;
        active_table_t * const active_table;
        minidir_read_manager_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>
            contract_ack_read_manager;
        contract_coordinator_t coordinator;
        table_meta_bcard_t::leader_bcard_t::set_config_mailbox_t set_config_mailbox;
    };

    /* `active_table_t` publishes the business card and hosts the `contract_executor_t`.
    It also hosts the `leader_table_t` if there is one. */
    class active_table_t :
        private raft_storage_interface_t<table_raft_state_t>
    {
    public:
        active_table_t(
            table_meta_manager_t *_parent,
            const namespace_id_t &_table_id,
            const table_meta_manager_bcard_t::timestamp_t::epoch_t &_epoch,
            const raft_member_id_t &member_id,
            const raft_persistent_state_t<table_raft_state_t> &initial_state,
            multistore_ptr_t *multistore_ptr);
        ~active_table_t();

        /* These are public so that `table_meta_manager_t` can see them */
        table_meta_manager_t * const parent;
        const namespace_id_t table_id;
        const table_meta_manager_bcard_t::timestamp_t::epoch_t epoch;
        const raft_member_id_t member_id;

        raft_member_t<table_raft_state_t> *get_raft() {
            return raft.get_raft();
        }

        /* `leader_table_t` uses this to set and unset the `leader` field on the bcard */
        void change_bcard_entry(const std::function<bool(table_meta_bcard_t *)> &cb) {
            bcard_entry->change(cb);
        }

    private:
        /* This is a `raft_storage_interface_t` method that the `raft_member_t` calls to
        write its state to disk. */
        void write_persistent_state(
            const raft_persistent_state_t<table_raft_state_t> &persistent_state,
            signal_t *interruptor);

        /* This is the callback for `table_directory_subs` */
        void on_table_directory_change(
            const std::pair<peer_id_t, namespace_id_t> &key,
            const table_meta_bcard_t *bcard);

        /* This is the callback for `raft_committed_subs` */
        void on_raft_committed_change();

        /* This is the callback for `raft_readiness_subs` */
        void on_raft_readiness_change();

        perfmon_collection_t perfmon_collection;
        perfmon_membership_t perfmon_membership;

        /* One of `active_table_t`'s jobs is extracting `raft_business_card_t`s from
        `table_meta_bcard_t`s and putting them into a map for the
        `raft_networked_member_t` to use. */
        std::map<peer_id_t, raft_member_id_t> old_peer_member_ids;
        watchable_map_var_t<raft_member_id_t, raft_business_card_t<table_raft_state_t> >
            raft_directory;

        raft_networked_member_t<table_raft_state_t> raft;

        /* `bcard_entry` creates our entry in the directory. It will always be non-empty;
        it's in a `scoped_ptr_t` to make it more convenient to create. */
        scoped_ptr_t<watchable_map_var_t<namespace_id_t, table_meta_bcard_t>::entry_t>
            bcard_entry;

        /* `leader` will be non-empty if we are the Raft leader */
        scoped_ptr_t<leader_table_t> leader;

        /* `leader_mutex` controls access to `leader`. This is important because creating
        and destroying the `leader_table_t` may block. */
        new_mutex_t leader_mutex;

        /* Every server constructs a `contract_executor_t` for every table it's a replica
        of. There's a lot of support machinery required here. The
        `execution_bcard_*_manager`s pass `contract_execution_bcard_t`s between different
        replicas of the same table, via the `execution_bcard_minidir_bcard` field of the
        `table_meta_bcard_t`. The `execution_minidir_bcard_finder` is responsible for
        finding the `execution_bcard_minidir_bcard`s in a form suitable for passing to
        `execution_bcard_write_manager_t`. */
        minidir_read_manager_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t> execution_bcard_read_manager;
        contract_executor_t contract_executor;
        scoped_ptr_t<execution_bcard_minidir_bcard_finder_t>
            execution_bcard_minidir_bcard_finder;
        minidir_write_manager_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t> execution_bcard_write_manager;

        auto_drainer_t drainer;

        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_bcard_t>
            ::all_subs_t table_directory_subs;
        watchable_subscription_t<raft_member_t<table_raft_state_t>::state_and_config_t>
            raft_committed_subs;
        watchable_subscription_t<bool> raft_readiness_subs;
    };

    /* `table_t`'s main job is to hold the `active_table_t`. If there is no
    `active_table_t`, this means that we used to be a replica for this table but no
    longer are; in this case, `table_t`'s job is to remember why we are no longer a
    replica for this table. */
    class table_t {
    public:
        /* `to_sync_set` holds the server IDs and peer IDs of all the servers for which
        we should call `do_sync()` with this table. `schedule_sync()` adds entries to
        `to_sync_set` and sometimes also spawns a coroutine to call `do_sync()`; it uses
        `sync_coro_running` to make sure it doesn't spawn redundant coroutines. */
        std::set<peer_id_t> to_sync_set;
        bool sync_coro_running;

        /* You must hold this mutex to access the other fields of the `table_t` except
        for `to_sync_set` and `sync_coro_running` */
        new_mutex_t mutex;

        /* `timestamp` is the timestamp of the latest action message we've received for
        this table. */
        table_meta_manager_bcard_t::timestamp_t timestamp;

        /* `is_deleted` is `true` if this table has been dropped. */
        bool is_deleted;

        /* If the table is not dropped and we are supposed to be a member for the table,
        then `multistore_ptr` will contain a reference to the table's files on disk and
        `active` will contain an `active_table_t`. The reason why `multistore_ptr` isn't
        part of `active_table_t` is that sometimes we need to delete and re-create the
        `active_table_t` but we don't want to delete and re-create the `multistore_ptr`.
        */
        scoped_ptr_t<multistore_ptr_t> multistore_ptr;
        scoped_ptr_t<active_table_t> active;
    };

    void on_table_meta_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_meta_bcard_t &value);

    /* `on_action()`, `on_get_config()`, and `on_set_config()` are mailbox callbacks */

    void on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_meta_manager_bcard_t::timestamp_t &timestamp,
        bool is_deletion,
        const boost::optional<raft_member_id_t> &member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_state,
        const mailbox_t<void()>::address_t &ack_addr);

    void on_get_config(
        signal_t *interruptor,
        const boost::optional<namespace_id_t> &table_id,
        const mailbox_t<void(std::map<namespace_id_t, table_config_and_shards_t>)>::
            address_t &reply_addr);

    /* `do_sync()` checks if it is necessary to send an action message to the given
    server regarding the given table, and sends one if so. It is called in the following
    situations:
      - Whenever another server connects, it is called for that server and every table
      - Whenever a server changes its `table_meta_bcard_t` in the directory for a table,
        it is called for that server and that table
      - Whenever a Raft transaction is committed for a table that we are a member of, it
        is called for every server and that table
    This results in a lot of redundant calls. This is OK because every action message has
    a timestamp, which makes them idempotent. */
    void do_sync(
        const namespace_id_t &table_id,
        /* `table` is our record for this table. Caller must own `table.mutex`. */
        const table_t &table,
        const server_id_t &server_id,
        /* This is the other server's directory entry for this table, or an empty
        optional if there is no entry. */
        const boost::optional<table_meta_bcard_t> &table_bcard,
        /* This is the other server's global directory entry. If the other server is not
        connected, don't call `do_sync()`. */
        const table_meta_manager_bcard_t &table_manager_bcard);

    /* `schedule_sync()` arranges for `do_sync()` to eventually be called with the given
    table and server. The caller must hold `table_meta_manager_t::mutex`.
    `schedule_sync()` does not block. */
    void schedule_sync(
        const namespace_id_t &table_id,
        table_t *table,
        const peer_id_t &peer_id);

    const server_id_t server_id;
    mailbox_manager_t * const mailbox_manager;
    watchable_map_t<peer_id_t, table_meta_manager_bcard_t>
        * const table_meta_manager_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_bcard_t>
        * const table_meta_directory;
    table_meta_persistence_interface_t * const persistence_interface;

    const base_path_t base_path;
    io_backender_t * const io_backender;

    backfill_throttler_t backfill_throttler;

    /* This map describes the table business cards that we show to other servers via the
    directory. `active_table_t` creates and deletes entries in this map. */
    watchable_map_var_t<namespace_id_t, table_meta_bcard_t>
        table_meta_bcards;

    /* Note: `tables` must be destroyed before `table_meta_bcards` or any of the const
    member variables. */
    std::map<namespace_id_t, scoped_ptr_t<table_t> > tables;

    /* You must hold this mutex whenever you:
       - Access the `tables` map
       - Access `table_t::dirty_set` or `table_t::send_actions_coro_running`
       - Get in line for a `table_t::mutex`. (However, once you are in line for the
         `table_t::mutex`, you can release this mutex; and once you own `table_t::mutex`,
         you can freely access the fields of `table_t` that it protects.)
    */
    mutex_assertion_t mutex;

    auto_drainer_t drainer;

    watchable_map_t<peer_id_t, table_meta_manager_bcard_t>::all_subs_t
        table_meta_manager_directory_subs;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_bcard_t>
        ::all_subs_t table_meta_directory_subs;

    table_meta_manager_bcard_t::action_mailbox_t action_mailbox;
    table_meta_manager_bcard_t::get_config_mailbox_t get_config_mailbox;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_META_MANAGER_HPP_ */

