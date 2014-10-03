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

    typedef mailbox_t<void(raft_term_t, bool)> request_vote_reply_mailbox_t;
    typedef mailbox_t<void(
        raft_term_t,
        raft_member_id_t,
        raft_log_index_t,
        raft_term_t,
        request_vote_reply_mailbox_t::address_t
        )> request_vote_mailbox_t;

    typedef mailbox_t<void(raft_term_t)> install_snapshot_reply_mailbox_t;
    typedef mailbox_t<void(
        raft_term_t,
        raft_member_id_t,
        raft_log_index_t,
        raft_term_t,
        table_raft_state_t,
        raft_complex_config_t,
        install_snapshot_reply_mailbox_t::address_t
        )> install_snapshot_mailbox_t;

    typedef mailbox_t<void(raft_term_t, bool)> append_entries_reply_mailbox_t;
    typedef mailbox_t<void(
        raft_term_t,
        raft_member_id_t,
        raft_log_t<table_raft_change_t>,
        raft_log_index_t,
        append_entries_reply_mailbox_t::address_t
        )> append_entries_mailbox_t;
};

class table_raft_t :
    private raft_network_and_storage_interface_t<table_raft_state_t, table_raft_change_t>
{
public:
    bool send_request_vote_rpc(
        const raft_member_id_t &dest,
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *vote_granted_out);

    bool send_install_snapshot_rpc(
        const raft_member_id_t &dest,
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot_state,
        const raft_complex_config_t &snapshot_configuration,
        signal_t *interruptor,
        raft_term_t *term_out);

    bool send_append_entries_rpc(
        const raft_member_id_t &dest,
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *success_out);

    clone_ptr_t<watchable_t<std::set<raft_member_id_t> > >
        get_connected_members();

    void write_persistent_state(
        const raft_persistent_state_t<state_t, change_t> &persistent_state,
        signal_t *interruptor);

private:
    raft_member_t<table_raft_state_t, table_raft_change_t> member;

    table_raft_business_card_t::request_vote_mailbox_t request_vote_mailbox;
    table_raft_business_card_t::install_snapshot_mailbox_t install_snapshot_mailbox;
    table_raft_business_card_t::append_entries_mailbox_t append_entries_mailbox;
};


#endif /* CLUSTERING_REACTOR_TABLE_DRIVER_HPP_ */

