// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_TABLE_DRIVER_HPP_
#define CLUSTERING_REACTOR_TABLE_DRIVER_HPP_

class table_epoch_t {
public:
    /* Every table's lifetime is divided into "epochs". Each epoch corresponds to one
    Raft instance. Normally tables only have one epoch; a new epoch is created only when
    the user manually overrides the Raft state, which requires creating a new Raft
    instance.

    `timestamp` is the wall-clock time when the epoch began. `id` is a unique ID created
    for the epoch. An epoch with a later `timestamp` supersedes an epoch with an earlier
    `timestamp`. `id` breaks ties. Ties are possible because the user may manually
    override the Raft state on both sides of a netsplit, for example. */
    microtime_t timestamp;
    uuid_u id;

    bool supersedes(const table_epoch_t &other) {
        if (timestamp > other.timestamp) {
            return true;
        } else if (timestamp < other.timestamp) {
            return false;
        } else {
            return id > other.id;
        }
    }
};

/* All table control actions (creating and dropping tables, adding and removing servers
from a table, overriding a table's config) are associated with a
`table_driver_timestamp_t`. These are used to keep track of which actions supersede which
other actions. */
class table_driver_timestamp_t {
public:
    table_epoch_t epoch;

    /* Within each epoch, Raft log indices provide a monotonically increasing clock. */
    raft_log_index_t log_index;

    bool supersedes(const table_driver_timestamp_t &other) {
        if (epoch.supersedes(other.epoch)) {
            return true;
        } else if (other.epoch.supersedes(epoch)) {
            return false;
        }
        return log_index > other.log_index;
    }

    /* `make_drop()` returns a timestamp that supersedes all regular timestamps. */
    static table_driver_timestamp_t make_drop_timestamp() {
        table_driver_timestamp_t t;
        t.epoch.timestamp = std::numeric_limits<microtime_t>::max();
        t.epoch.id = nil_uuid();
        t.log_index = 0;
        return t;
    }
};

/* Control actions are coordinated by sending messages to the `control` mailbox on a
server's `table_driver_business_card_t`. When the user creates or drops a table, or
manually overrides its state, then the parser will send control messages to the other
servers. In addition, if a server sees that another server's state is out of date, it
will send a control message to bring the other server up to date.

Examples:

  * When the user creates a table, the parsing server generates a UUID for the new table;
    an initial epoch ID; an initial configuration for the table; and a `raft_member_id_t`
    for each server in the table's configuration. Then it sends each of those servers a
    control message with `table_id` set to the new table's UUID; `timestamp` set to the
    newly-generated epoch; `is_deletion` set to `false`; `member_id` set to the new
    `raft_member_id_t` for that server; and `initial_state` set to the newly-generated
    table configuration.

  * When the user drops a table, the parsing server sends a control message to every
    visible member of the table's Raft cluster. `table_id` is the table being deleted;
    `timestamp` is `table_driver_timestamp_t::make_drop_timestamp()`; `is_deletion` is
    `true`; and `member_id` and `initial_state` are empty.

  * When the user adds a server to the table's configuration, the parsing server will
    initiate a regular Raft transaction (not a control message) to add the new server to
    the Raft state and generate a `raft_member_id_t` for it. After the transaction
    completes, one of the members in the Raft cluster will see that the new server is
    present in the Raft configuration, but not acting as a cluster member. So that member
    will send a control message to the new server with the new server's
    `raft_member_id_t`, the current epoch ID, and the new timestamp.

  * When the user removes a server from the table's configuration, the same process
    happens; after the Raft cluster commits the removal transaction, one of the remaining
    members sends it a message with `member_id` and `initial_state` empty, indicating
    it's no longer a member of the cluster.

  * In the table-creation scenario, suppose that two of the three servers for the table
    dropped offline as the table was being created, so they didn't get the messages. When
    they come back online, the server that did get the message will forward the message
    to the remaining servers, so the table will finish being created.

  * Similarly, if a server sees that a table was dropped, but it sees another server
    acting as though the table still exists, it will forward the drop message to that
    server.
*/

class table_driver_business_card_t {
public:
    typedef mailbox_t<void(
        namespace_id_t table_id,
        table_driver_timestamp_t timestamp,
        bool is_deletion,
        boost::optional<raft_member_id_t> member_id,
        boost::optional<raft_persistent_state_t<table_raft_state_t> > initial_state
        )> control_mailbox_t;

    control_mailbox_t::address_t control;
};

class table_persistent_state_t {
public:
    table_epoch_t epoch;
    raft_member_id_t member_id;
    raft_persistent_state_t<table_raft_state_t> raft_state;
};

class table_business_card_t {
public:
    /* The parsing server uses this mailbox to read the current table configuration from
    the Raft cluster. */
    typedef mailbox_t<void(
        mailbox_t<void(table_config_t)>::address_t
        )> get_config_mailbox_t;
    get_config_mailbox_t::address_t get_config;

    /* This mailbox is for writing the Raft cluster state. Only the Raft leader will
    expose this mailbox; the others will have `set_config` empty. */
    typedef mailbox_t<void(
        table_config_t,
        mailbox_t<void(bool)>::address_t
        )> set_config_mailbox_t;
    boost::optional<set_config_mailbox_t::address_t> set_config;

    /* This is exposed so that other servers can check if they need to send a control
    message to bring this server into a newer epoch */
    table_epoch_t epoch;

    /* The other members of the Raft cluster send Raft RPCs through
    `raft_business_card`. */
    raft_member_id_t raft_member_id;
    raft_business_card_t<table_raft_state_t> raft_business_card;
};

class table_driver_t {
public:
    table_driver_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_driver_business_card_t>
            *_table_driver_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_business_card_t>
            *_table_directory,
        persistent_file_t<table_driver_persistent_state_t> *_persistent_file);

    table_driver_business_card_t get_table_driver_business_card();
    watchable_map_t<namespace_id_t, table_business_card_t> get_table_business_cards() {
        return &table_business_cards;
    }

private:
    class table_t {
    public:
        uuid_u epoch;
        raft_member_id_t member_id;
        watchable_map_keyed_var_t<peer_id_t,
            raft_member_id_t, raft_business_card_t<state_t> > peers;
        raft_networked_member_t<table_raft_state_t> member;
    };

    void on_table_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_business_card_t &value);

    void on_control(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_driver_meta_state_t &new_meta_state,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_raft_state);

    void maybe_send_control(
        const namespace_id_t &table_id,
        const peer_id_t &peer_id);

    mailbox_manager_t *mailbox_manager;
    watchable_map_t<peer_id_t, table_driver_business_card_t> *table_driver_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_business_card_t>
        *table_directory;
    persistent_file_t<table_driver_persistent_state_t> *persistent_file;

    table_driver_persistent_state_t persistent_state;

    watchable_map_var_t<namespace_id_t, table_business_card_t> table_business_cards;

    std::map<namespace_id_t, scoped_ptr_t<table_t> > tables;
    std::map<namespace_id_t, table_raft_meta_state_t> meta_states;

    table_driver_business_card_t::control_mailbox_t control_mailbox;
};

#endif /* CLUSTERING_REACTOR_TABLE_DRIVER_HPP_ */

