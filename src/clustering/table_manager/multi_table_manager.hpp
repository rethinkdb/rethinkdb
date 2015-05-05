// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_MULTI_TABLE_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_MULTI_TABLE_MANAGER_HPP_

#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/table_contract/coordinator.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "clustering/table_contract/executor.hpp"
#include "clustering/table_manager/table_manager.hpp"
#include "clustering/table_manager/table_metadata.hpp"

/* There is one `multi_table_manager_t` on each server. For tables hosted on this server,
it handles administrative operations: table creation and deletion, adding and removing
this server from the table, and changing the table configuration. It's also responsible
for constructing the `contract_coordinator_t` and `contract_executor_t`, which carry out
the actual business of handling queries.

The `multi_table_manager_t` exposes an "action mailbox", a "get config mailbox", and a
"set config mailbox" via the directory. See `table_metadata.hpp` for the type signatures
of these mailboxes. The `table_meta_client_t` is responsible for translating client
actions into messages to the `multi_table_manager_t`'s mailboxes. However, the
`multi_table_manager_t` will also send messages directly to other
`multi_table_manager_t`s' mailboxes sometimes.

Here are some examples of scenarios involving the `multi_table_manager_t`:

* When the user creates a table, the `table_meta_client_t` generates a UUID for the new
table; an initial epoch ID; an initial configuration for the table; and a
`raft_member_id_t` for each server in the table's configuration. Then it sends a message
to each server's action mailbox with `table_id` set to the new table's UUID; `timestamp`
set to the newly-generated epoch; `status` set to `ACTIVE`; `basic_config` is empty;
`raft_member_id` set to the new `raft_member_id_t` for that server; and
`initial_raft_state` set to the newly-generated table configuration.

* When the user drops a table, the `table_meta_client_t` sends a message to the action
mailbox of every visible member of the table's Raft cluster. `table_id` is the table
being deleted; `timestamp` is set to the far future; `status` is `DELETED`; and
`basic_config`, `member_id`, and `initial_state` are empty.

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
members sends a message to its action mailbox with `status` set to `INACTIVE`, indicating
that it's no longer a member of the cluster.

* In the table-creation scenario, suppose that two of the three servers for the table
dropped offline as the table was being created, so they didn't get the messages. When
they come back online, the server that did get the message will forward the message to
the remaining servers' action mailboxes, so the table will finish being created.

* Similarly, if a server sees that a table was dropped, but it sees another server acting
as though the table still exists, it will forward the drop message to that server's
action mailbox. */

class multi_table_manager_t {
public:
    multi_table_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        table_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        perfmon_collection_repo_t *_perfmon_collection_repo);

    /* This constructor is used on proxy servers. */
    multi_table_manager_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory);

    multi_table_manager_bcard_t get_multi_table_manager_bcard() {
        multi_table_manager_bcard_t bcard;
        bcard.action_mailbox = action_mailbox.get_address();
        bcard.get_config_mailbox = get_config_mailbox.get_address();
        bcard.server_id = server_id;
        return bcard;
    }

    watchable_map_t<namespace_id_t, table_manager_bcard_t> *
            get_table_manager_bcards() {
        return &table_manager_bcards;
    }

    watchable_map_t<std::pair<namespace_id_t, uuid_u>, table_query_bcard_t> *
            get_table_query_bcards() {
        return &table_query_bcard_combiner;
    }

    watchable_map_t<namespace_id_t,
            std::pair<table_basic_config_t, multi_table_manager_bcard_t::timestamp_t> >
                *get_table_basic_configs() {
        return &table_basic_configs;
    }

