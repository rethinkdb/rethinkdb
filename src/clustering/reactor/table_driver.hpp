// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_TABLE_DRIVER_HPP_
#define CLUSTERING_REACTOR_TABLE_DRIVER_HPP_

class table_driver_business_card_t {
public:
    typedef mailbox_t<void(
        namespace_id_t,
        table_raft_instance_id_t,
        table_raft_state_t,
        raft_config_t
        )> force_mailbox_t;

    typedef mailbox_t<void(
        namespace_id_t,
        table_raft_instance_id_t
        )> recruit_mailbox_t;

    force_mailbox_t::address_t force;
    recruit_mailbox_t::address_t recruit;
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

    table_raft_instance_id_t instance;
    bool is_leader;
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
            *_table_directory) :
        mailbox_manager(_mailbox_manager),
        table_driver_directory(_table_driver_directory),
        table_directory(_table_directory)
        { }

    table_driver_business_card_t get_table_driver_business_card();
    watchable_map_t<namespace_id_t, table_business_card_t> get_table_business_cards() {
        return &table_business_cards;
    }

private:
    class table_t {
    public:
        table_t(
            table_raft_instance_id_t instance_id,
            raft_persistent_state_t<table_raft_state_t> initial_state);
        table_raft_instance_id_t instance_id;
        raft_networked_member_t<table_raft_state_t> member;
    };

    void on_force(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_raft_instance_id_t &instance_id,
        const table_raft_state_t &new_state,
        const raft_config_t &new_config);

    void on_recruit(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_raft_instance_id_t &instance_id);

    mailbox_manager_t *mailbox_manager;
    watchable_map_t<peer_id_t, table_driver_business_card_t> *table_driver_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_business_card_t>
        *table_directory;

    watchable_map_var_t<namespace_id_t, table_business_card_t> table_business_cards;

    std::map<namespace_id_t, scoped_ptr_t<table_t> > tables;

    table_driver_business_card_t::force_mailbox_t force_mailbox;
    table_driver_business_card_t::recruit_mailbox_t recruit_mailbox;
};

#endif /* CLUSTERING_REACTOR_TABLE_DRIVER_HPP_ */

