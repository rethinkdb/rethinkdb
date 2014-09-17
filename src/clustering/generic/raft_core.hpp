// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_CORE_HPP_
#define CLUSTERING_GENERIC_RAFT_CORE_HPP_

/* This file implements a generalization of the Raft consensus algorithm, as described in
the paper "In Search of an Understandable Consensus Algorithm (Extended Version)" (2014)
by Diego Ongaro and John Ousterhout. Because of the complexity and subtlety of the Raft
algorithm, this implementation follows the paper closely and refers back to it regularly.
You are advised to have a copy of the paper on hand when reading or modifying this file.

This file only contains the basic Raft algorithm itself; it doesn't contain any
networking or storage logic. Instead, it uses an abstract interface to send and receive
network messages and write data to persistent storage. This both keeps this file as
as simple as possible and makes it easy to test the Raft algorithm using mocked-up
network and storage systems.

This implementation differs from the Raft paper in several major ways:

  * This implementation allows the Raft cluster members to reject proposed changes
    according to an arbitrary criterion. A change cannot be committed unless a quorum of
    Raft members accept it. If even a single member rejects the change, the
    implementation is not guaranteed commit it, even if a quorum of members accept it.

  * This implementation allows different changes to require the approval of different
    subsets of the Raft cluster, instead of every change requiring the approval of more
    than half of the overall cluster.

  * The paper uses timeouts to detect when a cluster member is no longer available. This
    implementation assumes that the networking logic will tell it when a cluster member
    is no longer available. However, it still uses timeouts for leader election.

There are a number of smaller differences as well, but they aren't important enough to
list here.

This implementation is templatized on two types, `state_t` and `change_t`. `change_t`
describes the operations that are sent to the leader, replicated to the members of the
cluster, and stored in the log. `state_t` is the piece of data that the Raft cluster is
collectively managing.

This implementation supports log compaction, as described in Section 7 of the Raft paper.
The `state_t` is the type of a stored snapshot.

This implementation supports configuration changes, as described in Section 6 of the Raft
paper; but the details are slightly different. `state_t` is responsible for tracking the
cluster configuration, including judging whether a set of nodes constitutes a quorum. To
support configuration changes, `state_t` must be capable of expressing a joint consensus.
The client submits a specially-tagged `change_t` that switches to a joint consensus, and
then the leader automatically submits a second `change_t` to switchs to the new
configuration.

Both `state_t` and `change_t` must be default-constructable, destructable, and copy- and
move- constructable and assignable. In addition, they must have the following methods,
which must be deterministic and side-effect-free unless otherwise noted:

    bool state_t::consider_change(const change_t &) const;

`consider_change()` returns `true` if the change is valid for the state, and `false` if
not. If the change is not valid for the state at the time that the leader receives it,
the leader will not even attempt to apply it to the cluster.

    void state_t::apply_change(const change_t &);

`apply_change()` applies a change in place to the `state_t`, mutating it in place. The
change is guaranteed to be valid for the state.

    std::set<raft_member_id_t> state_t::get_all_members() const;

`get_all_members()` returns a set of all the Raft members that need to receive updates.

    bool state_t::is_quorum_for_change(
        const std::set<raft_member_id_t> &members,
        const change_t &change) const;

`is_quorum_for_change()` returns `true` if `members` is a sufficient set of members to
approve the given change. If `m1` and `m2` are disjoint sets, then
`is_quorum_for_change(m1, c)` and `is_quorum_for_change(m2, c)` mustn't both return
`true`. The change is guaranteed to be valid for the state.

    bool state_t::is_quorum_for_all(const std::set<raft_member_id_t> &members) const;

 `is_quorum_for_all(m)` returns `true` if `is_quorum_for_change(m, c)` would return
`true` for every `c`.

    raft_change_type_t change_t::get_change_type() const;
    change_t change_t::make_config_second_change() const;

`get_change_type()` returns `regular` for an ordinary `change_t`; `config_first` for a
change that puts the cluster into a joint consensus state; and `config_second` for a
change that takes the cluster out of a joint consensus state. Clients should not
directly submit `config_second` changes; instead, the client should submit the
`config_first` change and the leader will call `make_config_second_change()` to derive
the corresponding `config_second` change. */

/* `raft_term_t` and `raft_log_index_t` are typedefs to improve the readability of the
implementation, by making it clearer what the meaning of a particular number is. */
typedef uint64_t raft_term_t;
typedef uint64_t raft_log_index_t;

