// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_NETWORK_HPP_
#define CLUSTERING_GENERIC_RAFT_NETWORK_HPP_

/* This file is for running the Raft protocol using RethinkDB's clustering primitives.
The core logic for the Raft protocol is in `raft_core.hpp`, not here. This just adds a
networking layer over `raft_core.hpp`. */

template<class state_t>
class raft_business_card_t {
public:
    typedef mailbox_t<void(
        raft_request_vote_rpc_t,
        mailbox_t<void(raft_request_vote_reply_t)>::address_t
        )> request_vote_mailbox_t;
    typedef mailbox_t<void(
        raft_install_snapshot_rpc_t<state_t>,
        mailbox_t<void(raft_install_snapshot_reply_t)>::address_t
        )> install_snapshot_mailbox_t;
    typedef mailbox_t<void(
        raft_append_entries_rpc_t<state_t>,
        mailbox_t<void(raft_append_entries_reply_t)>::address_t
        )> append_entries_mailbox_t;

    typename request_vote_mailbox_t::address_t request_vote;
    typename install_snapshot_mailbox_t::address_t install_snapshot;
    typename append_entries_mailbox_t::address_t append_entries;
};

template<class state_t>
class raft_networked_member_t {
public:
    raft_networked_member_t(
        const raft_member_id_t &this_member_id,
        mailbox_manager_t *mailbox_manager,
        clone_ptr_t<watchable_t<std::map<raft_member_id_t, 
        raft_storage_interface_t<state_t> *storage,
        
    /* The `send_*_rpc()`, `get_connected_members()`, and `write_persistent_state()`
    methods implement the `raft_network_and_storage_interface_t` interface. */
    bool send_request_vote_rpc(
        const raft_member_id_t &dest,
        const raft_request_vote_rpc_t &rpc,
        signal_t *interruptor,
        raft_request_vote_reply_t *reply_out);
    bool send_install_snapshot_rpc(
        const raft_member_id_t &dest,
        const raft_install_snapshot_rpc_t<table_raft_state_t, table_raft_change_t> &rpc,
        signal_t *interruptor,
        raft_install_snapshot_reply_t *reply_out);
    bool send_append_entries_rpc(
        const raft_member_id_t &dest,
        const raft_append_entries_rpc_t<table_raft_change_t> &rpc,
        signal_t *interruptor,
        raft_append_entries_reply_t *reply_out);
    clone_ptr_t<watchable_t<std::set<raft_member_id_t> > >
        get_connected_members();
    void write_persistent_state(
        const raft_persistent_state_t<table_raft_state_t, table_raft_change_t>
            &persistent_state,
        signal_t *interruptor);

    /* The `on_*_rpc()` methods are mailbox callbacks. */
    void on_request_vote_rpc(
        const raft_request_vote_rpc_t &rpc,
        const mailbox_t<void(raft_request_vote_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive);
    void on_install_snapshot_rpc(
        const raft_install_snapshot_rpc_t<table_raft_state_t, table_raft_change_t> &rpc,
        const mailbox_t<void(raft_install_snapshot_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive);
    void on_append_entries_rpc(
        const raft_append_entries_rpc_t<table_raft_change_t> &rpc,
        const mailbox_t<void(raft_append_entries_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive);

    const machine_id_t machine_id;
    mailbox_manager_t *mailbox_manager;

    raft_member_t<table_raft_state_t, table_raft_change_t> member;

    auto_drainer_t drainer;

    table_raft_business_card_t::request_vote_mailbox_t request_vote_mailbox;
    table_raft_business_card_t::install_snapshot_mailbox_t install_snapshot_mailbox;
    table_raft_business_card_t::append_entries_mailbox_t append_entries_mailbox;
};

#endif   /* CLUSTERING_GENERIC_RAFT_NETWORK_HPP_ */

