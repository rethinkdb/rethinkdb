// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_NETWORK_HPP_
#define CLUSTERING_GENERIC_RAFT_NETWORK_HPP_

#include "clustering/generic/raft_core.hpp"
#include "concurrency/watchable_transform.hpp"
#include "rpc/mailbox/typed.hpp"

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

    typename rpc_mailbox_t::address_t rpc;

    boost::optional<raft_term_t> virtual_heartbeats;

    RDB_MAKE_ME_SERIALIZABLE_2(raft_business_card_t, rpc, virtual_heartbeats);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_2(raft_business_card_t, rpc, virtual_heartbeats);
};

template<class state_t>
class raft_networked_member_t :
    private raft_network_interface_t<state_t> {
public:
    raft_networked_member_t(
        const raft_member_id_t &this_member_id,
        mailbox_manager_t *mailbox_manager,
        watchable_map_t<raft_member_id_t, raft_business_card_t<state_t> > *peers,
        raft_storage_interface_t<state_t> *storage,
        const std::string &log_prefix,
        const raft_start_election_immediately_t start_election_immediately);

    clone_ptr_t<watchable_t<raft_business_card_t<state_t> > > get_business_card() {
        return business_card.get_watchable();
    }

    raft_member_t<state_t> *get_raft() {
        return &member;
    }

private:
    /* The `send_rpc()`, `send_virtual_heartbeats()`, and `get_connected_members()`
    methods implement the `raft_network_interface_t` interface. */
    bool send_rpc(
        const raft_member_id_t &dest,
        const raft_rpc_request_t<state_t> &rpc,
        signal_t *interruptor,
        raft_rpc_reply_t *reply_out);
    void send_virtual_heartbeats(
        const boost::optional<raft_term_t> &term);
    watchable_map_t<raft_member_id_t, boost::optional<raft_term_t> >
        *get_connected_members();

    /* The `on_rpc()` methods are mailbox callbacks. */
    void on_rpc(
        signal_t *interruptor,
        const raft_rpc_request_t<state_t> &rpc,
        const mailbox_t<void(raft_rpc_reply_t)>::address_t &reply_addr);

    mailbox_manager_t *mailbox_manager;
    watchable_map_t<raft_member_id_t, raft_business_card_t<state_t> > *peers;

    /* This transforms the `watchable_map_t` that we got through our constructor into a
    value suitable for returning from `get_connected_members()` */
    watchable_map_value_transform_t<raft_member_id_t,
        raft_business_card_t<state_t>, boost::optional<raft_term_t> >
            peers_map_transformer;

    raft_member_t<state_t> member;

    typename raft_business_card_t<state_t>::rpc_mailbox_t rpc_mailbox;

    watchable_variable_t<raft_business_card_t<state_t> > business_card;
};

#endif   /* CLUSTERING_GENERIC_RAFT_NETWORK_HPP_ */

