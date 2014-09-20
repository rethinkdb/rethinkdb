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

This implementation differs from the Raft paper in several ways:

  * The paper uses timeouts to detect when a cluster member is no longer available. This
    implementation assumes that the networking logic will tell it when a cluster member
    is no longer available. However, it still uses timeouts for leader election.

  * There are some other differences too small to list here.

This implementation supports both log compaction and configuration changes.

This implementation is templatized on two types, `state_t` and `change_t`. `change_t`
describes the operations that are sent to the leader, replicated to the members of the
cluster, and stored in the log. `state_t` is the piece of data that the Raft cluster is
collectively managing.

Both `state_t` and `change_t` must be default-constructable, destructable, and copy- and
move- constructable and assignable. In addition, `state_t` must support the following
method:

    void state_t::apply_change(const change_t &);
    
`apply_change()` applies a change in place to the `state_t`, mutating it in place. */

/* `raft_term_t` and `raft_log_index_t` are typedefs to improve the readability of the
implementation, by making it clearer what the meaning of a particular number is. */
typedef uint64_t raft_term_t;
typedef uint64_t raft_log_index_t;

/* Every member of the Raft cluster is identified by a UUID. The Raft paper uses integers
for this purpose, but we use UUIDs because we have no reliable distributed way of
assigning integers. */
typedef uuid_u raft_member_id_t;

/* `raft_config_t` describes the set of members that are involved in the Raft cluster. */
class raft_config_t {
public:
    /* Regular members of the Raft cluster go in `voting_members`. `non_voting_members`
    is for members that should receive updates, but that don't count for voting purposes.
    */
    std::set<raft_member_id_t> voting_members, non_voting_members;

    /* Returns a list of all members, voting and non-voting. */
    std::set<raft_member_id_t> get_all_members() const {
        std::set<raft_member_id_t> members;
        members.insert(voting_members.begin(), voting_members.end());
        members.insert(non_voting_members.begin(), non_voting_members.end());
        return members;
    }

    /* Returns `true` if `members` constitutes a majority. */
    bool is_quorum(const std::set<raft_member_id_t> &members) const {
        size_t votes = 0;
        for (const raft_member_id_t &m : members) {
            votes += voting_members.count(m);
        }
        return (votes*2 > voting_members.size());
    }
};

/* `raft_complex_config_t` can represent either a `raft_config_t` or a joint consensus of
an old and a new `raft_config_t`. */
class raft_complex_config_t {
public:
    /* For a regular configuration, `config` holds the configuration and `new_config` is
    empty. For a joint consensus configuration, `config` holds the old configuration and
    `new_config` holds the new configuration. */
    raft_config_t config;
    boost::optional<raft_config_t> new_config;

    bool is_joint_consensus() const {
        return static_cast<bool>(new_config);
    }

    std::set<raft_member_id_t> get_all_members() const {
        std::set<raft_member_id_t> members = config.get_all_members();
        if (is_joint_consensus) {
            /* Raft paper, Section 6: "Log entries are replicated to all servers in both
            configurations." */
            std::set<raft_member_id_t> members2 = new_config->get_all_members();
            members.insert(members2.begin(), members2.end());
        }
        return members;
    }

    bool is_quorum(const std::set<raft_member_id_t> &members) const {
        /* Raft paper, Section 6: "Agreement (for elections and entry commitment)
        requires separate majorities from both the old and new configurations." */
        if (is_joint_consensus) {
            return config.is_quorum(members) && new_config->is_quorum(members);
        } else {
            return config.is_quorum(members);
        }
    }
};

/* `raft_log_entry_t` describes an entry in the Raft log. */
template<class change_t>
class raft_log_entry_t {
public:
    enum class type_t {
        /* A `regular` log entry is one with a `change_t`. */
        regular,
        /* A `configuration` log entry has a `raft_complex_config_t`. They are used to
        change the cluster configuration. See Section 6 of the Raft paper. */
        configuration,
        /* A `noop` log entry does nothing and carries niether a `change_t` nor a
        `raft_configuration_t`. See Section 8 of the Raft paper. */
        noop
    };

    type_t type;
    raft_term_t term;
    boost::optional<change_t> change;
    boost::optional<raft_complex_config_t> configuration;
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

