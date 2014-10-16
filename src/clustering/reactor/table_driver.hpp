// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_TABLE_DRIVER_HPP_
#define CLUSTERING_REACTOR_TABLE_DRIVER_HPP_

/* `table_driver_meta_state_t` describes the state of one server's relationship to one
table, in a high-level sense. It's used to determine when the `table_driver_t` needs to
take an action like spinning up a new Raft cluster member. */
class table_driver_meta_state_t {
public:
    /* `table_is_dropped` is `true` if the table has been dropped. In this case, the
    other variables are irrelevant. */
    bool table_is_dropped;
    /* `epoch` is the UUID of the current epoch for the table, and `epoch_timestamp` is
    the associated timestamp for that epoch. */
    uuid_u epoch;
    microtime_t epoch_timestamp;
    /* If this server is a member (voting or non-voting) for the Raft cluster for this
    table, then `server_is_member` will be `true` and `member_id` will be the server's
    member ID. Otherwise, `server_is_member` will be false. In either case,
    `member_id_log_index` is the index of an entry in the Raft log at which time the
    information in `server_is_member` and `member_id` was accurate. */
    bool server_is_member;
    raft_member_id_t member_id;
    raft_log_index_t member_id_log_index;
};

class table_driver_business_card_t {
public:
    typedef mailbox_t<void(
        namespace_id_t,
        table_driver_meta_state_t,
        boost::optional<raft_persistent_state_t<table_raft_state_t> >
        )> control_mailbox_t;

    control_mailbox_t::address_t control;
};

class table_driver_persistent_state_t {
public:
    class table_state_t {
    public:
        table_driver_meta_state_t meta_state;
        raft_persistent_state_t<table_raft_state_t> raft_state;
    };
    std::map<namespace_id_t, table_state_t> tables;
};

class table_business_card_t {
public:
    typedef mailbox_t<void(
        mailbox_t<void(table_config_t)>::address_t
        )> get_config_mailbox_t;

    typedef mailbox_t<void(
        table_config_t,
        mailbox_t<void()>::address_t
        )> set_config_mailbox_t;

    table_driver_meta_state_t meta_state;

    raft_business_card_t<table_raft_state_t> raft_business_card;

    boost::optional<raft_term_t> term_if_leader;
    get_config_mailbox_t::address_t get_config;
    set_config_mailbox_t::address_t set_config;
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

