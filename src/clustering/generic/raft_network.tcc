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
    member(this_member_id, storage, this, persistent_state),
    request_vote_mailbox(mailbox_manager,
        std::bind(&raft_networked_member_t::on_request_vote_rpc, this,
            ph::_1, ph::_2, auto_drainer_t::lock_t(&drainer))),
    install_snapshot_mailbox(mailbox_manager,
        std::bind(&raft_networked_member_t::on_install_snapshot_rpc, this,
            ph::_1, ph::_2, auto_drainer_t::lock_t(&drainer))),
    append_entries_mailbox(mailbox_manager,
        std::bind(&raft_networked_member_t::on_append_entries_rpc, this,
            ph::_1, ph::_2, auto_drainer_t::lock_t(&drainer)))
    { }

template<class state_t>
raft_business_card_t<state_t> raft_networked_member_t<state_t>::get_business_card() {
    raft_business_card_t<state_t> bc;
    bc.request_vote = request_vote_mailbox.get_address();
    bc.install_snapshot = install_snapshot_mailbox.get_address();
    bc.append_entries = append_entries_mailbox.get_address();
}

template<class state_t>
bool raft_networked_member_t<state_t>::send_request_vote_rpc(
        const raft_member_id_t &dest, const raft_request_vote_rpc_t &rpc,
        signal_t *interruptor, raft_request_vote_reply_t *reply_out) {
    send_generic_rpc(dest, &raft_business_card_t<state_t>::request_vote, rpc,
        interruptor, reply_out);
}

template<class state_t>
bool raft_networked_member_t<state_t>::send_install_snapshot_rpc(
        const raft_member_id_t &dest, const raft_install_snapshot_rpc_t &rpc,
        signal_t *interruptor, raft_install_snapshot_reply_t *reply_out) {
    send_generic_rpc(dest, &raft_business_card_t<state_t>::install_snapshot, rpc,
        interruptor, reply_out);
}

template<class state_t>
bool raft_networked_member_t<state_t>::send_append_entries_rpc(
        const raft_member_id_t &dest, const raft_append_entries_rpc_t &rpc,
        signal_t *interruptor, raft_append_entries_reply_t *reply_out) {
    send_generic_rpc(dest, &raft_business_card_t<state_t>::append_entries, rpc,
        interruptor, reply_out);
}

template<class state_t>
template<class rpc_t, class reply_t>
bool raft_networked_member_t<state_t>::send_generic_rpc(
        const raft_member_id_t &dest,
        mailbox_t<void(rpc_t, mailbox_t<void(reply_t)>::address_t)>::address_t
            raft_business_card_t<state_t>::*business_card_field,
        const rpc_t &rpc,
        signal_t *interruptor,
        reply_t *reply_out) {
    /* Find the given member's mailbox address */
    bool ok;
    mailbox_t<void(rpc_t, mailbox_t<void(reply_t)>::address_t)>::address_t addr;
    peers->read_key(dest, [&](const raft_business_card_t *bcard) {
        if (bcard == nullptr) {
            ok = false;
        } else {
            addr = bcard->*business_card_field;
            ok = true;
        }
    });
    if (!ok) {
        /* The member is not connected */
        return false;
    }
    /* Send message and wait for a reply */
    peer_id_t peer = addr.get_peer();
    disconnect_watcher_t watcher(mailbox_manager, peer);
    cond_t got_reply;
    mailbox_t<void(reply_t)> reply_mailbox(
        mailbox_manager,
        [&](reply_t &&reply) {
            *reply_out = std::move(reply);
            got_reply.pulse();
        });
    send(mailbox_manager, addr, rpc, reply_mailbox.get_address());
    wait_any_t waiter(&watcher, &got_reply);
    wait_interruptible(&waiter, interruptor);
    return got_reply.is_pulsed();
}

clone_ptr_t<watchable_t<std::set<raft_member_id_t> > >
        get_connected_members() {
    
}

template<class state_t>
void raft_networked_member_t<state_t>::on_request_vote_rpc(
        const raft_request_vote_rpc_t &rpc,
        const mailbox_t<void(raft_request_vote_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive) {
    raft_request_vote_reply_t reply;
    member.on_request_vote_rpc(rpc, keepalive.get_drain_signal(), &reply);
    send(mailbox_manager, reply_addr, reply);
}

template<class state_t>
void raft_networked_member_t<state_t>::on_install_snapshot_rpc(
        const raft_install_snapshot_rpc_t<state_t> &rpc,
        const mailbox_t<void(raft_install_snapshot_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive) {
    raft_install_snapshot_reply_t reply;
    member.on_install_snapshot_rpc(rpc, keepalive.get_drain_signal(), &reply);
    send(mailbox_manager, reply_addr, reply);
}

template<class state_t>
void raft_networked_member_t<state_t>::on_append_entries_rpc(
        const raft_append_entries_rpc_t<state_t> &rpc,
        const mailbox_t<void(raft_append_entries_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive) {
    raft_append_entries_reply_t reply;
    member.on_append_entries_rpc(rpc, keepalive.get_drain_signal(), &reply);
    send(mailbox_manager, reply_addr, reply);
}

#endif   /* CLUSTERING_GENERIC_RAFT_NETWORK_TCC_ */

