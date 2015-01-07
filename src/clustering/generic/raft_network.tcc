// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_NETWORK_TCC_
#define CLUSTERING_GENERIC_RAFT_NETWORK_TCC_

#include "clustering/generic/raft_network.hpp"

template<class state_t>
raft_networked_member_t<state_t>::raft_networked_member_t(
        const raft_member_id_t &this_member_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<raft_member_id_t, raft_business_card_t<state_t> > *_peers,
        raft_storage_interface_t<state_t> *storage,
        const raft_persistent_state_t<state_t> &persistent_state) :
    mailbox_manager(_mailbox_manager),
    peers(_peers),
    peers_map_transformer(peers),
    member(this_member_id, storage, this, persistent_state),
    rpc_mailbox(mailbox_manager,
        std::bind(&raft_networked_member_t::on_rpc, this, ph::_1, ph::_2, ph::_3))
    { }

template<class state_t>
raft_business_card_t<state_t> raft_networked_member_t<state_t>::get_business_card() {
    raft_business_card_t<state_t> bc;
    bc.rpc = rpc_mailbox.get_address();
    return bc;
}

template<class state_t>
bool raft_networked_member_t<state_t>::send_rpc(
        const raft_member_id_t &dest,
        const raft_rpc_request_t<state_t> &request,
        signal_t *interruptor,
        raft_rpc_reply_t *reply_out) {
    /* Find the given member's mailbox address */
    boost::optional<raft_business_card_t<state_t> > bcard = peers->get_key(dest);
    if (!static_cast<bool>(bcard)) {
        /* The member is not connected */
        return false;
    }
    /* Send message and wait for a reply */
    disconnect_watcher_t watcher(mailbox_manager, bcard->rpc.get_peer());
    cond_t got_reply;
    mailbox_t<void(raft_rpc_reply_t)> reply_mailbox(
        mailbox_manager,
        [&](signal_t *, raft_rpc_reply_t &&reply) {
            *reply_out = reply;
            got_reply.pulse();
        });
    send(mailbox_manager, bcard->rpc, request, reply_mailbox.get_address());
    wait_any_t waiter(&watcher, &got_reply);
    wait_interruptible(&waiter, interruptor);
    return got_reply.is_pulsed();
}

template<class state_t>
watchable_map_t<raft_member_id_t, std::nullptr_t> *
        raft_networked_member_t<state_t>::get_connected_members() {
    return &peers_map_transformer;
}

template<class state_t>
void raft_networked_member_t<state_t>::on_rpc(
        signal_t *interruptor,
        const raft_rpc_request_t<state_t> &request,
        const mailbox_t<void(raft_rpc_reply_t)>::address_t &reply_addr) {
    raft_rpc_reply_t reply;
    member.on_rpc(request, interruptor, &reply);
    send(mailbox_manager, reply_addr, reply);
}

#endif   /* CLUSTERING_GENERIC_RAFT_NETWORK_TCC_ */

