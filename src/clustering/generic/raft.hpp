// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_HPP_
#define CLUSTERING_GENERIC_RAFT_HPP_

/* This file implements a generalization of the Raft consensus algorithm, as described in
the paper "In Search of an Understandable Consensus Algorithm (Extended Version)" (2014)
by Diego Ongaro and John Ousterhout. Because of the complexity and subtlety of the Raft
algorithm, this implementation follows the paper closely and refers back to it regularly.
You are advised to have a copy of the paper on hand when reading or modifying this file.

This implementation differs from the original Raft algorithm in several ways:

 1. If more than half of the nodes in a Raft cluster become unavailable, the cluster
    cannot proceed. This implementation allows an administrator to manually override the
    cluster state in this situation. This is implemented by starting a fresh Raft
    instance, which will ignore any messages related to the previous Raft instance.
    Because this change doesn't interfere with the operation of each individual Raft
    instance, it's unlikely to introduce bugs.

 2. This implementation allows the Raft cluster members to reject proposed changes
    according to an arbitrary criterion.

 3. This implementation allows different changes to require the approval of different
    subsets of the Raft cluster, instead of every change requiring the approval of more
    than half of the overall cluster.

*/

/* A `raft_instance_id_t` is used to identify an instance of the Raft algorithm; if the
user manually overrides Raft, a new `raft_instance_id_t` is created. If a Raft member
sees that there exists an instance with a newer timestamp (or equal timestamp and
lexicographically larger uuid) then it will discard its state and attempt to join the
newer instance. This way, if the user's manual override fails to reach every member of
the cluster, the other members won't get stuck. */
class raft_instance_id_t {
public:
    uuid_u uuid;
    uint64_t timestamp;
};

/* Every member of the Raft cluster is identified by a UUID. The Raft paper uses integers
for this purpose, but we use UUIDs because we have no reliable distributed way of
assigning integers. */
typedef uuid_u raft_member_id_t;

/* `raft_term_t` and `raft_log_index_t` are typedefs to improve the readability of the
implementation, by making it clearer what the meaning of a particular number is. */
typedef uint64_t raft_term_t;
typedef uint64_t raft_log_index_t;

/* `raft_log_t` stores a collection of log entries that might not stretch all the way
back to the beginning of time. There are two situations where this shows up in Raft:
in an append-entries message, and in each server's local state. The Raft paper represents
this as three separate variables, but grouping them together makes the implementation
clearer. */
template<class change_t>
class raft_log_t {
public:
    /* In an append-entries message, `prev_log_index`, `prev_log_term`, and `entries`
    corresponds to the parameters with the same names in the "AppendEntries RPC"
    described in Figure 2 of the Raft paper.

    In a server's local status, `prev_log_index` and `prev_log_term` correspond to the
    "last included index" and "last included term" variables as described in Section 7.
    `entries` corresponds to the `log` variable described in Figure 2. */

    raft_log_index_t prev_log_index;
    raft_term_t prev_log_term;
    std::deque<std::pair<change_t, raft_term_t> > entries;

    raft_log_index_t get_latest_index() const {
        return prev_log_index + entries.size();
    }
    raft_term_t get_entry_term(raft_log_index_t index) const {
        if (index < prev_log_index) {
            crash("the log doesn't go back this far");
        } else if (index == prev_log_index) {
            return prev_log_term;
        } else if (index <= get_latest_index()) {
            return entries[index - prev_log_index - 1].second;
        } else {
            crash("the log doesn't go forward this far");
        }
    }
    const std::pair<change_t, raft_term_t> &get_entry(raft_log_index_t index) const {
        if (index <= prev_log_index) {
            crash("the log doesn't go back this far");
        } else if (index > get_latest_index()) {
            crash("the log doesn't go forward this far");
        } else {
            return entries[index - prev_log_index - 1];
        }
    }
    void delete_entries_from(raft_log_index_t index) {
        if (index <= prev_log_index) {
            crash("the log doesn't go back this far");
        }
        entries.erase(entries.begin() + index - prev_log_index - 1, entries.end());
    }
    void append(const std::pair<change_t, raft_term_t> &entry) {
        entries.push_back(entry);
    }
}

template<class state_t, class change_t>
class raft_address_t {

