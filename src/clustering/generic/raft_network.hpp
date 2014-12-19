// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_NETWORK_HPP_
#define CLUSTERING_GENERIC_RAFT_NETWORK_HPP_

#include "clustering/generic/raft_core.hpp"

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
        const raft_persistent_state_t<state_t> &persistent_state);

    raft_business_card_t get_business_card();

private:
    /* The `send_rpc()`, `get_connected_members()`, and `write_persistent_state()`
    methods implement the `raft_network_interface_t` interface. */
    bool send_rpc(
        const raft_member_id_t &dest,
        const raft_rpc_request_t<state_t> &rpc,
        signal_t *interruptor,
        raft_rpc_reply_t *reply_out);
    watchable_map_t<raft_member_id_t, std::nullptr_t> *get_connected_members();

    /* The `on_rpc()` methods are mailbox callbacks. */
    void on_rpc(
        const raft_rpc_reply_t &rpc,
        const mailbox_t<void(raft_rpc_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *mailbox_manager;
    watchable_map_t<raft_member_id_t, raft_business_card_t<state_t> > *peers;

    /* This transforms the `watchable_map_t` that we got through our constructor into a
    value suitable for returning from `get_connected_members()` */
    class peers_map_transformer_t :
        watchable_map_transform_t<
            raft_member_id_t, raft_business_card_t<state_t>,
            raft_member_id_t, std::nullptr_t>
    {
    private:
        bool key_1_to_2(const raft_member_id_t &key1, raft_member_id_t *key2_out) {
            *key2_out = key1;
            return true;
        }
        void value_1_to_2(const raft_business_card_t<state_t>, std::nullptr_t *) {
            /* Do nothing; the `std::nullptr_t *` must already be set to the correct
            value. */
        }
        bool key_2_to_1(const raft_member_id_t &key2, raft_member_id_t *key1_out) {
            *key1_out = key2;
            return true;
        }
    };
    peers_map_transformer_t peers_map_transformer;

    raft_member_t<state_t> member;

    auto_drainer_t drainer;

    typename raft_business_card_t<state_t>::rpc_mailbox_t rpc_mailbox;
};

#endif   /* CLUSTERING_GENERIC_RAFT_NETWORK_HPP_ */

