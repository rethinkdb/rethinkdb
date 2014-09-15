// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_TCC_
#define CLUSTERING_GENERIC_RAFT_TCC_

template<class state_t, class change_t>
raft_member_t<state_t, change_t>::raft_member_t(
        raft_storage_t<state_t, change_t> *_storage,
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, raft_address_t> > >
            _directory,
        signal_t *interruptor) :
    storage(_storage),
    mailbox_manager(_mailbox_manager),
    directory(_directory),
    commit_index(0),
    last_applied(0),
    append_entries_mailbox(mailbox_manager, boost::bind(
        &raft_member_t<state_t, change_t>::on_append_entries, this,
        _1, _2, _3, _4, _5, _6, auto_drainer_t::lock_t(&drainer))),
    request_vote_mailbox(mailbox_manager, boost::bind(
        &raft_member_t<state_t, change_t>::on_request_vote, this,
        _1, _2, _3, _4, _5, _6, auto_drainer_t::lock_t(&drainer))),
    install_snapshot_mailbox(mailbox_manager, boost::bind(
        &raft_member_t<state_t, change_t>::on_install_snapshot, this,
        _1, _2, _3, _4, _5, _6, _7, auto_drainer_t::lock_t(&drainer))),
    propose_mailbox(mailbox_manager, boost::bind(
        &raft_member_t<state_t, change_t>::on_propose, this,
        _1, _2, auto_drainer_t::lock_t(&drainer))),
    reset_instance_mailbox(mailbox_manager, boost::bind(
        &raft_member_t<state_t, change_t>::on_reset_instance, this,
        _1, _2, _3, auto_drainer_t::lock_t(&drainer))) {
    storage->read_all(&member_id, &world, &current_term, &voted_for, &snapshot,
        &snapshot_term, &snapshot_log_index, &log, interruptor);
    state_machine.set_value(snapshot);
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::propose_if_leader(
        const change_t &change, signal_t *interruptor) {
    
}

void raft_member_t<state_t, change_t>::become_leader(???) {
    mutex_t::acq_t mutex_acq(&mutex);
    guarantee(mode == mode_t::follower);
    mode = mode_t::candidate;
    ++current_term;
    voted_for = member_id;
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_append_entries(
        const raft_instance_id_t &sender_instance_id,
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        const append_entries_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive) {
    assert_thread();
    mutex_t::acq_t mutex_acq(&mutex);

    if (sender_instance_id != world) {
        return;
    }

    for (raft_log_index_t i = min(entries.get_latest_index(), leader_commit) + 1;
                          i <= entries.get_latest_index();
                        ++i) {
        if (!consider(entries.get_entry(i).first)) {
            return;
        }
    }

    if (term < current_term) {
        send(mailbox_manager, reply_address, current_term, false);
        return;
    }

    if (entries.prev_log_index > log.get_latest_index() ||
            log.get_entry_term(prev_log_index) != entries.prev_log_term) {
        send(mailbox_manager, reply_address, current_term, false);
    }

    for (raft_log_index_t i = entries.prev_log_index + 1;
                          i <= min(log.get_latest_index(), entries.get_latest_index());
                        ++i) {
        if (log.get_entry_term(i) != entries.get_entry_term(i)) {
            log.delete_entries_from(i);
            break;
        }
    }

    for (raft_log_index_t i = log.get_latest_index() + 1;
                          i <= entries.get_latest_index();
                        ++i) {
        log.append(entries.get_entry(i));
    }

    if (leader_commit > commit_index) {
        commit_index = min(leader_commit, entries.get_latest_index());
    }

    storage->write_all(member_id, world, current_term, voted_for, snapshot, log,
        keepalive.get_drain_signal());

    send(mailbox_manager, reply_address, current_term, true);
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_request_vote(
        const raft_instance_id_t &sender_instance_id,
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        const request_vote_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive) {
    assert_thread();
    mutex_t::acq_t mutex_acq(&mutex);

    if (sender_instance_id != world) {
        return;
    }

    if (term < current_term) {
        send(mailbox_manager, reply_address, current_term, false);
        return;
    }

    if (term > current_term) {
        current_term = term;
        voted_for = nil_uuid();
    }

    if (!voted_for.is_nil() && voted_for != candidate_id) {
        send(mailbox_manager, reply_address, current_term, false);
        return;
    }

    bool candidate_is_at_least_as_up_to_date =
        last_log_term > log.get_entry_term(log.get_latest_index()) ||
            (last_log_term == log.get_entry_term(log.get_latest_index()) &&
                last_log_index >= log.get_latest_index());

    if (!candidate_is_at_least_as_up_to_date) {
        send(mailbox_manager, reply_address, current_term, false);
        return;
    }

    voted_for = candidate_id;

    send(mailbox_manager, reply_address, current_term, true);
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_install_snapshot(
        const raft_instance_id_t &sender_instance_id,
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot,
        const install_snapshot_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive) {
    assert_thread();
    mutex_t::acq_t mutex_acq(&mutex);
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_propose(
        const change_t &proposal,
        const propose_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive) {
    assert_thread();
    mutex_t::acq_t mutex_acq(&mutex);
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_reset_instance(
        const raft_instance_id_t &new_world,
        const state_t &new_state,
        const reset_instance_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive) {
    assert_thread();
    mutex_t::acq_t mutex_acq(&mutex);
}

#endif /* CLUSTERING_GENERIC_RAFT_TCC_ */