/* Every member of the Raft cluster is identified by a UUID. The Raft paper uses integers
for this purpose, but we use UUIDs because we have no reliable distributed way of
assigning integers. */
typedef uuid_u raft_member_id_t;

/* `raft_change_type_t` describes whether a given `change_t` has any effect on the
Raft configuration. */
enum class raft_change_type_t {
    /* A `regular` change doesn't alter the Raft configuration in any way. */
    regular,
    /* A `config_first` change initiates a reconfiguration by putting the cluster into a
    joint consensus state. */
    config_first,
    /* A `config_second` change ends a reconfiguration by taking the cluster back out of
    the joint consensus state. */
    config_second
};

/* `raft_log_t` stores a slice of the Raft log. There are two situations where this shows
up in Raft: in an "AppendEntries RPC", and in each server's local state. The Raft paper
represents this as three separate variables, but grouping them together makes the
implementation clearer. */
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

    /* Return the latest index that is present in the log. If the log is empty, returns
    the index on which the log is based. */
    raft_log_index_t get_latest_index() const {
        return prev_log_index + entries.size();
    }

    /* Returns the term of the log entry at the given index. The index must either be
    present in the log or the last index before the log. */
    raft_term_t get_entry_term(raft_log_index_t index) const {
        guarantee(index >= prev_log_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        if (index == prev_log_index) {
            return prev_log_term;
        } else {
            return get_entry(index).second;
        }
    }

    /* Returns the entry in the log at the given index. */
    const std::pair<change_t, raft_term_t> &get_entry(raft_log_index_t index) const {
        guarantee(index > prev_log_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        return entries[index - prev_log_index - 1];
    }

    /* Deletes the log entry at the given index and all entries after it. */
    void delete_entries_from(raft_log_index_t index) {
        guarantee(index > prev_log_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        entries.erase(entries.begin() + (index - prev_log_index - 1), entries.end());
    }

    /* Deletes the log entry at the given index and all entries before it. */
    void delete_entries_to(raft_log_index_t index) {
        guarantee(index > prev_log_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        raft_term_t index_term = get_entry_term(index);
        entries.erase(entries.begin(), entries.begin() + (index - prev_log_index));
        prev_log_index = index;
        prev_log_term = index_term;
    }

    /* Appends the given entry ot the log. */
    void append(const std::pair<change_t, raft_term_t> &entry) {
        entries.push_back(entry);
    }
};

/* `raft_persistent_state_t` describes the information that each member of the Raft
cluster persists to stable storage. */
template<class state_t, class change_t>
class raft_persistent_state_t {
public:
    /* `make_initial(s)` returns a `raft_persistent_state_t` for a member of a new Raft
    instance with starting state `s`. The caller must ensure that every member of the
    new Raft instance starts with the same value of `s`. */
    static raft_persistent_state_t make_initial(const state_t &s);

    /* `make_join()` returns a `raft_persistent_state_t` for a Raft member that will be
    joining an already-established Raft clustter. */
    static raft_persistent_state_t make_join();

    /* `current_term` and `voted_for` correspond to the variables with the same names in
    Figure 2 of the Raft paper. `snapshot` corresponds to the stored snapshotted state,
    as described in Section 7. `log.prev_log_index` and `log.prev_log_term` correspond to
    the "last included index" and "last included term" as described in Section 7.
    `log.entries` corresponds to the `log` variable in Figure 2. */
    raft_term_t current_term;
    raft_member_id_t voted_for;
    state_t snapshot;
    raft_log_t<change_t> log;
};

/* `raft_change_result_t` describes the outcome of proposing a change to the Raft
cluster. In the original Raft algorithm, only the `success` and `retry` outcomes are
possible, so this is represented as a boolean value instead. It means slightly different
things in different contexts, which is why the description given here is somewhat vague.
*/
enum class raft_change_outcome_t {
    /* The change happened. */
    success,
    /* The change did not happen because of a temporary condition; perhaps the leader
    proposing the change is out of date. */
    retry,
    /* The change did not happen because one or more Raft members rejected the proposed
    change. */
    rejected
};

/* `raft_network_and_storage_interface_t` is the abstract class that the Raft
implementation uses to send and receive messages over the network, and to store data to
disk. */
template<class state_t, class change_t>
class raft_network_and_storage_interface_t {
public:
    /* The `send_*_rpc()` methods all follow these rules:
      * They send an RPC message to the Raft member indicated in the `dest` field.
      * The message will be delivered by calling the `on_*_rpc()` method on the
        `raft_member_t` in question.
      * The method blocks until a response is received or the interruptor is pulsed. It
        will retry the RPC as many times as necessary.
      * If a response is received, it stores the response in the `*_out` variables.
      * If the interruptor is pulsed, it throws `interrupted_exc_t`. The message may or
        may not have been sent. */

    /* `send_append_entries_rpc` corresponds to the "AppendEntries RPC" described in
    Figure 2 of the Raft paper. */
    virtual void send_append_entries_rpc(
        const raft_member_id_t &dest,
        /* `term`, `leader_id`, and `leader_commit` correspond to the parameters with the
        same names in the Raft paper. `entries` corresponds to three of the paper's
        variables: `prevLogIndex`, `prevLogTerm`, and `entries`. */
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        signal_t *interruptor,
        /* `term_out` corresponds to the `term` parameter of the RPC reply in the paper.
        */
        raft_term_t *term_out,
        /* `success_out` corresponds to the `success` parameter of the RPC reply in the
        paper. `success` and `retry` correspond to `true` and `false` in the paper;
        `rejected` is returned if the member rejected the change. */
        raft_change_outcome_t *success_out) = 0;

    /* `send_request_vote_rpc` corresponds to the "RequestVote RPC" described in Figure
    2 of the Raft paper. */
    virtual void send_request_vote_rpc(
        const raft_member_id_t &dest,
        /* `term`, `candidate_id`, `last_log_index`, and `last_log_term` correspond to
        the parameters with the same names in the Raft paper. */
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        signal_t *interruptor,
        /* `term_out` and `vote_granted_out` correspond to the `term` and `voteGranted`
        parameters of the RPC reply in the Raft paper. */
        raft_term_t *term_out,
        bool *vote_granted_out) = 0;

    /* `send_install_snapshot_rpc` corresponds to the "InstallSnapshot RPC" described in
    Figure 13 of the Raft paper. */
    virtual void send_install_snapshot_rpc(
        const raft_member_id_t &dest,
        /* `term`, `leader_id`, `last_included_index`, and `last_included_term`
        correspond to the parameters with the same names in the Raft paper. In the Raft
        paper, the content of the snapshot is sent as a series of binary blobs, but we
        don't want to do that; instead, we send a single `state_t` instance. So our
        `snapshot` parameter replaces the `offset`, `data`, and `done` parameters of the
        Raft paper. */
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot,
        signal_t *interruptor,
        /* `term_out` corresponds to the `term` parameter of the RPC reply in the Raft
        paper. */
        raft_term_t *term_out) = 0;

    /* `write_persistent_state()` writes the state of the Raft member to stable storage.
    It does not return until the state is safely stored. The values stored with
    `write_state()` will be passed to the `raft_member_t` constructor when the Raft
    member is restarted. */
    virtual void write_persistent_state(
        const raft_persistent_state_t<state_t, change_t> &persistent_state,
        signal_t *interruptor) = 0;

    /* `consider_proposed_change()` returns `true` if the proposed change is OK and
    `false` if it is not. The criterion for accepting or rejecting the proposed change
    can be anything; it may be non-deterministic, have side-effects, etc. The Raft
    cluster will not commit a change unless enough members return `true` in
    `consider_proposed_change()` that `state.is_quorum_for_change()` returns `true` for
    that set of members. In addition, if any member returns `false` for
    `consider_proposed_change()`, the Raft cluster may (but is not required to) refuse
    to commit that change. */
    virtual bool consider_proposed_change(
        const change_t &change,
        signal_t *interruptor) = 0;
};

/* `raft_member_t` is responsible for managing the activity of a single member of the
Raft cluster. */
template<class state_t, class change_t>
class raft_member_t<state_t, change_t> :
    public home_thread_mixin_debug_only_t
{
public:
    raft_member_t(
        const raft_member_id_t &member_id,
        raft_network_and_storage_interface_t *_interface,
        const raft_persistent_state_t<state_t, change_t> &_persistent_state);

    /* Note that if a method on `raft_member_t` is interrupted, the `raft_member_t` will
    be left in an undefined internal state. Therefore, no further method calls should be
    made once the interruptor has been pulsed. (However, even though the internal state
    is undefined, the interrupted method call will not make invalid RPC calls or write
    invalid data to persistent storage.) */

    /* The `on_*_rpc()` methods are called when a Raft member calls a `send_*_rpc()`
    method on their `raft_network_and_storage_interface_t`. */
    void on_append_entries_rpc(
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *success_out);
    void on_request_vote_rpc(
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *vote_granted_out);
    void on_install_snapshot_rpc(
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot,
        signal_t *interruptor,
        raft_term_t *term_out);

    /* `on_member_disconnect()` notifies the `raft_member_t` that the connection to the
    given Raft member has been lost. This may be the trigger to start a new leader
    election. */
    void on_member_disconnect(
        const raft_member_id_t &member,
        signal_t *interruptor);

    /* `propose_change_if_leader()` tries to perform the given change if this Raft member
    is the leader. A return value of `success` means the change was committed. `retry`
    means the change may or may not have been committed; either this member is not the
    leader, or a temporary condition went wrong. `rejected` means the change may or may
    not have been committed, because one or more members rejected it. */
    raft_change_outcome_t propose_change_if_leader(
        const change_t &change,
        signal_t *interruptor);

private:
    enum class mode_t {
        follower,
        candidate,
        leader
    };

    /* `update_term` sets the term to `new_term` and resets all per-term variables. */
    void update_term(raft_term_t new_term,
                     const mutex_t::acq_t *mutex_acq);

    /* The Raft paper specifies actions we should take every time `commit_index` changes.
    These are encapsulated in `update_commit_index()`, which changes `commit_index` and
    also takes the relevant actions. */
    void update_commit_index(raft_log_index_t new_commit_index,
                             const mutex_t::acq_t *mutex_acq);

    /* `become_follower()` moves us from the `candidate` or `leader` state to `follower`
    state. It blocks until `leader_coro()` exits. */
    void become_follower(const mutex_t::acq_t *mutex_acq);

    /* `become_candidate()` moves us from the `follower` state to the `candidate` state.
    It spawns `leader_coro()`. */
    void become_candidate(const mutex_t::acq_t *mutex_acq);

    /* `leader_coro()` contains most of the candidate- and leader-specific logic. It runs
    for as long as we are a candidate or leader. */
    void leader_coro(
        /* the `mutex_t::acq_t` used by `become_candidate()` */
        const mutex_t::acq_t *mutex_acq_for_setup,
        /* A condition variable to pulse when `mutex_acq` is no longer needed */
        cond_t *pulse_when_done_with_setup,
        /* To make sure that `leader_coro` stops before the `raft_member_t` is destroyed
        */
        auto_drainer_t::lock_t keepalive);

    /* Returns what the state machine would look like if every change in the log were
    applied. This is important for cluster configuration purposes, because we're supposed
    to respect cluster configuration changes as soon as they appear in the log, even if
    they're not committed yet. */
    state_t get_state_including_log(const mutex_t::acq_t *mutex_acq);

    raft_member_id_t member_id;
    raft_network_and_storage_interface_t *interface;

    /* A two-letter name was chosen for this variable because we end up writing `ps.*`
    constantly. */
    raft_persistent_state_t<state_t, change_t> ps;

    /* This `state_t` describes the current state of the "state machine" that the Raft
    cluster is controlling. */
    state_t state_machine;

    /* `commit_index` and `last_applied` correspond to the volatile state variables with
    the same names in Figure 2 of the Raft paper. */
    raft_log_index_t commit_index, last_applied;

    /* Only `leader_coro()` should ever change `mode` */
    mode_t mode;

    /* `this_term_leader_id` is the ID of the member that is leader during this term. If
    we haven't seen any node acting as leader this term, it's `nil_uuid()`. We use it to
    redirect clients as described in Figure 2 and Section 8. This implementation also
    uses it in `on_member_disconnect()` to determine if we should start a new election.
    */
    raft_member_id_t this_term_leader_id;

    /* To reduce the likelihood of unexpected race conditions, we use this mutex to
    guard almost all operations on the `raft_member_t`. */
    mutex_t mutex;

    /* This makes sure that `leader_coro()` stops when the `raft_member_t` is destroyed.
    It's in a `scoped_ptr_t` so that `become_follower()` can destroy it to kill
    `leader_coro()`. */
    scoped_ptr_t<auto_drainer_t> drainer;
};

#endif /* CLUSTERING_GENERIC_RAFT_CORE_HPP_ */