private:
    /* The `multi_table_manager_t` has four possible states with respect to a table:

    1. We've never heard of the table. In this case, we don't have any records for the
        table.

    2. We've heard of the table, but we don't consider ourselves to be a replica for the
        table. In this case, we store a `table_t` in our `tables` map with status
        `INACTIVE`. We record the table's name and database in the metadata but we don't
        have a data file for the table.

    3. We are currently a replica for the table, In this case, we store a `table_t` in
        our `tables` map with status `ACTIVE`. We store a record for the table in the
        on-disk metadata; we also have a data file on disk for the table; and we have an
        `active_table_t`, which takes care of actually storing the data for the table. We
        also broadcast a business card for the table.

    4. We believe the table has been deleted. In this case, we store a `table_t` in our
        `tables` map with status `DELETED`. We don't record anything on disk for the
        table. This means that if we restart, we'll forget that it ever existed and
        return to the first state in this list.
    */

    class table_t;

    class active_table_t {
    public:
        active_table_t(
            multi_table_manager_t *parent,
            table_t *table,
            const namespace_id_t &table_id,
            const multi_table_manager_bcard_t::timestamp_t::epoch_t &epoch,
            const raft_member_id_t &member_id,
            const raft_persistent_state_t<table_raft_state_t> &initial_state,
            multistore_ptr_t *multistore_ptr,
            perfmon_collection_repo_t::collections_t *perfmon_collections);

        raft_member_t<table_raft_state_t> *get_raft() {
            return manager.get_raft();
        }

        void on_raft_commit();
        void update_basic_configs_entry();

        multi_table_manager_t *const parent;
        table_t *const table;
        namespace_id_t const table_id;

        /* The `table_manager_t` is the guts of the `active_table_t`. The
        `active_table_t` is just a thin wrapper that handles forwarding the bcards from
        the `table_manager_t` into the `watchable_map_t`s, and also sending updates to
        other `multi_table_manager_t`s. */
        table_manager_t manager;

        /* The `watchable_map_entry_copier_t` copies the `table_manager_bcard_t` from
        `manager.get_table_manager_bcard()` into the `table_manager_bcards` map. */
        watchable_map_entry_copier_t<namespace_id_t, table_manager_bcard_t>
            table_manager_bcard_copier;

        /* The `table_query_bcard_source` receives `table_query_bcard_t`s from the
        `table_manager_t` and sends them on to `table_query_bcard_combiner`. From there
        they will be sent to other servers' `namespace_repo_t`s. */
        watchable_map_combiner_t<namespace_id_t, uuid_u, table_query_bcard_t>::source_t
            table_query_bcard_source;

        watchable_subscription_t<raft_member_t<table_raft_state_t>::state_and_config_t>
            raft_committed_subs;
    };

    /* `table_t`'s main job is to hold the `active_table_t`. If there is no
    `active_table_t`, this means that we used to be a replica for this table but no
    longer are; in this case, `table_t`'s job is to remember why we are no longer a
    replica for this table. */
    class table_t {
    public:
        enum class status_t { ACTIVE, INACTIVE, DELETED };

        table_t() : sync_coro_running(false) { }

        /* `to_sync_set` holds the server IDs and peer IDs of all the servers for which
        we should call `do_sync()` with this table. `schedule_sync()` adds entries to
        `to_sync_set` and sometimes also spawns a coroutine to call `do_sync()`; it uses
        `sync_coro_running` to make sure it doesn't spawn redundant coroutines. */
        std::set<peer_id_t> to_sync_set;
        bool sync_coro_running;

        /* You must hold this mutex to access the other fields of the `table_t` except
        for `to_sync_set` and `sync_coro_running` */
        new_mutex_t mutex;

        status_t status;

        /* If `status` is either `ACTIVE` or `INACTIVE`, `basic_configs_entry` propagates
        knowledge about the table's basic config to the `table_meta_client_t` via
        `table_basic_configs`. When `status` is `ACTIVE`, it's kept up-to-date by the
        `active_table_t`. (Note that the `active_table_t` may change the value without
        holding `mutex`.) When `status` is `INACTIVE`, it's updated when we receive
        messages from other servers. */
        object_buffer_t<watchable_map_var_t<namespace_id_t,
                std::pair<table_basic_config_t, multi_table_manager_bcard_t::timestamp_t>
                >::entry_t>
            basic_configs_entry;

        /* If `status` is `ACTIVE`, `multistore_ptr` contains our files on disk for the
        table, and `active` contains our Raft instance, etc. Otherwise these are empty.
        */
        scoped_ptr_t<multistore_ptr_t> multistore_ptr;
        scoped_ptr_t<active_table_t> active;

        /* Destructor order matters here. We must destroy `active` before
        `multistore_ptr` because `active` uses the multistore. We must destroy `active`
        before `basic_configs_entry` because `active` sometimes manipulates
        `basic_configs_entry` through a `table_t *`. */
    };

    void on_table_manager_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_manager_bcard_t &value);

    /* `on_action()` and `on_get_config()` are mailbox callbacks */

    void on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const multi_table_manager_bcard_t::timestamp_t &timestamp,
        multi_table_manager_bcard_t::status_t status,
        const boost::optional<table_basic_config_t> &basic_config,
        const boost::optional<raft_member_id_t> &raft_member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_raft_state,
        const mailbox_t<void()>::address_t &ack_addr);

    void on_get_config(
        signal_t *interruptor,
        const boost::optional<namespace_id_t> &table_id,
        const mailbox_t<void(
            std::map<namespace_id_t, table_config_and_shards_t>
            )>::address_t &reply_addr);

    /* `do_sync()` checks if it is necessary to send an action message to the given
    server regarding the given table, and sends one if so. It is called in the following
    situations:
      - Whenever another server connects, it is called for that server and every table
      - Whenever a server changes its `table_manager_bcard_t` in the directory for a
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
        const boost::optional<table_manager_bcard_t> &table_bcard,
        /* This is the other server's global directory entry. If the other server is not
        connected, don't call `do_sync()`. */
        const multi_table_manager_bcard_t &table_manager_bcard);

    /* `schedule_sync()` arranges for `do_sync()` to eventually be called with the given
    table and server. The caller must hold `multi_table_manager_t::mutex`.
    `schedule_sync()` does not block. */
    void schedule_sync(
        const namespace_id_t &table_id,
        table_t *table,
        const peer_id_t &peer_id);

    /* If we're a proxy server, then `is_proxy_server` will be `true`; `server_id` will
    be `nil_uuid()`; `persistence_interface` will be `nullptr`; `base_path` will be
    empty; and `io_backender` will be `nullptr`. */

    bool is_proxy_server;
    server_id_t server_id;
    mailbox_manager_t * const mailbox_manager;
    watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
        * const multi_table_manager_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
        * const table_manager_directory;
    table_persistence_interface_t *persistence_interface;

    boost::optional<base_path_t> base_path;
    io_backender_t *io_backender;

    perfmon_collection_repo_t *perfmon_collection_repo;

    backfill_throttler_t backfill_throttler;

    /* This collects the `table_basic_config_t` for every non-deleted table in the
    `tables` map, for the benefit of the `table_meta_client_t`. */
    watchable_map_var_t<namespace_id_t,
            std::pair<table_basic_config_t, multi_table_manager_bcard_t::timestamp_t> >
        table_basic_configs;

    /* This map describes the table business cards that we show to other servers via the
    directory. `active_table_t` creates and deletes entries in this map. */
    watchable_map_var_t<namespace_id_t, table_manager_bcard_t>
        table_manager_bcards;

    /* This collects `table_query_bcards_t`s from all of the tables on this server into
    a single `watchable_map_t`. */
    watchable_map_combiner_t<namespace_id_t, uuid_u, table_query_bcard_t>
        table_query_bcard_combiner;

    /* Note: `tables` must be destroyed before `table_manager_bcards` or any of the
    earlier member variables. */
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

    watchable_map_t<peer_id_t, multi_table_manager_bcard_t>::all_subs_t
        multi_table_manager_directory_subs;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
        ::all_subs_t table_manager_directory_subs;

    multi_table_manager_bcard_t::action_mailbox_t action_mailbox;
    multi_table_manager_bcard_t::get_config_mailbox_t get_config_mailbox;
};

#endif /* CLUSTERING_TABLE_MANAGER_MULTI_TABLE_MANAGER_HPP_ */

