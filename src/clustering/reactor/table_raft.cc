// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/table_raft.hpp"

table_raft_t::table_raft_t(
        const machine_id_t &_machine_id,
        mailbox_manager_t *_mailbox_manager) :
    machine_id(_machine_id),
    mailbox_manager(_mailbox_manager),
    member(machine_id, this, ?),
    request_vote_mailbox(mailbox_manager,
        boost::bind(&table_raft_t::on_request_vote_rpc,
            this, _1, _2, auto_drainer_t::lock_t(&drainer))),
    install_snapshot_mailbox(mailbox_manager,
        boost::bind(&table_raft_t::on_install_snapshot_rpc,
            this, _1, _2, auto_drainer_t::lock_t(&drainer))),
    append_entries_mailbox(mailbox_manager,
        boost::bind(&table_raft_t::on_append_entries_rpc,
            this, _1, _2, auto_drainer_t::lock_t(&drainer)))
    { }

bool table_raft_t::send_request_vote_rpc(
        const raft_member_id_t &dest,
        const raft_request_vote_rpc_t &rpc,
        signal_t *interruptor,
        raft_request_vote_reply_t *reply_out) {
}

bool table_raft_t::send_install_snapshot_rpc(
        const raft_member_id_t &dest,
        const raft_install_snapshot_rpc_t<table_raft_state_t, table_raft_change_t> &rpc,
        signal_t *interruptor,
        raft_install_snapshot_reply_t *reply_out) {
}

bool table_raft_t::send_append_entries_rpc(
        const raft_member_id_t &dest,
        const raft_append_entries_rpc_t<table_raft_change_t> &rpc,
        signal_t *interruptor,
        raft_append_entries_reply_t *reply_out) {
}

clone_ptr_t<watchable_t<std::set<raft_member_id_t> > >
        table_raft_t::get_connected_members() {
}

void table_raft_t::write_persistent_state(
        const raft_persistent_state_t<table_raft_state_t, table_raft_change_t>
            &persistent_state,
        signal_t *interruptor) {
}

void table_raft_t::on_request_vote_rpc(
        const raft_request_vote_rpc_t &rpc,
        const mailbox_t<void(raft_request_vote_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive) {
    raft_request_vote_reply_t reply;
    member.on_request_vote_rpc(rpc, keepalive.get_drain_signal(), &reply);
    send(mailbox_manager, reply_addr, reply);
}

void table_raft_t::on_install_snapshot_rpc(
        const raft_install_snapshot_rpc_t<table_raft_state_t, table_raft_change_t> &rpc,
        const mailbox_t<void(raft_install_snapshot_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive) {
    raft_install_snapshot_reply_t reply;
    member.on_install_snapshot_rpc(rpc, keepalive.get_drain_signal(), &reply);
    send(mailbox_manager, reply_addr, reply);
}

void table_raft_t::on_append_entries_rpc(
        const raft_append_entries_rpc_t<table_raft_change_t> &rpc,
        const mailbox_t<void(raft_append_entries_reply_t)>::address_t &reply_addr,
        auto_drainer_t::lock_t keepalive) {
    raft_append_entries_reply_t reply;
    member.on_append_entries_rpc(rpc, keepalive.get_drain_signal(), &reply);
    send(mailbox_manager, reply_addr, reply);
}

