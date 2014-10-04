// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_TABLE_DRIVER_HPP_
#define CLUSTERING_REACTOR_TABLE_DRIVER_HPP_

class table_raft_state_t;
class table_raft_change_t;

class table_raft_instance_id_t {
public:
    uuid_u uuid;
    microtime_t timestamp;
};

class table_raft_business_card_t {
public:
    table_raft_instance_id_t instance_id;

    typedef mailbox_t<void(
        raft_request_vote_rpc_t,
        mailbox_t<void(raft_request_vote_reply_t)>::address_t
        )> request_vote_mailbox_t;
    typedef mailbox_t<void(
        raft_install_snapshot_rpc_t<table_raft_state_t, table_raft_change_t>,
        mailbox_t<void(raft_install_snapshot_reply_t)>::address_t
        )> install_snapshot_mailbox_t;
    typedef mailbox_t<void(
        raft_append_entries_rpc_t<table_raft_change_t>,
        mailbox_t<void(raft_append_entries_reply_t)>::address_t
        )> append_entries_mailbox_t;
};

class table_raft_t :
    private raft_network_and_storage_interface_t<table_raft_state_t, table_raft_change_t>
{
public:
    table_raft_t(
        const machine_id_t &_machine_id,
        mailbox_manager_t *_mailbox_manager);

private:
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


#endif /* CLUSTERING_REACTOR_TABLE_DRIVER_HPP_ */