    /* `append_entries_mailbox_t` and `append_entries_reply_mailbox_t` correspond to the
    "AppendEntries RPC" described in Figure 2 of the Raft paper. */
    typedef mailbox_t<void(
        raft_term_t term,
        bool success
        )> append_entries_reply_mailbox_t;
    typedef mailbox_t<void(
        /* `sender_instance_id` does not appear in the Raft paper; it's part of an
        extension to the original algorithm. */
        raft_instance_id_t sender_instance_id,
        raft_term_t term,
        raft_member_id_t leader_id,
        /* This implementation combines the three variables `prevLogIndex`,
        `prevLogTerm`, and `entries` into a single variable. */
        raft_log_t<change_t> entries,
        raft_log_index_t leader_commit,
        append_entries_reply_mailbox_t::address_t reply_address
        )> append_entries_mailbox_t;

    /* `request_vote_mailbox_t` and `request_vote_reply_mailbox_t` correspond to the
    "RequestVote RPC" described in Figure 2 of the Raft paper. */
    typedef mailbox_t<void(
        raft_term_t term,
        bool vote_granted
        )> request_vote_reply_mailbox_t;
    typedef mailbox_t<void(
        /* `sender_instance_id` does not appear in the Raft paper; it's part of an
        extension to the original algorithm. */
        raft_instance_id_t sender_instance_id,
        raft_term_t term,
        raft_member_id_t candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        request_vote_reply_mailbox_t::address_t reply_address
        )> request_vote_mailbox_t;

    /* `install_snapshot_mailbox_t` and `install_snapshot_reply_mailbox_t` correspond to
    the "InstallSnapshot RPC" described in Figure 13 of the Raft paper. */
    typedef mailbox_t<void(
        raft_term_t term
        )> install_snapshot_reply_mailbox_t;
    typedef mailbox_t<void(
        /* `sender_instance_id` does not appear in the Raft paper; it's part of an
        extension to the original algorithm. */
        raft_instance_id_t sender_instance_id,
        raft_term_t term,
        raft_member_id_t leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        /* In the Raft paper, the snapshot is sent incrementally as a series of binary
        blobs. This implementation sends the snapshot as a single unit, as an instance of
        the `state_t` type. So this `snapshot` parameter replaces the `offset`, `data`,
        and `done` parameters of the Raft paper's "InstallSnapshot RPC". */
        state_t snapshot,
        install_snapshot_reply_mailbox_t::address_t reply_address
        )> install_snapshot_mailbox_t;

    /* `propose_mailbox_t` is how clients send requests to the Raft leader. The Raft
    paper describes an outline of this in Section 8, but it doesn't provide specifics. */
    typedef mailbox_t<void(
        /* The ID of the current leader. If the client sends its request to a member that
        isn't the leader, it can use this information to find the real leader. */
        raft_member_id_t leader_id,
        /* Returns `true` if the proposed change has been committed to the Raft cluster,
        and `false` if anything went wrong (in which case the change may or may not be
        committed) */
        bool success
        )> propose_reply_mailbox_t;
    typedef mailbox_t<void(
        change_t proposal,
        propose_reply_mailbox_t::address_t reply_address
        )> propose_mailbox_t;

    /* `reset_instance_mailbox_t` is not part of the original Raft algorithm. It's used
    to manually repair a Raft cluster that cannot proceed because it has lost more than
    half of its members. */
    typedef mailbox_t<void(
        )> reset_instance_reply_mailbox_t;
    typedef mailbox_t<void(
        /* The instance ID of the new Raft instance that the member is being forced to
        join */
        raft_instance_id_t new_instance_id,
        /* The initial state of the new Raft instance that the member is being forced to
        join; it will be the same for every member of the instance. */
        state_t new_state,
        reset_instance_reply_mailbox_t::address_t reply_address
        )> reset_instance_mailbox_t;

    raft_member_id_t member_id;
    raft_instance_id_t instance_id;
    append_entries_mailbox_t::address_t append_entries;
    request_vote_mailbox_t::address_t request_vote;
    install_snapshot_mailbox_t::address_t install_snapshot;
    propose_mailbox_t::address_t propose;
    reset_instance_mailbox_t::address_t reset_instance;
};