    raft_log_index_t prev_log_index;   /* RSI: rename these */
    raft_term_t prev_log_term;
    std::deque<raft_log_entry_t<change_t> > entries;

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
            return get_entry(index).term;
        }
    }

    /* Returns the entry in the log at the given index. */
    const raft_log_entry_t<change_t> &get_entry(raft_log_index_t index) const {
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
    void append(const raft_log_entry_t<change_t> &entry) {
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
    joining an already-established Raft cluster. */
    static raft_persistent_state_t make_join();

    /* `current_term` and `voted_for` correspond to the variables with the same names in
    Figure 2 of the Raft paper. `snapshot_state` and `snapshot_configuration` correspond
    to the stored snapshotted state, as described in Section 7. `log.prev_log_index` and
    `log.prev_log_term` correspond to the "last included index" and "last included term"
    as described in Section 7. `log.entries` corresponds to the `log` variable in Figure
    2. */
    raft_term_t current_term;
    raft_member_id_t voted_for;
    state_t snapshot_state;
    raft_complex_config_t snapshot_configuration;
    raft_log_t<change_t> log;
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
      * `send_*_rpc()` will retry the RPC as many times as necessary. This means that
        `on_*_rpc()` may be called multiple times for a single call to `send_*_rpc()`.
      * `send_*_rpc()` blocks until a response is received or the interruptor is pulsed.
      * If a response is received, it stores the response in the `*_out` variables.
      * If the interruptor is pulsed, it throws `interrupted_exc_t`. The message may or
        may not have been sent. */

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
        don't want to do that; instead, we send the `state_t` and `raft_configuration_t`
        directly. So our `snapshot_state` and `snapshot_configuration` parameters replace
        the `offset`, `data`, and `done` parameters of the Raft paper. */
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot_state,
        const raft_complex_config_t &snapshot_configuration,
        signal_t *interruptor,
        /* `term_out` corresponds to the `term` parameter of the RPC reply in the Raft
        paper. */
        raft_term_t *term_out) = 0;

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
        paper. */
        bool *success_out) = 0;

    /* `write_persistent_state()` writes the state of the Raft member to stable storage.
    It does not return until the state is safely stored. The values stored with
    `write_state()` will be passed to the `raft_member_t` constructor when the Raft
    member is restarted. */
    virtual void write_persistent_state(
        const raft_persistent_state_t<state_t, change_t> &persistent_state,
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
        const state_t &snapshot_state,
        const raft_complex_config_t &snapshot_configuration,
        signal_t *interruptor,
        raft_term_t *term_out);
    void on_append_entries_rpc(
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *success_out);

    /* `on_member_disconnect()` notifies the `raft_member_t` that the connection to the
    given Raft member has been lost. This may be the trigger to start a new leader
    election. */
    void on_member_disconnect(
        const raft_member_id_t &member,
        signal_t *interruptor);

    /* `propose_change_if_leader()` tries to perform the given change if this Raft member
    is the leader. A return value of `true` means the change was accepted. `false` means
    the change might or might not have been accepted; either something went wrong or we
    weren't the leader. */
    bool propose_change_if_leader(
        const change_t &change,
        signal_t *interruptor);

    /* `propose_config_change_if_leader()` is like `propose_change_if_leader()` except
    that it proposes a reconfiguration instead of a `change_t`. */
    bool propose_config_change_if_leader(
        const raft_config_t &configuration,
        signal_t *interruptor);

