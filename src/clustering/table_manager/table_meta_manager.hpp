// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_MANAGER_HPP_

#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/table_manager/table_metadata.hpp"

/* There is one `table_meta_manager_t` on each server. For tables hosted on this server,
it handles administrative operations: table creation and deletion, adding and removing
this server from the table, and changing the table configuration. It's also responsible
for constructing the `table_raft_leader_t` and `table_raft_follower_t`, which carry out
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
table's configuration. Next, the `table_raft_leader_t` will initiate Raft transactions to
add the new server to the Raft configuration and generate a `raft_member_id_t` for it.
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
    table_meta_manager_business_card_t::action_timestamp_t::epoch_t epoch;
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
        watchable_map_t<peer_id_t, table_meta_manager_business_card_t>
            *_table_meta_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
            *_table_meta_directory,
        table_meta_persistence_interface_t *_persistence_interface);

    table_meta_manager_business_card_t get_table_meta_manager_business_card() {
        table_meta_manager_business_card_t bcard;
        bcard.action_mailbox = action_mailbox.get_address();
        bcard.server_id = server_id;
        return bcard;
    }

    watchable_map_t<namespace_id_t, table_meta_business_card_t> *
            get_table_meta_business_cards() {
        return &table_meta_business_cards;
    }

private:
    /* We store a `active_table_t` for every table that we're currently acting as a Raft
    cluster member for. We'll have a file for the table on disk if and only if we have a
    `active_table_t` for that table. */
    class active_table_t :
        private raft_storage_interface_t<table_raft_state_t>
    {
    public:
        active_table_t(
            table_meta_manager_t *_parent,
            const namespace_id_t &_table_id,
            const table_meta_manager_business_card_t::action_timestamp_t::epoch_t
                &_epoch,
            const raft_member_id_t &member_id,
            const raft_persistent_state_t<table_raft_state_t> &initial_state,
            multistore_ptr_t *multistore_ptr);
        ~active_table_t();

        /* This is a `raft_storage_interface_t` method that the `raft_member_t` calls to
        write its state to disk. */
        void write_persistent_state(
            const raft_persistent_state_t<table_raft_state_t> &persistent_state,
            signal_t *interruptor);

        /* This is the callback for `table_directory_subs` */
        void on_table_directory_change(
            const std::pair<peer_id_t, namespace_id_t> &key,
            const table_meta_business_card_t *bcard);

        /* This is the callback for `raft_committed_subs` */
        void on_raft_committed_change();

        /* This is the callback for `raft_readiness_subs` */
        void on_raft_readiness_change();

        table_meta_manager_t * const parent;
        const namespace_id_t table_id;
        const table_meta_manager_business_card_t::action_timestamp_t::epoch_t epoch;
        const raft_member_id_t member_id;

        /* One of `active_table_t`'s jobs is extracting `raft_business_card_t`s from
        `table_meta_business_card_t`s and putting them into a map for the
        `raft_networked_member_t` to use. */
        std::map<peer_id_t, raft_member_id_t> old_peer_member_ids;
        watchable_map_var_t<raft_member_id_t, raft_business_card_t<table_raft_state_t> >
            raft_directory;

        raft_networked_member_t<table_raft_state_t> raft;

        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
            ::all_subs_t table_directory_subs;
        watchable_subscription_t<raft_member_t<table_raft_state_t>::state_and_config_t>
            raft_committed_subs;
        watchable_subscription_t<bool> raft_readiness_subs;
    };

    /* We store a `table_t` for every table that we've ever been a member of since the
    last time the process restarted, even if we're no longer a member or the table has
    been dropped. The `active_table_t` (if any) is stored inside the `table_t`. */
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
        table_meta_manager_business_card_t::action_timestamp_t timestamp;

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
        const table_meta_business_card_t &value);

    /* `on_action()`, `on_get_config()`, and `on_set_config()` are mailbox callbacks */

    void on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_meta_manager_business_card_t::action_timestamp_t &timestamp,
        bool is_deletion,
        const boost::optional<raft_member_id_t> &member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_state,
        const mailbox_t<void()>::address_t &ack_addr);

    void on_get_config(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const mailbox_t<void(boost::optional<table_config_t>)>::address_t &reply_addr);

    void on_set_config(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_config_t &new_config,
        const mailbox_t<void(bool)>::address_t &reply_addr);

    /* `do_sync()` checks if it is necessary to send an action message to the given
    server regarding the given table, and sends one if so. It is called in the following
    situations:
      - Whenever another server connects, it is called for that server and every table
      - Whenever a server changes its `table_meta_business_card_t` in the directory for a
        table, it is called for that server and that table
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
        const boost::optional<table_meta_business_card_t> &table_bcard,
        /* This is the other server's global directory entry. If the other server is not
        connected, don't call `do_sync()`. */
        const table_meta_manager_business_card_t &table_manager_bcard);

    /* `schedule_sync()` arranges for `do_sync()` to eventually be called with the given
    table and server. The caller must hold `table_meta_manager_t::mutex`.
    `schedule_sync()` does not block. */
    void schedule_sync(
        const namespace_id_t &table_id,
        table_t *table,
        const peer_id_t &peer_id);

    const server_id_t server_id;
    mailbox_manager_t * const mailbox_manager;
    watchable_map_t<peer_id_t, table_meta_manager_business_card_t>
        * const table_meta_manager_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
        * const table_meta_directory;
    table_meta_persistence_interface_t * const persistence_interface;

    std::map<namespace_id_t, scoped_ptr_t<table_t> > tables;

    /* You must hold this mutex whenever you:
       - Access the `tables` map
       - Access `table_t::dirty_set` or `table_t::send_actions_coro_running`
       - Get in line for a `table_t::mutex`. (However, once you are in line for the
         `table_t::mutex`, you can release this mutex; and once you own `table_t::mutex`,
         you can freely access the fields of `table_t` that it protects.)
    */
    mutex_assertion_t mutex;

    /* This map describes the table business cards that we show to other servers via the
    directory. `active_table_t` creates and deletes entries in this map. */
    watchable_map_var_t<namespace_id_t, table_meta_business_card_t>
        table_meta_business_cards;

    auto_drainer_t drainer;

    watchable_map_t<peer_id_t, table_meta_manager_business_card_t>::all_subs_t
        table_meta_manager_directory_subs;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
        ::all_subs_t table_meta_directory_subs;

    table_meta_manager_business_card_t::action_mailbox_t action_mailbox;
    table_meta_manager_business_card_t::get_config_mailbox_t get_config_mailbox;
    table_meta_manager_business_card_t::set_config_mailbox_t set_config_mailbox;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_META_MANAGER_HPP_ */