/* `raft_storage_t` is an abstract class that provides a Raft member with persistent
storage. */
template<class state_t, class change_t>
class raft_storage_t {
public:
    /* If `write_all` has never been called, then `read_all` should return a nil UUID for
    `member_id_out`. */
    virtual void read_all(
        raft_member_id_t *member_id_out,
        raft_instance_id_t *instance_out,
        raft_term_t *current_term_out,
        raft_member_id_t *voted_for_out,
        state_t *snapshot_out,
        raft_log_t<change_t> *log_out,
        signal_t *interruptor) = 0;
    virtual void write_all(
        const raft_member_id_t &member_id,
        const raft_instance_t &instance,
        raft_term_t current_term,
        const raft_member_id_t &voted_for,
        const state_t &snapshot,
        const raft_log_t<change_t> &log,
        signal_t *interruptor) = 0;
};

/* `raft_member_t` contains the logic for a single member of a Raft cluster. */
template<class state_t, class change_t>
class raft_member_t :
    public home_thread_mixin_debug_only_t
{
public:
    raft_member_t(
        raft_storage_t<state_t, change_t> *_storage,
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, raft_address_t> > >
            _directory,
        signal_t *interruptor);
    clone_ptr_t<watchable_t<raft_address_t> > get_address();

    clone_ptr_t<watchable_t<state_t> > get_state() {
        return state_machine.get_watchable();
    }

    virtual bool consider(const change_t &change) = 0;

    /* `propose_if_leader()` attempts to make the given change to the cluster's state if
    this node is the leader. If the change is committed, returns `true`. If this node is
    not the leader, or if it ceases to be leader before the change is committed, returns
    `false`. */
    bool propose_if_leader(const change_t &change, signal_t *interruptor);

private:
    void on_append_entries(
        const raft_instance_id_t &sender_instance_id,
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        cosnt append_entries_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive);

    void on_request_vote(
        const raft_instance_id_t &sender_instance_id,
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        const request_vote_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive);

    void on_install_snapshot(
        const raft_instance_id_t &sender_instance_id,
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot,
        const install_snapshot_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive);

    void on_propose(
        const change_t &proposal,
        const propose_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive);

    void on_reset_instance(
        const raft_instance_id_t &new_instance,
        const state_t &new_state,
        const reset_instance_reply_mailbox_t::address_t &reply_address,
        auto_drainer_t::lock_t keepalive);

    raft_storage_t<state_t, change_t> *storage;
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, raft_address_t> > >
        directory;

    /* These are mirrored in persistent storage */
    raft_member_id_t member_id;
    raft_instance_id_t instance_id;

    /* These correspond to the persistent state variables listed in the Raft paper */
    raft_term_t current_term;
    raft_member_id_t voted_for;
    state_t snapshot;
    /* `log` contains the snapshot state variables called "last included index",
    "last included term", plus the state variable called `log`, from the Raft paper. */
    raft_log_t<change_t> log;

    /* These correspond to the volatile state variables listed in the Raft paper */
    raft_log_index_t commit_index;
    raft_log_index_t last_applied;
    std::map<raft_member_id_t, raft_log_index_t> next_index;
    std::map<raft_member_id_t, raft_log_index_t> match_index;

    enum class mode_t { follower, candidate, leader };
    mode_t mode;

    watchable_variable_t<state_t> state_machine;

    mutex_t mutex;
    auto_drainer_t drainer;

    typename raft_address_t<state_t, change_t>::append_entries_mailbox_t
        append_entries_mailbox;
    typename raft_address_t<state_t, change_t>::request_vote_mailbox_t
        request_vote_mailbox;
    typename raft_address_t<state_t, change_t>::install_snapshot_mailbox_t
        install_snapshot_mailbox;
    typename raft_address_t<state_t, change_t>::propose_mailbox_t
        propose_mailbox;
    typename raft_address_t<state_t, change_t>::reset_instance_mailbox_t
        reset_instance_mailbox;
};

template<class state_t, class change_t>
class raft_client_t {
    raft_client_t(
        mailbox_manager_t *mm,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, raft_address_t> > > peer_map);

    bool reset_instance(const state_t &new_state, signal_t *interruptor);
    bool propose(const change_t &change, signal_t *interruptor);
};

#endif /* CLUSTERING_GENERIC_RAFT_HPP_ */

