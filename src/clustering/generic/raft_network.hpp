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
        raft_rpc_request_t<state_t>,
        mailbox_t<void(raft_rpc_reply_t)>::address_t
        )> rpc_mailbox_t;

    typename rpc_mailbox_t::address_t rpd;
};

template<class state_t>
class raft_networked_member_t {
public:
    raft_networked_member_t(
        const raft_member_id_t &this_member_id,
        mailbox_manager_t *mailbox_manager,
        watchable_map_t<raft_member_id_t, raft_business_card_t<state_t> > *peers,
        raft_storage_interface_t<state_t> *storage,
        const raft_persistent_state_t<state_t> &persistent_state);

    raft_business_card_t get_business_card();

private:
    /* The `send_rpc()`, `get_connected_members()`, and `write_persistent_state()`
    methods implement the `raft_network_and_storage_interface_t` interface. */
    bool send_rpc(
        const raft_member_id_t &dest,
        const raft_rpc_request_t<state_t> &rpc,
        signal_t *interruptor,
        raft_rpc_reply_t *reply_out);
    clone_ptr_t<watchable_t<std::set<raft_member_id_t> > >
        get_connected_members();

    /* The `on_rpc()` methods are mailbox callbacks. */
    void on_rpc(
        const raft_rpc_reply_t &rpc,
        const mailbox_t<void(raft_rpc_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *mailbox_manager;
    watchable_map_t<raft_member_id_t, raft_business_card_t<state_t> > *peers;

    raft_member_t<state_t> member;

    auto_drainer_t drainer;

    typename raft_business_card_t<state_t>::rpc_mailbox_t rpc_mailbox;
};

#endif   /* CLUSTERING_GENERIC_RAFT_NETWORK_HPP_ */