private:
    enum class mode_t {
        follower,
        candidate,
        leader
    };

    /* `update_term` sets the term to `new_term` and resets all per-term variables. */
    void update_term(raft_term_t new_term,
                     const new_mutex_acq_t *mutex_acq);

    /* When we change the commit index we have to also apply changes to the state
    machine. `update_commit_index()` handles that automatically. */
    void update_commit_index(raft_log_index_t new_commit_index,
                             const new_mutex_acq_t *mutex_acq);

    /* When we change `match_index` we might have to update `commit_index` as well.
    `update_match_index()` handles that automatically. */
    void update_match_index(
        /* Since `match_index` lives on the stack of `run_candidate_and_leader()`, we
        have to pass in a pointer. */
        std::map<raft_member_id_t, raft_log_index_t> *match_index,
        raft_member_id_t key,
        raft_log_index_t new_value,
        const new_mutex_acq_t *mutex_acq);

    /* `become_follower()` moves us from the `candidate` or `leader` state to `follower`
    state. It kills `run_candidate_and_leader()` and blocks until it exits. */
    void become_follower(const new_mutex_acq_t *mutex_acq);

    /* `become_candidate()` moves us from the `follower` state to the `candidate` state.
    It spawns `run_candidate_and_leader()`.*/
    void become_candidate(const new_mutex_acq_t *mutex_acq);

    /* `run_candidate_and_leader()` contains most of the candidate- and leader-specific
    logic. It runs in a separate coroutine for as long as we are a candidate or leader.
    */
    void run_candidate_and_leader(
        /* the `new_mutex_acq_t` used by `become_candidate()` */
        const new_mutex_acq_t *mutex_acq_for_setup,
        /* A condition variable to pulse when `mutex_acq` is no longer needed */
        cond_t *pulse_when_done_with_setup,
        /* To make sure that `run_candidate_and_leader` stops before the `raft_member_t`
        is destroyed. This is also used by `become_follower()` to  */
        auto_drainer_t::lock_t leader_keepalive);

    /* `run_election()` is a helper function for `run_candidate_and_leader()`. It sends
    out request-vote RPCs and wait for us to get enough votes. It blocks until we are
    elected; the caller is responsible for detecting the case where another leader is
    elected and also for detecting the case where the election times out. In either of
    these cases, it should pulse `interruptor`. */
    void run_election(
        /* Note that `run_election()` may temporarily release `mutex_acq`, but it will
        always be holding the lock when `run_election()` exits. */
        scoped_ptr_t<new_mutex_acq_t> *mutex_acq,
        signal_t *interruptor);

    /* `run_spawn_update_coros()` is a helper function for `run_candidate_and_leader()`
    that spawns or kills instances of `run_updates()` as necessary to ensure that there
    is always one for each cluster member. */
    void run_spawn_update_coros(
        /* A map containing `matchIndex` for each connected peer, as described in Figure
        2 of the Raft paper. This lives on the stack in `run_candidate_and_leader()`. */
        std::map<raft_member_id_t, raft_log_index_t> *match_indexes,
        /* A map containing an `auto_drainer_t` for each running update coroutine. */
        std::map<raft_member_id_t, scoped_ptr_t<auto_drainer_t> > *update_drainers,
        const new_mutex_acq_t *mutex_acq);

    /* `run_updates()` is a helper function for `run_candidate_and_leader()`;
    `run_candidate_and_leader()` spawns one in a new coroutine for each peer. It pushes
    install-snapshot RPCs and/or append-entry RPCs out to the given peer until
    `update_keepalive.get_drain_signal()` is pulsed. */
    void run_updates(
        const raft_member_id_t &peer,
        raft_log_index_t initial_next_index,
        std::map<raft_member_id_t, raft_log_index_t> *match_indexes,
        auto_drainer_t::lock_t update_keepalive);

    /* `run_reconfiguration_second_phase()` is a helper function for
    `run_candidate_and_leader()`. It checks if we have completed the first phase of a
    configuration phase (by committing a joint consensus configuration) and if so, it
    starts the second phase. */
    void run_reconfiguration_second_phase(
        const new_mutex_acq_t *mutex_acq);

    /* `note_term_as_leader()` is a helper function for `run_election()` and
    `run_updates()`. If the given term is greater than the current term, it updates the
    current term and interrupts `run_candidate_and_leader()`. It returns `true` if the
    term was changed. */
    bool note_term_as_leader(raft_term_t term, const new_mutex_acq_t *mutex_acq);

    /* `propose_change_internal()` is a helper for `propose_change_if_leader()` and
    `propose_config_change_if_leader()`. It adds an entry to the log but then returns
    immediately. */
    void propose_change_internal(
        const raft_log_entry_t<change_t> &log_entry,
        const new_mutex_acq_t *mutex_acq);

    /* Returns the latest configuration that appears in the log or the snapshot. The
    returned reference points to something in `ps`.
    Raft paper, Section 6: "a server always uses the latest configuration in its log,
    regardless of whether the entry is committed" */
    const raft_complex_config_t &get_configuration();

    const raft_member_id_t member_id;
    const raft_network_and_storage_interface_t *interface;

    /* A two-letter name was chosen for this variable because we end up writing `ps.*`
    constantly. */
    raft_persistent_state_t<state_t, change_t> ps;

    /* This `state_t` describes the current state of the "state machine" that the Raft
    cluster is controlling. */
    state_t state_machine;

    /* `commit_index` and `last_applied` correspond to the volatile state variables with
    the same names in Figure 2 of the Raft paper. */
    raft_log_index_t commit_index, last_applied;

    /* Only `run_candidate_and_leader()` should ever change `mode` */
    mode_t mode;

    /* `this_term_leader_id` is the ID of the member that is leader during this term. If
    we haven't seen any node acting as leader this term, it's `nil_uuid()`. We use it to
    redirect clients as described in Figure 2 and Section 8. This implementation also
    uses it in `on_member_disconnect()` to determine if we should start a new election.
    */
    raft_member_id_t this_term_leader_id;   /* RSI set this when elected */

    /* This mutex ensures that operations don't interleave in confusing ways. Each RPC
    acquires this mutex when it begins and releases it when it returns. Also, if
    `run_candidate_and_leader()` is running, it holds this mutex when actively
    manipulating state and releases it when waiting. */
    new_mutex_t mutex;

    /* This mutex assertion controls writes to the Raft log and associated state.
    Specifically, anything writing to `ps.log`, `ps.snapshot`, `state_machine`,
    `commit_index`, or `last_applied` should hold this mutex assertion.
    If we are follower, `on_append_entries_rpc()` and `on_install_snapshot_rpc()` acquire
    this temporarily; if we are candidate or leader, `run_candidate_and_leader()` holds
    this at all times. */
    mutex_assertion_t log_mutex;

    /* When we are leader, `update_watchers` is a set of conds that should be pulsed
    every time we append something to the log or update `commit_index`. If we are not
    leader, this is empty and unused. */
    std::set<cond_t *> update_watchers;

    /* This makes sure that `run_candidate_and_leader()` stops when the `raft_member_t`
    is destroyed. It's in a `scoped_ptr_t` so that `become_follower()` can destroy it to
    kill `run_candidate_and_leader()`. */
    scoped_ptr_t<auto_drainer_t> leader_drainer;

    /* Occasionally we have to spawn miscellaneous coroutines. This makes sure that they
    all get stopped before the `raft_member_t` is destroyed. */
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_GENERIC_RAFT_CORE_HPP_ */

