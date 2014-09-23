// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_CORE_TCC_
#define CLUSTERING_GENERIC_RAFT_CORE_TCC_

#include "clustering/generic/raft_core.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/coroutines.hpp"
#include "containers/map_sentries.hpp"

template<class state_t, class change_t>
raft_persistent_state_t<state_t, change_t>
raft_persistent_state_t<state_t, change_t>::make_initial(
        const state_t &initial_state,
        const raft_config_t &initial_config) {
    raft_persistent_state_t<state_t, change_t> ps;
    /* The Raft paper indicates that `current_term` should be initialized to 0 and the
    first log index is 1. However, we initialize `current_term` to 1 and the first log
    entry will have index 2. We reserve term 1 and log index 1 for a "virtual" log entry
    that sets `initial_state` and `initial_configuration`. This ensures that when a
    member joins the cluster with a state created by `make_join()`, the initial state
    will be correctly transferred to it. In effect, we are constructing a
    `raft_persistent_state_t` in which one change has already been committed. */
    ps.current_term = 1;
    ps.voted_for = nil_uuid();
    ps.snapshot_state = boost::optional<state_t>(initial_state);
    raft_complex_config_t complex_config;
    complex_config.config = initial_config;
    ps.snapshot_configuration = boost::optional<raft_complex_config_t>(complex_config);
    ps.log.prev_index = 1;
    ps.log.prev_term = 1;
    return ps;
}

template<class state_t, class change_t>
raft_persistent_state_t<state_t, change_t>
raft_persistent_state_t<state_t, change_t>::make_join() {
    raft_persistent_state_t<state_t, change_t> ps;
    /* Here we initialize `current_term` to 0 and the first log index to 1, as the Raft
    paper says to. */
    ps.current_term = 0;
    ps.voted_for = nil_uuid();
    ps.snapshot_state = boost::optional<state_t>();
    ps.snapshot_configuration = boost::optional<raft_complex_config_t>();
    ps.log.prev_index = 0;
    /* We don't initialize `ps.log.prev_term`; it shouldn't ever be used. */
    return ps;
}

template<class state_t, class change_t>
raft_member_t<state_t, change_t>::raft_member_t(
        const raft_member_id_t &_this_member_id,
        raft_network_and_storage_interface_t<state_t, change_t> *_interface,
        const raft_persistent_state_t<state_t, change_t> &_persistent_state) :
    this_member_id(_this_member_id),
    interface(_interface),
    ps(_persistent_state),
    /* Restore state machine from snapshot */
    state_machine(static_cast<bool>(ps.snapshot_state) ? *ps.snapshot_state : state_t()),
    /* We initialize `commit_index` and `last_applied` to the last commit that was stored
    in the snapshot so they will be consistent with the state of the state machine. */
    commit_index(ps.log.prev_index),
    last_applied(ps.log.prev_index),
    /* Raft paper, Section 5.2: "When servers start up, they begin as followers." */
    mode(mode_t::follower),
    current_term_leader_id(nil_uuid()),
    /* Set this so we start an election if we don't hear from a leader within an election
    timeout of when the constructor was called */
    last_heard_from_leader(current_microtime()),
    drainer(new auto_drainer_t),
    /* Setting the `watchdog_timer` to ring every `election_timeout_min_ms` means that
    we'll start a new election after between 1 and 2 election timeouts have passed. This
    is OK. */
    watchdog_timer(new repeating_timer_t(
        election_timeout_min_ms,
        [this] () { this->on_watchdog_timer(); }
        ))
{
    if (static_cast<bool>(ps.snapshot_state)) {
        initialized_cond.pulse();
    }

    new_mutex_acq_t mutex_acq(&mutex);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t, class change_t>
raft_member_t<state_t, change_t>::~raft_member_t() {
    /* Now that the destructor has been called, we can safely assume that our public
    methods will not be called. */

    new_mutex_acq_t mutex_acq(&mutex);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    /* Destroy `watchdog_timer` so that it doesn't start any new actions while we're
    cleaning up. */
    watchdog_timer.reset();

    /* Destroy `drainer` to kill any miscellaneous coroutines that aren't related to
    `candidate_and_leader_coro()`. */
    drainer.reset();

    /* Now kill `candidate_and_leader_coro()`, if it's running */
    if (mode != mode_t::follower) {
        candidate_or_leader_become_follower(&mutex_acq);
    }

    /* All the coroutines have stopped, so we can start calling member destructors. */
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::propose_change_if_leader(
        const change_t &change,
        signal_t *interruptor) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex, interruptor);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    if (mode != mode_t::leader) {
        return false;
    }

    /* TODO: We shouldn't allow changes to pile up on a leader that isn't capable of
    getting them committed. In particular:
      * If the number of uncommitted log entries is greater than a threshold, we should
        not accept new changes.
      * If we're not in contact with a majority of the cluster, we should not accept new
        changes.
    The same goes for configuration changes. */

    raft_log_entry_t<change_t> new_entry;
    new_entry.type = raft_log_entry_t<change_t>::type_t::regular;
    new_entry.change = boost::optional<change_t>(change);
    new_entry.term = ps.current_term;

    leader_append_log_entry(new_entry, &mutex_acq, interruptor);

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
    return true;
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::propose_config_change_if_leader(
        const raft_config_t &new_config,
        signal_t *interruptor) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex, interruptor);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    if (mode != mode_t::leader) {
        return false;
    }

    /* Calculate the latest committed configuration */
    raft_complex_config_t old_complex_config = *ps.snapshot_configuration;
    for (raft_log_index_t i = ps.log.prev_index + 1; i <= commit_index; ++i) {
        if (ps.log.get_entry(i).type ==
                raft_log_entry_t<change_t>::type_t::configuration) {
            old_complex_config = *ps.log.get_entry(i).configuration;
        }
    }

    /* We forbid starting a new config change before the old one is done. The Raft paper
    doesn't explicitly say anything about multiple interleaved configuration changes; but
    it's safer and simpler to forbid it. */
    if (old_complex_config.is_joint_consensus()) {
        return false;
    }
    for (raft_log_index_t i = commit_index + 1; i <= ps.log.get_latest_index(); ++i) {
        if (ps.log.get_entry(i).type ==
                raft_log_entry_t<change_t>::type_t::configuration) {
            return false;
        }
    }

    raft_complex_config_t new_complex_config;
    new_complex_config.config = old_complex_config.config;
    new_complex_config.new_config = boost::optional<raft_config_t>(new_config);

    raft_log_entry_t<change_t> new_entry;
    new_entry.type = raft_log_entry_t<change_t>::type_t::configuration;
    new_entry.configuration = boost::optional<raft_complex_config_t>(new_complex_config);
    new_entry.term = ps.current_term;

    leader_append_log_entry(new_entry, &mutex_acq, interruptor);

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
    return true;
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_request_vote_rpc(
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *vote_granted_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex, interruptor);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    /* Raft paper, Section 6: "Servers disregard RequestVote RPCs when they believe a
    current leader exists... Specifically, if a server receives a RequestVote RPC within
    the minimum election timeout of hearing from a current leader, it does not update its
    term or grant its vote."
    Note that if we are a leader, we disregard all RequestVote RPCs, because we
    believe that a current leader (us) exists. */
    if (mode == mode_t::leader ||
            (mode == mode_t::follower && current_microtime() <
                last_heard_from_leader + election_timeout_min_ms * 1000)) {
        *term_out = ps.current_term;
        *vote_granted_out = false;
        return;
    }

    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        update_term(term, &mutex_acq);
        if (mode != mode_t::follower) {
            candidate_or_leader_become_follower(&mutex_acq);
        }
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 2: "Reply false if term < currentTerm" */
    if (term < ps.current_term) {
        *term_out = ps.current_term;
        *vote_granted_out = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    /* Sanity checks, not explicitly described in the Raft paper. */
    guarantee(candidate_id != this_member_id, "We shouldn't be requesting a vote from "
        "ourself.");
    if (mode != mode_t::follower) {
        guarantee(ps.voted_for == this_member_id, "We should have voted for ourself "
            "already.");
    }

    /* Raft paper, Section 5.2: "A server remains in follower state as long as it
    receives valid RPCs from a leader or candidate."
    So candidate RPCs are sufficient to delay the watchdog timer. */
    last_heard_from_leader = current_microtime();

    /* Raft paper, Figure 2: "If votedFor is null or candidateId, and candidate's log is
    at least as up-to-date as receiver's log, grant vote */

    /* So if `voted_for` is neither `nil_uuid()` nor `candidate_id`, we don't grant the
    vote */
    if (!ps.voted_for.is_nil() && ps.voted_for != candidate_id) {
        *term_out = ps.current_term;
        *vote_granted_out = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    /* Raft paper, Section 5.4.1: "Raft determines which of two logs is more up-to-date
    by comparing the index and term of the last entries in the logs. If the logs have
    last entries with different terms, then the log with the later term is more
    up-to-date. If the logs end with the same term, then whichever log is longer is more
    up-to-date." */
    bool candidate_is_at_least_as_up_to_date =
        last_log_term > ps.log.get_entry_term(ps.log.get_latest_index()) ||
            (last_log_term == ps.log.get_entry_term(ps.log.get_latest_index()) &&
                last_log_index >= ps.log.get_latest_index());
    if (!candidate_is_at_least_as_up_to_date) {
        *term_out = ps.current_term;
        *vote_granted_out = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    ps.voted_for = candidate_id;

    /* Raft paper, Figure 2: "Persistent state [is] updated on stable storage before
    responding to RPCs" */
    interface->write_persistent_state(ps, interruptor);

    *term_out = ps.current_term;
    *vote_granted_out = true;

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_install_snapshot_rpc(
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot_state,
        const raft_complex_config_t &snapshot_configuration,
        signal_t *interruptor,
        raft_term_t *term_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex, interruptor);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        update_term(term, &mutex_acq);
        if (mode != mode_t::follower) {
            candidate_or_leader_become_follower(&mutex_acq);
        }
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 13: "Reply immediately if term < currentTerm" */
    if (term < ps.current_term) {
        *term_out = ps.current_term;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    guarantee(term == ps.current_term);   /* sanity check */

    /* Raft paper, Section 5.2: "While waiting for votes, a candidate may receive an
    AppendEntries RPC from another server claiming to be leader. If the leader's term
    (included in its RPC) is at least as large as the candidate's current term, then the
    candidate recognizes the leader as legitimate and returns to follower state."
    This implementation also does this in response to install-snapshot RPCs. This is
    because it's conceivably possible that the newly-elected leader will send an
    install-snapshot RPC instead of an append-entries RPC in our implementation. */
    if (mode == mode_t::candidate) {
        candidate_or_leader_become_follower(&mutex_acq);
    }

    /* Raft paper, Section 5.2: "at most one candidate can win the election for a
    particular term"
    If we're leader, then we won the election, so it makes no sense for us to receive an
    RPC from another member that thinks it's leader. */
    guarantee(mode != mode_t::leader);

    /* Raft paper, Section 5.2: "A server remains in follower state as long as it
    receives valid RPCs from a leader or candidate."
    So we should make a note that we received an RPC. */
    last_heard_from_leader = current_microtime();

    /* Recall that `current_term_leader_id` is set to `nil_uuid()` if we haven't seen a
    leader yet this term. */
    if (current_term_leader_id.is_nil()) {
        current_term_leader_id = leader_id;
    } else {
        /* Raft paper, Section 5.2: "at most one candidate can win the election for a
        particular term" */
        guarantee(current_term_leader_id == leader_id);
    }

    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* Note that this implementation goes out of order compared to the Raft paper. The
    Raft paper says that the first step should be to save the snapshot, which for us
    would correspond to setting `ps.snapshot_state` and `ps.snapshot_configuration`. But
    because we only store one snapshot at a time and require that snapshot to be right
    before the start of the log, that doesn't make sense for us. Instaed, we save the
    snapshot if and when we update the log. */

    /* Raft paper, Figure 13: "If existing log entry has same index and term as
    snapshot's last included entry, retain log entries following it and reply" */
    if (last_included_index <= ps.log.prev_index) {
        /* The proposed snapshot starts at or before our current snapshot. It's
        impossible to check if an existing log entry has the same index and term because
        the snapshot's last included entry is before our most recent entry. But if that's
        the case, we don't need this snapshot, so we can safely ignore it. */
        if (last_included_index == ps.log.prev_index) {
            guarantee(last_included_term == ps.log.prev_term, "The entry we shapshotted "
                "at was committed, so we shouldn't be receiving a different committed "
                "entry at the same index.");
        }
        *term_out = ps.current_term;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    } else if (last_included_index <= ps.log.get_latest_index() &&
            ps.log.get_entry_term(last_included_index) == last_included_term) {
        /* Raft paper, Section 7: "If ... the follower receives a snapshot that describes
        a prefix of its log (due to retransmission or by mistake), then log entries
        covered by the snapshot are deleted but entries following the snapshot are still
        valid and must be retained. */
        ps.log.delete_entries_to(last_included_index);

        /* Raft paper, Figure 13: "Save snapshot file"
        (We're going slightly out of order, as described above) */
        ps.snapshot_state = boost::optional<state_t>(snapshot_state);
        ps.snapshot_configuration =
            boost::optional<raft_complex_config_t>(snapshot_configuration);
        guarantee(ps.log.prev_index == last_included_index);
        guarantee(ps.log.prev_term == last_included_term);

        /* The "retain entries following it and reply" language in the paper seems to
        imply that we should stop now. But it's pretty obvious that we should update the
        state machine as well if the snapshot advances the committed index, so we don't
        return yet. */
    } else {
        /* Raft paper, Figure 13: "Discard the entire log" */
        ps.log.entries.clear();

        /* Raft paper, Figure 13: "Save snapshot file"
        (We're going slightly out of order, as described above) */
        ps.snapshot_state = boost::optional<state_t>(snapshot_state);
        ps.snapshot_configuration =
            boost::optional<raft_complex_config_t>(snapshot_configuration);
        ps.log.prev_index = last_included_index;
        ps.log.prev_term = last_included_term;
    }

    /* Raft paper, Figure 13: "Reset state machine using snapshot contents"
    This implementation doesn't update the state machine if the current state machine is
    actually more up to date than the snapshot. The paper doesn't explicitly say to do
    this; I assume this was an omission in the paper. */
    if (ps.log.prev_index > last_applied) {
        /* The snapshot state must be an initialized state; i.e. it's impossible for us
        to ever receive the initial empty state in the form of a snapshot. */
        state_machine.set_value(*ps.snapshot_state);
        initialized_cond.pulse_if_not_already_pulsed();
        last_applied = ps.log.prev_index;
        /* It's hypothetically possible that `commit_index` could be greater than
        `last_applied` */
        commit_index = std::max(ps.log.prev_index, commit_index);
    }

    /* Raft paper, Figure 2: "Persistent state [is] updated on stable storage before
    responding to RPCs" */
    interface->write_persistent_state(ps, interruptor);

    *term_out = ps.current_term;

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_append_entries_rpc(
        raft_term_t term,
        const raft_member_id_t &leader_id,
        const raft_log_t<change_t> &entries,
        raft_log_index_t leader_commit,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *success_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex, interruptor);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    /* Raft paper, Figure 2: "If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower" */
    if (term > ps.current_term) {
        update_term(term, &mutex_acq);
        if (mode != mode_t::follower) {
            candidate_or_leader_become_follower(&mutex_acq);
        }
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 2: "Reply false if term < currentTerm (SE 5.1)"
    Raft paper, Section 5.1: "If a server receives a request with a stale term number, it
    rejects the request" */
    if (term < ps.current_term) {
        /* Raft paper, Figure 2: term should be set to "currentTerm, for leader to update
        itself" */
        *term_out = ps.current_term;
        *success_out = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    guarantee(term == ps.current_term);   /* sanity check */

    /* Raft paper, Section 5.2: "While waiting for votes, a candidate may receive an
    AppendEntries RPC from another server claiming to be leader. If the leader's term
    (included in its RPC) is at least as large as the candidate's current term, then the
    candidate recognizes the leader as legitimate and returns to follower state." */
    if (mode == mode_t::candidate) {
        candidate_or_leader_become_follower(&mutex_acq);
    }

    /* Raft paper, Section 5.2: "at most one candidate can win the election for a
    particular term"
    If we're leader, then we won the election, so it makes no sense for us to receive an
    RPC from another member that thinks it's leader. */
    guarantee(mode != mode_t::leader);

    /* Raft paper, Section 5.2: "A server remains in follower state as long as it
    receives valid RPCs from a leader or candidate."
    So we should make a note that we received an RPC. */
    last_heard_from_leader = current_microtime();

    /* Recall that `current_term_leader_id` is set to `nil_uuid()` if we haven't seen a
    leader yet this term. */
    if (current_term_leader_id.is_nil()) {
        current_term_leader_id = leader_id;
    } else {
        /* Raft paper, Section 5.2: "at most one candidate can win the election for a
        particular term" */
        guarantee(current_term_leader_id == leader_id);
    }

    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* Raft paper, Figure 2: "Reply false if log doesn't contain an entry at prevLogIndex
    whose term matches prevLogTerm" */
    if (entries.prev_index > ps.log.get_latest_index() ||
            ps.log.get_entry_term(entries.prev_index) != entries.prev_term) {
        *term_out = ps.current_term;
        *success_out = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    /* Raft paper, Figure 2: "If an existing entry conflicts with a new one (same index
    but different terms), delete the existing entry and all that follow it" */
    for (raft_log_index_t i = entries.prev_index + 1;
            i <= std::min(ps.log.get_latest_index(), entries.get_latest_index());
            ++i) {
        if (ps.log.get_entry_term(i) != entries.get_entry_term(i)) {
            ps.log.delete_entries_from(i);
            break;
        }
    }

    /* Raft paper, Figure 2: "Append any new entries not already in the log" */
    for (raft_log_index_t i = ps.log.get_latest_index() + 1;
            i <= entries.get_latest_index();
            ++i) {
        ps.log.append(entries.get_entry_ref(i));
    }

    /* Raft paper, Figure 2: "If leaderCommit > commitIndex, set commitIndex = min(
    leaderCommit, index of last new entry)" */
    while (leader_commit > commit_index) {
        update_commit_index(
            std::min(leader_commit, entries.get_latest_index()),
            &mutex_acq);
    }

    /* Raft paper, Figure 2: "Persistent state [is] updated on stable storage before
    responding to RPCs" */
    interface->write_persistent_state(ps, interruptor);

    *term_out = ps.current_term;
    *success_out = true;

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::check_invariants(
        const std::set<raft_member_t<state_t, change_t> *> &members) {
    /* We acquire each member's mutex to ensure we don't catch them in invalid states */
    std::vector<scoped_ptr_t<new_mutex_acq_t> > mutex_acqs;
    for (auto member : members) {
        scoped_ptr_t<new_mutex_acq_t> mutex_acq(new new_mutex_acq_t(&member->mutex));
        /* Check each member's invariants individually */
        member->check_invariants(mutex_acq.get());
        mutex_acqs.push_back(std::move(mutex_acq));
    }

    {
        /* Raft paper, Figure 3: "Election Safety: at most one leader can be elected in
        a given term" */
        std::set<raft_term_t> claimed;
        for (auto member : members) {
            if (member->mode == mode_t::leader) {
                auto pair = claimed.insert(member->ps.current_term);
                guarantee(pair.second, "At most one leader can be elected in a given "
                    "term");
            }
        }
    }

    /* It's impractical to test the Leader Append-Only property described in Figure 3
    because we don't store history, so we can't tell what operations the leader is
    performing on its log. */

    {
        /* Raft paper, Figure 3: "Log Matching: if two logs contain an entry with the
        same index and term, then the logs are identical in all entries up through the
        given index." */
        for (auto m1 : members) {
            for (auto m2 : members) {
                bool match_so_far = true;
                for (raft_log_index_t i = std::max(m1->ps.log.prev_index + 1,
                                                   m2->ps.log.prev_index + 1);
                        i <= std::min(m1->ps.log.get_latest_index(),
                                      m2->ps.log.get_latest_index());
                        ++i) {
                    raft_log_entry_t<change_t> e1 = m1->ps.log.get_entry_ref(i);
                    raft_log_entry_t<change_t> e2 = m2->ps.log.get_entry_ref(i);
                    if (e1.term == e2.term) {
                        guarantee(e1.type == e2.type,
                            "Log matching property violated");
                        guarantee(e1.change == e2.change,
                            "Log matching property violated");
                        guarantee(e1.configuration == e2.configuration,
                            "Log matching property violated");
                        guarantee(match_so_far,
                            "Log matching property violated");
                    } else {
                        match_so_far = false;
                    }
                }
            }
        }
    }

    /* It's impractical to test the Leader Completeness property because we snapshot log
    entries so quickly that the test would in practice be a no-op. */

    {
        /* Raft paper, Figure 3: "State Machine Safety: if a server has applied a log
        entry at a given index to its state machine, no other server will ever apply a
        different log entry for the same index. */
        std::map<raft_member_t<state_t, change_t> *, boost::optional<state_t> > states;
        std::map<raft_member_t<state_t, change_t> *,
            boost::optional<raft_complex_config_t> > configs;
        raft_log_index_t start = std::numeric_limits<raft_log_index_t>::max(),
                         end = std::numeric_limits<raft_log_index_t>::min();
        for (auto m : members) {
            start = std::min(start, m->ps.log.prev_index);
            end = std::max(end, m->commit_index);
        }
        for (raft_log_index_t i = start; i <= end; ++i) {
            for (auto m : members) {
                if (i == m->ps.log.prev_index) {
                    states.insert(std::make_pair(m, m->ps.snapshot_state));
                    configs.insert(std::make_pair(m, m->ps.snapshot_configuration));
                } else if (i > m->ps.log.prev_index &&
                        i <= m->ps.log.get_latest_index()) {
                    raft_log_entry_t<change_t> e = m->ps.log.get_entry_ref(i);
                    if (e.type == raft_log_entry_t<change_t>::type_t::regular) {
                        states.at(m)->apply_change(*e.change);
                    } else if (e.type ==
                            raft_log_entry_t<change_t>::type_t::configuration) {
                        configs.at(m) = *e.configuration;
                    }
                } else if (i == m->ps.log.get_latest_index() + 1) {
                    states.erase(m);
                    configs.erase(m);
                } else {
                    guarantee(states.count(m) == 0);
                }
            }
            for (const auto &pair1 : states) {
                for (const auto &pair2 : states) {
                    if (pair1.first == pair2.first) {
                        continue;
                    }
                    guarantee(pair1.second == pair2.second, "Two Raft members' state "
                        "machines don't match.");
                    guarantee(configs.at(pair1.first) == configs.at(pair2.first), "Two "
                        "Raft members' configurations don't match.");
                }
            }
        }
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::check_invariants(
        const new_mutex_acq_t *mutex_acq) {
    assert_thread();
    mutex_acq->assert_is_holding(&mutex);

    /* Some of these checks are copied directly from LogCabin's list of invariants. */

    /* Checks related to being uninitialized */
    if (!initialized_cond.is_pulsed()) {
        if (ps.current_term == 0) {
            guarantee(ps.voted_for.is_nil(), "If current_term is 0, that should mean "
                "we've never seen another peer, so we shouldn't have voted for anybody");
        }
        guarantee(!static_cast<bool>(ps.snapshot_state), "If we're uninitialized, our "
            "snapshot should be empty.");
        guarantee(!static_cast<bool>(ps.snapshot_configuration), "state and "
            "configuration should be initialized together and propagated together");
        guarantee(ps.log.prev_index == 0, "If we're uninitialized, we shouldn't have "
            "committed any changes.");
        guarantee(ps.log.entries.empty(), "If we're uninitialized, we shouldn't have "
            "any log entries.");
        guarantee(commit_index == 0, "If we're uninitialized we shouldn't have "
            "committed any changes yet");
        guarantee(last_applied == 0, "If we're uninitialized we shouldn't have applied "
            "any changes yet");
        guarantee(mode == mode_t::follower, "If we're uninitialized we shouldn't "
            "participate in elections");
        return;
    }
    guarantee(ps.current_term >= 1, "If we're initialized, we should have the initial "
        "virtual term");
    guarantee(static_cast<bool>(ps.snapshot_state), "If we're initialized, snapshot "
        "should be non-empty.");
    guarantee(static_cast<bool>(ps.snapshot_configuration), "state and configuration "
        "should be initialized together and propagated together");
    guarantee(ps.log.prev_index >= 1, "If we're initialized, we should have the initial "
        "virtual change");;
    guarantee(commit_index >= 1, "If we're initialized, we should have committed the "
        "initial virtual change.");
    guarantee(last_applied >= 1, "If we're initialized, we should have applied the "
        "initial virtual change");

    /* Checks related to the initial virtual term and initial virtual change */
    if (ps.current_term == 1) {
        guarantee(ps.voted_for.is_nil(), "If we're still in the initial virtual term, "
            "nobody has started an election, so we shouldn't have voted for anybody.");
        guarantee(ps.log.entries.empty(), "If we're still in the initial virtual term, "
            "we shouldn't have any real log entries.");
        guarantee(ps.log.prev_index == 1, "The log index of the initial virtual change "
            "should be 1");
        guarantee(commit_index == 1, "If we're still in the initial virtual term, we "
            "shouldn't have committed anything but the initial virtual change.");
        guarantee(last_applied == 1, "If we're still in the initial virtual term, we "
            "shouldn't have applied anything but the initial virtual change.");
        guarantee(mode == mode_t::follower, "If we're still in the initial virtual "
            "term, there shouldn't have been an election, so we should be follower.");
        guarantee(current_term_leader_id.is_nil(), "The initial virtual term shouldn't "
            "have a leader.");
    }
    guarantee((ps.log.prev_index == 1) == (ps.log.prev_term == 1), "The initial virtual "
        "change should be in the initial virtual term, but no other log entries should "
        "be.");

    /* Checks related to the log */
    raft_term_t latest_term_in_log = ps.log.get_entry_term(ps.log.prev_index);
    for (raft_log_index_t i = ps.log.prev_index + 1;
            i <= ps.log.get_latest_index();
            ++i) {
        guarantee(ps.log.get_entry_ref(i).term > 1,
            "There shouldn't be any real log entries in the initial virtual term.");
        guarantee(ps.log.get_entry_ref(i).term >= latest_term_in_log,
            "Terms of log entries should monotonically increase");
        raft_log_entry_t<change_t> entry = ps.log.get_entry_ref(i);
        latest_term_in_log = entry.term;

        switch (entry.type) {
        case raft_log_entry_t<change_t>::type_t::regular:
            guarantee(static_cast<bool>(entry.change), "Regular log entries should "
                "carry changes");
            guarantee(!static_cast<bool>(entry.configuration), "Regular log entries "
                "shouldn't carry configurations.");
            break;
        case raft_log_entry_t<change_t>::type_t::configuration:
            guarantee(!static_cast<bool>(entry.change), "Configuration log entries "
                "shouldn't carry changes");
            guarantee(static_cast<bool>(entry.configuration), "Configuration log "
                "entries should carry configurations.");
            break;
        case raft_log_entry_t<change_t>::type_t::noop:
            guarantee(!static_cast<bool>(entry.change), "Noop log entries shouldn't "
                "carry changes");
            guarantee(!static_cast<bool>(entry.configuration), "Noop log entries"
                "shouldn't carry configurations.");
            break;
        default:
            unreachable();
        }
    }
    guarantee(latest_term_in_log <= ps.current_term, "There shouldn't be any log "
        "entries with terms greater than current_term");

    /* Checks related to reconfigurations */
    raft_complex_config_t committed_config = *ps.snapshot_configuration;
    size_t uncommitted_config_1s = 0, uncommitted_config_2s = 0;
    for (raft_log_index_t i = ps.log.prev_index + 1;
            i <= ps.log.get_latest_index();
            ++i) {
        raft_log_entry_t<change_t> entry = ps.log.get_entry_ref(i);
        if (entry.type != raft_log_entry_t<change_t>::type_t::configuration) {
            continue;
        }
        if (i <= commit_index) {
            committed_config = *entry.configuration;
        } else {
            if (entry.configuration->is_joint_consensus()) {
                ++uncommitted_config_1s;
            } else {
                ++uncommitted_config_2s;
            }
        }
    }
    if (committed_config.is_joint_consensus()) {
        guarantee(uncommitted_config_1s == 0, "We shouldn't have two overlapping "
            "reconfigurations going on at once");
        guarantee(uncommitted_config_2s <= 1, "We shouldn't have two overlapping "
            "reconfigurations going on at once");
    } else {
        guarantee(uncommitted_config_1s <= 1, "We shouldn't have two overlapping "
            "reconfigurations going on at once");
        guarantee(uncommitted_config_2s == 0, "It's unsafe to go directly from a "
            "non-joint-consensus configuration to a non-joint-consensus configuration");
    }

    /* Checks related to the state machine and commits */
    guarantee(commit_index >= ps.log.prev_index, "All of the log entries in the "
        "snapshot should be committed.");
    guarantee(last_applied <= commit_index, "We shouldn't have applied any changes to "
        "the state machine that weren't committed.");
    guarantee(commit_index <= ps.log.get_latest_index(), "We shouldn't have committed "
        "any entries that aren't in the log yet.");
    {
        state_t st = *ps.snapshot_state;
        for (raft_log_index_t i = ps.log.prev_index+1; i <= last_applied; ++i) {
            raft_log_entry_t<change_t> entry = ps.log.get_entry_ref(i);
            if (entry.type == raft_log_entry_t<change_t>::type_t::regular) {
                st.apply_change(*entry.change);
            }
        }
        state_machine.apply_read([&](const state_t *state) {
            guarantee(st == *state, "State machine should be equal to snapshot with log "
                "entries applied.");
        });
    }
    /* These are properties that this implementation follows, but they aren't essential
    to Raft, and it would be totally reasonable to change them for performance reasons or
    something. */
    guarantee(last_applied == commit_index, "The implementation is supposed to apply "
        "changes to the state machine as soon as they're committed.");
    guarantee(ps.log.prev_index == last_applied, "The implementation is supposed to "
        "snapshot changes as soon as they're applied");

    /* Checks related to the follower/candidate/leader roles */
    guarantee((mode == mode_t::leader) == (current_term_leader_id == this_member_id),
        "mode should say we're leader iff current_term_leader_id says we're leader");
    /* We could test that `current_term_leader_id` is non-nil if there are any changes in
    the log for the current term. But this would be false immediately after startup, so
    we'd need an extra flag to detect that, and that's more work than it's worth. */
    guarantee(leader_drainer.has() == (mode != mode_t::follower),
        "candidate_and_leader_coro() should be running unless we're a follower");
    if (mode != mode_t::leader) {
        guarantee(update_watchers.empty(), "We shouldn't be using `update_watchers` "
            "unless we're a leader.");
    }
    switch (mode) {
    case mode_t::follower:
        break;
    case mode_t::candidate:
        guarantee(current_term_leader_id.is_nil(), "if we have a leader we shouldn't be "
            "having an election");
        guarantee(ps.voted_for == this_member_id, "if we're a candidate we should have "
            "voted for ourself");
        break;
    case mode_t::leader:
        guarantee(ps.voted_for == this_member_id, "if we're a leader we should have "
            "voted for ourself");
        guarantee(latest_term_in_log == ps.current_term, "if we're a leader we should "
            "have put at least the initial noop entry in the log");
        break;
    default:
        unreachable();
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_watchdog_timer() {
    if (mode != mode_t::follower) {
        /* If we're already a candidate or leader, there's no need for this. Candidates
        have their own mechanism for retrying stuck elections. */
        return;
    }
    microtime_t now = current_microtime();
    if (last_heard_from_leader > now) {
        /* System clock went in reverse. This is possible because `current_microtime()`
        estimates wall-clock time, not real time. Reset `last_heard_from_leader` to
        ensure that we'll still start an election in a reasonable amount of time if we
        don't hear from a leader. */
        last_heard_from_leader = now;
        return;
    }
    /* Raft paper, Section 5.2: "If a follower receives no communication over a period of
    time called the election timeout, then it assumes there is no viable leader and
    begins an election to choose a new leader." */
    if (last_heard_from_leader < now - election_timeout_min_ms * 1000) {
        /* We shouldn't block in this callback, so we immediately spawn a coroutine */
        auto_drainer_t::lock_t keepalive(drainer.get());
        coro_t::spawn_sometime([this, now, keepalive /* important to capture */]() {
            try {
                scoped_ptr_t<new_mutex_acq_t> mutex_acq(
                    new new_mutex_acq_t(&mutex, keepalive.get_drain_signal()));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                /* Double-check that nothing has changed while the coroutine was
                spawning. */
                if (mode != mode_t::follower) {
                    return;
                }
                if (last_heard_from_leader >= now - election_timeout_min_ms * 1000) {
                    return;
                }
                /* If we're not a voting member, we shouldn't begin an election. If
                `ps.snapshot_configuration` is empty, we assume we're not a voting
                member. */
                if (!static_cast<bool>(ps.snapshot_configuration)) {
                    return;
                }
                if (!get_configuration_ref().is_valid_leader(this_member_id)) {
                    return;
                }
                /* Begin an election */
                guarantee(!leader_drainer.has());
                leader_drainer.init(new auto_drainer_t);
                coro_t::spawn_sometime(boost::bind(
                    &raft_member_t<state_t, change_t>::candidate_and_leader_coro,
                    this,
                    mutex_acq.release(),
                    auto_drainer_t::lock_t(leader_drainer.get())));
            } catch (interrupted_exc_t) {
                /* If `keepalive.get_drain_signal()` fires, the `raft_member_t` is being
                destroyed, so don't start an election. */
            }
        });
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::update_term(
        raft_term_t new_term,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);

    guarantee(new_term > ps.current_term);
    ps.current_term = new_term;

    /* In Figure 2, `votedFor` is defined as "candidateId that received vote in
    current term (or null if none)". So when the current term changes, we have to
    update `voted_for`. */
    ps.voted_for = nil_uuid();

    /* The same logic applies to `current_term_leader_id`. */
    current_term_leader_id = nil_uuid();
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::update_commit_index(
        raft_log_index_t new_commit_index,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);

    guarantee(new_commit_index > commit_index);
    commit_index = new_commit_index;

    /* Raft paper, Figure 2: "If commitIndex > lastApplied: increment lastApplied, apply
    log[lastApplied] to state machine" */
    while (last_applied < commit_index) {
        ++last_applied;
        if (ps.log.get_entry_ref(last_applied).type ==
                raft_log_entry_t<change_t>::type_t::regular) {
            guarantee(initialized_cond.is_pulsed());
            state_machine.apply_atomic_op([&](state_t *state) -> bool {
                state->apply_change(*ps.log.get_entry_ref(last_applied).change);
                return true;
            });
        }
    }

    /* Take a snapshot as described in Section 7. We can snapshot any time we like; this
    implementation currently snapshots after every change. If the `state_t` ever becomes
    large enough that flushing it to disk is expensive, we could delay snapshotting until
    many changes have accumulated. */
    if (last_applied > ps.log.prev_index) {
        state_machine.apply_read([&](const state_t *state) {
            ps.snapshot_state = *state;
        });
        for (raft_log_index_t i = ps.log.prev_index + 1; i <= last_applied; ++i) {
            if (ps.log.get_entry_ref(i).type ==
                    raft_log_entry_t<change_t>::type_t::configuration) {
                ps.snapshot_configuration =
                    boost::optional<raft_complex_config_t>(
                        *ps.log.get_entry_ref(i).configuration);
            }
        } 
        /* This automatically updates `ps.log.prev_index` and `ps.log.prev_term`, which
        are equivalent to the "last included index" and "last included term" described in
        Section 7 of the Raft paper. */
        ps.log.delete_entries_to(last_applied);
    }

    /* This wakes up instances of `leader_send_updates()`, which will push the updated
    commit index to replicas if necessary. It also wakes up
    `candidate_and_leader_coro()`; if we've committed a joint consensus,
    `candidate_and_leader_coro()` will put an entry in the log for the second phase of
    the configuration change. */
    guarantee(mode == mode_t::leader || update_watchers.empty());
    for (cond_t *cond : update_watchers) {
        cond->pulse_if_not_already_pulsed();
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::leader_update_match_index(
        std::map<raft_member_id_t, raft_log_index_t> *match_index,
        raft_member_id_t key,
        raft_log_index_t new_value,
        const new_mutex_acq_t *mutex_acq,
        signal_t *interruptor) {
    guarantee(mode == mode_t::leader);
    mutex_acq->assert_is_holding(&mutex);

    auto it = match_index->find(key);
    guarantee(it != match_index->end());
    guarantee(it->second <= new_value);
    it->second = new_value;

    raft_complex_config_t configuration = get_configuration_ref();

    /* Raft paper, Figure 2: "If there exists an N such that N > commitIndex, a majority
    of matchIndex[i] >= N, and log[N].term == currentTerm: set commitIndex = N" */
    raft_log_index_t new_commit_index = commit_index;
    for (raft_log_index_t i = commit_index + 1; i <= ps.log.get_latest_index(); ++i) {
        if (ps.log.get_entry_term(i) != ps.current_term) {
            continue;
        }
        std::set<raft_member_id_t> approving_members;
        for (auto const &pair : *match_index) {
            if (pair.second >= i) {
                approving_members.insert(pair.first);
            }
        }
        if (configuration.is_quorum(approving_members)) {
            new_commit_index = i;
        } else {
            /* If this log entry isn't approved, no later log entry can possibly be
            approved either */
            break;
        }
    }
    if (new_commit_index != commit_index) {
        update_commit_index(new_commit_index, mutex_acq);
        interface->write_persistent_state(ps, interruptor);
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::candidate_or_leader_become_follower(
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::candidate || mode == mode_t::leader);
    guarantee(leader_drainer.has());

    /* This will interrupt `candidate_and_leader_coro()` and block until it exits */
    leader_drainer.reset();

    /* `candidate_and_leader_coro()` should have reset `mode` when it exited */
    guarantee(mode == mode_t::follower);
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::candidate_and_leader_coro(
        new_mutex_acq_t *mutex_acq_on_heap,
        auto_drainer_t::lock_t leader_keepalive) {
    scoped_ptr_t<new_mutex_acq_t> mutex_acq(mutex_acq_on_heap);
    guarantee(mode == mode_t::follower);
    mutex_acq->assert_is_holding(&mutex);
    leader_keepalive.assert_is_holding(leader_drainer.get());

    /* Raft paper, Section 5.2: "To begin an election, a follower increments its current
    term and transitions to candidate state." */
    update_term(ps.current_term + 1, mutex_acq.get());
    /* `update_term()` changed `ps`, but we don't need to flush to stable storage
    immediately, because `candidate_run_election()` will do it. */

    mode = mode_t::candidate;

    /* While we're candidate or leader, we'll never update our log in response to an RPC.
    */
    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* This `try` block is to catch `interrupted_exc_t` */
    try {
        /* The first election won't necessarily succeed. So we loop until either we
        become leader or we are interrupted. */
        while (true) {
            signal_timer_t election_timeout;
            election_timeout.start(
                /* Choose a random timeout between `election_timeout_min_ms` and
                `election_timeout_max_ms`. */
                election_timeout_min_ms +
                    randuint64(election_timeout_max_ms - election_timeout_min_ms));
            bool elected = candidate_run_election(&mutex_acq, &election_timeout,
                leader_keepalive.get_drain_signal());
            if (elected) {
                guarantee(mode == mode_t::leader);
                break;   /* exit the loop */
            } else {
                guarantee(election_timeout.is_pulsed());
                /* Raft paper, Section 5.2: "... if many followers become candidates at
                the same time, votes could be split so that no candidate obtains a
                majority. When this happens, each candidate will time out and start a new
                election by incrementing its term and initiating another round of
                RequestVote RPCs." */
                update_term(ps.current_term + 1, mutex_acq.get());
                /* Go around the `while`-loop again. */
            }
        }

        /* We got elected. */
        guarantee(mode == mode_t::leader);

        guarantee(current_term_leader_id.is_nil());
        current_term_leader_id = this_member_id;

        /* Raft paper, Section 5.3: "When a leader first comes to power, it initializes
        all nextIndex values to the index just after the last one in its log"
        It's important to do this before we append the no-op to the log, so that we
        replicate the no-op on the first try instead of having to try twice. */
        raft_log_index_t initial_next_index = ps.log.get_latest_index() + 1;

        /* Raft paper, Section 8: "[Raft has] each leader commit a blank no-op entry into
        the log at the start of its term."
        This is to ensure that we'll commit any entries that are possible to commit,
        since we can't commit entries from earlier terms except by committing an entry
        from our own term. */
        {
            raft_log_entry_t<change_t> new_entry;
            new_entry.type = raft_log_entry_t<change_t>::type_t::noop;
            new_entry.term = ps.current_term;
            leader_append_log_entry(new_entry, mutex_acq.get(),
                leader_keepalive.get_drain_signal());
        }

        /* `match_indexes` corresponds to the `matchIndex` array described in Figure 2 of
        the Raft paper. */
        std::map<raft_member_id_t, raft_log_index_t> match_indexes;

        /* `update_drainers` contains an `auto_drainer_t` for each running instance of
        `leader_send_updates()`. */
        std::map<raft_member_id_t, scoped_ptr_t<auto_drainer_t> > update_drainers;

        while (true) {

            /* This will spawn instances of `leader_send_updates()`. The instances of
            `leader_send_updates()` will then take care of sending append-entries RPCs to
            the other members of the cluster, including the initial empty append-entries
            RPC. */
            leader_spawn_update_coros(
                initial_next_index,
                &match_indexes,
                &update_drainers,
                mutex_acq.get());

            /* Check if there is a committed joint consensus configuration but no entry
            in the log for the second phase of the config change. If this is the case,
            then we will add an entry. Also check if we have just committed a config in
            which we are no longer leader, in which case we need to step down. */
            leader_continue_reconfiguration(
                mutex_acq.get(),
                leader_keepalive.get_drain_signal());

            /* Block until either a new entry is appended to the log or a new entry is
            committed. If a new entry is appended to the log, we might need to re-run
            `leader_spawn_update_coros()`; if a new entry is committed, we might need to
            re-run `leader_continue_reconfiguration()`. */
            {
                cond_t cond;
                set_insertion_sentry_t<cond_t *> cond_sentry(&update_watchers, &cond);

                /* Release the mutex while blocking so that we can receive RPCs */
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();

                wait_interruptible(&cond, leader_keepalive.get_drain_signal());

                /* Reacquire the mutex. Note that if `wait_interruptible()` throws
                `interrupted_exc_t`, we won't reacquire the mutex. This is by design;
                probably `candidate_or_leader_become_follower()` is holding the mutex. */
                mutex_acq.init(
                    new new_mutex_acq_t(&mutex, leader_keepalive.get_drain_signal()));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
            }
        }

    } catch (interrupted_exc_t) {
        /* This means either the `raft_member_t` is being destroyed, or we were told to
        revert to follower state. In either case we do the same thing. */
    }

    mode = mode_t::follower;
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::candidate_run_election(
        scoped_ptr_t<new_mutex_acq_t> *mutex_acq,
        signal_t *cancel_signal,
        signal_t *interruptor) {
    (*mutex_acq)->assert_is_holding(&mutex);
    guarantee(mode == mode_t::candidate);

    raft_complex_config_t configuration = get_configuration_ref();

    std::set<raft_member_id_t> votes_for_us;
    cond_t we_won_the_election;

    /* Raft paper, Section 5.2: "It then votes for itself." */
    ps.voted_for = this_member_id;
    votes_for_us.insert(this_member_id);
    if (configuration.is_quorum(votes_for_us)) {
        /* In some degenerate cases, like if the cluster has only one node, then
        our own vote might be enough to be elected. */
        we_won_the_election.pulse();
    }

    /* Flush to stable storage so we don't forget we voted for ourself. I'm not sure if
    this is strictly necessary. */
    interface->write_persistent_state(ps, interruptor);

    /* Raft paper, Section 5.2: "[The candidate] issues RequestVote RPCs in parallel to
    each of the other servers in the cluster." */
    std::set<raft_member_id_t> peers = configuration.get_all_members();
    scoped_ptr_t<auto_drainer_t> request_vote_drainer(new auto_drainer_t);
    for (const raft_member_id_t &peer : peers) {
        if (peer == this_member_id) {
            /* Don't request a vote from ourself */
            continue;
        }

        auto_drainer_t::lock_t request_vote_keepalive(request_vote_drainer.get());
        coro_t::spawn_sometime([this, &configuration, &votes_for_us,
                &we_won_the_election, peer,
                request_vote_keepalive /* important to capture */]() {
            try {
                /* Send the RPC and wait for a response */
                raft_term_t reply_term;
                bool reply_vote_granted;

                while (true) {
                    /* Don't bother trying to send an RPC until the peer is connected */
                    interface->get_connected_members()->run_until_satisfied(
                        [&](const std::set<raft_member_id_t> &connected) {
                            return connected.count(peer) == 1;
                        }, request_vote_keepalive.get_drain_signal());

                    /* It's safe to pass `ps.*` by reference to `send_request_vote_rpc()`
                    because they can't change while we're in candidate state, and the
                    only way for us to exit candidate state is for `request_vote_drainer`
                    to be destroyed first. */
                    bool ok = interface->send_request_vote_rpc(
                        peer,
                        ps.current_term,
                        this_member_id,
                        ps.log.get_latest_index(),
                        ps.log.get_entry_term(ps.log.get_latest_index()),
                        request_vote_keepalive.get_drain_signal(),
                        &reply_term,
                        &reply_vote_granted);
                    if (!ok) {
                        /* Raft paper, Section 5.1: "Servers retry RPCs if they do not
                        receive a response in a timely manner" */
                        continue;
                    }

                    new_mutex_acq_t mutex_acq_2(
                        &mutex, request_vote_keepalive.get_drain_signal());
                    DEBUG_ONLY_CODE(check_invariants(&mutex_acq_2));
                    /* We might soon be leader, but we shouldn't be leader quite yet */
                    guarantee(mode == mode_t::candidate);

                    if (candidate_or_leader_note_term(reply_term, &mutex_acq_2)) {
                        /* We got a response with a higher term than our current term.
                        `candidate_and_leader_coro()` will be interrupted soon. */
                        return;
                    }

                    if (reply_vote_granted) {
                        votes_for_us.insert(peer);
                        /* Raft paper, Section 5.2: "A candidate wins an election if it
                        receives votes from a majority of the servers in the full cluster
                        for the same term." */
                        if (configuration.is_quorum(votes_for_us)) {
                            we_won_the_election.pulse_if_not_already_pulsed();
                        }
                        break;
                    }

                    /* If they didn't grant the vote, just keep pestering them until they
                    do or we are interrupted. This is necessary because followers will
                    temporarily reject votes if they have heard from a current leader
                    recently, so they might reject a vote and then later accept it in the
                    same term. */
                }

            } catch (interrupted_exc_t) {
                /* Ignore since we're in a coroutine */
                return;
            }
        });
    }

    /* Release the mutex while we block. This is important because we might get an
    append-entries RPC, and we need to let it acquire the mutex so it can call
    `candidate_or_leader_become_follower()` if necessary. */
    mutex_acq->reset();

    /* Raft paper, Section 5.2: "A candidate continues in this state until one of three
    things happens: (a) it wins the election, (b) another server establishes itself as
    leader, or (c) a period of time goes by with no winner."
    If (b) or (c) happens then `candidate_and_leader_coro()` will pulse `interruptor`. */
    wait_any_t waiter(&we_won_the_election, cancel_signal);
    wait_interruptible(&waiter, interruptor);

    mutex_acq->init(new new_mutex_acq_t(&mutex, interruptor));
    DEBUG_ONLY_CODE(check_invariants(mutex_acq->get()));

    if (we_won_the_election.is_pulsed()) {
        /* Make sure that any ongoing request-vote coroutines stop, because they assert
        `mode == mode_t::candidate`. */
        request_vote_drainer.reset();
        mode = mode_t::leader;
        return true;
    } else {
        return false;
    }
    /* Here `request_vote_drainer` is destroyed, so we won't exit until all of the
    coroutines we spawned have stopped, regardless of whether we won the election or not.
    */
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::leader_spawn_update_coros(
        raft_log_index_t initial_next_index,
        std::map<raft_member_id_t, raft_log_index_t> *match_indexes,
        std::map<raft_member_id_t, scoped_ptr_t<auto_drainer_t> > *update_drainers,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::leader);

    /* Calculate the new configuration */
    raft_complex_config_t configuration = get_configuration_ref();
    std::set<raft_member_id_t> peers = configuration.get_all_members();

    /* Spawn coroutines as necessary */
    for (const raft_member_id_t &peer : peers) {
        if (peer == this_member_id) {
            /* We don't need to send updates to ourself */
            continue;
        }
        if (update_drainers->count(peer) == 1) {
            /* We already have a coroutine for this one */
            continue;
        }
        /* Raft paper, Figure 2: "[matchIndex is] initialized to 0" */
        match_indexes->insert(std::make_pair(peer, 0));

        scoped_ptr_t<auto_drainer_t> update_drainer(new auto_drainer_t);
        auto_drainer_t::lock_t update_keepalive(update_drainer.get());
        update_drainers->insert(std::make_pair(peer, std::move(update_drainer)));
        coro_t::spawn_sometime(boost::bind(&raft_member_t::leader_send_updates, this,
            peer,
            initial_next_index,
            match_indexes,
            update_keepalive));
    }

    /* Kill coroutines as necessary */
    for (auto it = update_drainers->begin(); it != update_drainers->end();
            ++it) {
        raft_member_id_t peer = it->first;
        if (peers.count(peer) == 1) {
            /* `peer` is still a member of the cluster, so the coroutine should stay
            alive */
            continue;
        }
        auto jt = it;
        ++jt;
        /* This will block until the update coroutine stops */
        update_drainers->erase(it);
        it = jt;
        match_indexes->erase(peer);
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::leader_send_updates(
        const raft_member_id_t &peer,
        raft_log_index_t initial_next_index,
        std::map<raft_member_id_t, raft_log_index_t> *match_indexes,
        auto_drainer_t::lock_t update_keepalive) {
    try {
        guarantee(peer != this_member_id);
        guarantee(mode == mode_t::leader);
        guarantee(match_indexes->count(peer) == 1);
        scoped_ptr_t<new_mutex_acq_t> mutex_acq(
            new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
        DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

        /* `next_index` corresponds to the entry for this peer in the `nextIndex` map
        described in Figure 2 of the Raft paper. */
        raft_log_index_t next_index = initial_next_index;

        /* `member_commit_index` tracks the last value of `commit_index` we sent to the
        member. It doesn't correspond to anything in the Raft paper. This implementation
        deviates from the Raft paper slightly in that when the leader commits a log
        entry, it immediately sends an append-entries RPC so that clients can commit the
        log entry too, instead of waiting for the next heartbeat to tell the clients it's
        time to commit. This allows the latency of a transaction to be much shorter than
        the heartbeat interval. */
        raft_log_index_t member_commit_index = 0;

        /* Raft paper, Figure 2: "Upon election: send initial empty AppendEntries RPCs
        (heartbeat) to each server"
        Setting `send_even_if_empty` will ensure that we send an RPC immediately. */
        bool send_even_if_empty = true;

        /* This implementation deviates slightly from the Raft paper in that the initial
        message may not be an empty append-entries RPC. Because `leader_send_updates()`
        runs in its own coroutine, it's possible that entries may be appended to the log
        (or even committed, although this is unlikely) between when we are elected as
        leader and when we go to send initial messages. So we could send an
        append-entries RPC that isn't empty, or even an install-snapshot RPC. This is OK
        because if our implementation is a candidate and receives an install-snapshot RPC
        for the current term, it will revert to follower just as if it had received an
        append-entries RPC.

        This is in a loop because after we send the initial RPC, we'll block until it's
        time to send another update, and then go around the loop again. */
        while (true) {
            signal_timer_t heartbeat_timer;
            heartbeat_timer.start(heartbeat_interval_ms);

            /* First, wait for the peer to be connected; there's no point in trying to
            send an RPC if the peer isn't even connected. */
            DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
            mutex_acq.reset();
            interface->get_connected_members()->run_until_satisfied(
                [&](const std::set<raft_member_id_t> &peers) {
                    return peers.count(peer) == 1;
                }, update_keepalive.get_drain_signal());
            mutex_acq.init(
                new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
            DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

            if (next_index <= ps.log.prev_index) {
                /* The peer's log ends before our log begins. So we have to send an
                install-snapshot RPC instead of an append-entries RPC. */

                /* Make local copies of the RPC parameters before releasing the lock.
                This is important because `send_install_snapshot_rpc()` takes some
                parameters by reference, and the originals might change. */
                raft_term_t term_to_send = ps.current_term;
                raft_log_index_t last_included_index_to_send = ps.log.prev_index;
                raft_term_t last_included_term_to_send = ps.log.prev_term;
                guarantee(static_cast<bool>(ps.snapshot_state));
                state_t snapshot_state_to_send = *ps.snapshot_state;
                guarantee(static_cast<bool>(ps.snapshot_configuration));
                raft_complex_config_t snapshot_configuration_to_send =
                    *ps.snapshot_configuration;

                raft_term_t reply_term;
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();
                bool ok = interface->send_install_snapshot_rpc(
                    peer,
                    term_to_send,
                    this_member_id,
                    last_included_index_to_send,
                    last_included_term_to_send,
                    snapshot_state_to_send,
                    snapshot_configuration_to_send,
                    update_keepalive.get_drain_signal(),
                    &reply_term);
                mutex_acq.init(
                    new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

                if (!ok) {
                    /* Raft paper, Section 5.1: "Servers retry RPCs if they do not
                    receive a response in a timely manner"
                    This implementation deviates from the Raft paper slightly in that we
                    don't retry the exact same RPC necessarily. If the snapshot has been
                    updated, we could send a newer snapshot on our next try. */
                    continue;
                }

                if (candidate_or_leader_note_term(reply_term, mutex_acq.get())) {
                    /* We got a reply with a higher term than our term.
                    `candidate_and_leader_coro()` will be interrupted soon. */
                    return;
                }

                next_index = last_included_index_to_send + 1;
                leader_update_match_index(
                    match_indexes,
                    peer,
                    last_included_index_to_send,
                    mutex_acq.get(),
                    update_keepalive.get_drain_signal());
                send_even_if_empty = false;

            } else if (next_index <= ps.log.get_latest_index() ||
                    member_commit_index < commit_index ||
                    send_even_if_empty) {
                /* The peer's log ends right where our log begins, or in the middle of
                our log. Send an append-entries RPC. */

                /* Just like above, copy everything we need to local variables before
                releasing the lock */
                raft_term_t term_to_send = ps.current_term;
                raft_log_t<change_t> entries_to_send = ps.log;
                if (next_index > ps.log.prev_index + 1) {
                    entries_to_send.delete_entries_to(next_index - 1);
                }
                raft_log_index_t leader_commit_to_send = commit_index;

                raft_term_t reply_term;
                bool reply_success;
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();
                bool ok = interface->send_append_entries_rpc(
                    peer,
                    term_to_send,
                    this_member_id,
                    entries_to_send,
                    leader_commit_to_send,
                    update_keepalive.get_drain_signal(),
                    &reply_term,
                    &reply_success);
                mutex_acq.init(
                    new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

                if (!ok) {
                    /* Raft paper, Section 5.1: "Servers retry RPCs if they do not
                    receive a response in a timely manner"
                    This implementation deviates from the Raft paper slightly in that we
                    don't retry the exact same RPC necessarily. If more entries have been
                    added to the log, the RPC we retry will include those new entries; or
                    if the peer falls too far behind, we might send an install-snapshot
                    RPC next time. */
                    continue;
                }

                if (candidate_or_leader_note_term(reply_term, mutex_acq.get())) {
                    /* We got a reply with a higher term than our term.
                    `candidate_and_leader_coro()` will be interrupted soon. */
                    return;
                }

                if (reply_success) {
                    /* Raft paper, Figure 2: "If successful: update nextIndex and
                    matchIndex for follower */
                    next_index = entries_to_send.get_latest_index() + 1;
                    if (match_indexes->at(peer) < entries_to_send.get_latest_index()) {
                        leader_update_match_index(
                            match_indexes,
                            peer,
                            entries_to_send.get_latest_index(),
                            mutex_acq.get(),
                            update_keepalive.get_drain_signal());
                    }
                    member_commit_index = leader_commit_to_send;
                } else {
                    /* Raft paper, Section 5.3: "After a rejection, the leader decrements
                    nextIndex and retries the AppendEntries RPC. */
                    --next_index;
                }
                send_even_if_empty = false;

            } else {
                guarantee(next_index == ps.log.get_latest_index() + 1);
                guarantee(member_commit_index == commit_index);
                /* OK, the peer is completely up-to-date. Wait until either an entry is
                appended to the log; our `commit_index` advances; or the heartbeat
                interval elapses, and then go around the loop again. */

                /* `cond` will be pulsed when an entry is appended to the log or our
                `commit_index` advances. */
                cond_t cond;
                set_insertion_sentry_t<cond_t *> cond_sentry(&update_watchers, &cond);

                wait_any_t waiter(&cond, &heartbeat_timer);

                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();
                wait_interruptible(&waiter, update_keepalive.get_drain_signal());
                mutex_acq.init(
                    new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

                if (heartbeat_timer.is_pulsed()) {
                    send_even_if_empty = true;
                }
            }
        }
    } catch (interrupted_exc_t) {
        /* The leader interrupted us. This could be because the `raft_member_t` is being
        destroyed; because the leader is no longer leader; or because a config change
        removed `peer` from the cluster. In any case, we just return. */
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::leader_continue_reconfiguration(
        const new_mutex_acq_t *mutex_acq,
        signal_t *interruptor) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::leader);
    /* Check if we recently committed a joint consensus configuration, or a configuration
    in which we are no longer leader. */
    guarantee(static_cast<bool>(ps.snapshot_configuration));
    raft_complex_config_t committed_config = *ps.snapshot_configuration;
    for (raft_log_index_t i = ps.log.prev_index + 1; i <= commit_index; ++i) {
        if (ps.log.get_entry_ref(i).type ==
                raft_log_entry_t<change_t>::type_t::configuration) {
            committed_config = *ps.log.get_entry_ref(i).configuration;
        }
    }
    if (committed_config.is_joint_consensus()) {
        /* OK, we recently committed a joint consensus configuration. Check if the second
        phase of the reconfiguration isn't already in the log. */
        bool no_other_config = true;
        for (raft_log_index_t i = commit_index + 1; i <= ps.log.get_latest_index(); ++i) {
            const raft_log_entry_t<change_t> &entry = ps.log.get_entry_ref(i);
            if (entry.type == raft_log_entry_t<change_t>::type_t::configuration) {
                guarantee(!entry.configuration->is_joint_consensus());
                no_other_config = false;
                break;
            }
        }
        if (no_other_config) {
            /* OK, the second phase of the reconfiguration isn't already in the log. */

            /* Raft paper, Section 6: "Once C_old,new has been committed... it is now
            safe for the leader to create a log entry describing C_new and replicate it
            to the cluster. */
            raft_complex_config_t new_config;
            new_config.config = *committed_config.new_config;

            raft_log_entry_t<change_t> new_entry;
            new_entry.type = raft_log_entry_t<change_t>::type_t::configuration;
            new_entry.configuration = boost::optional<raft_complex_config_t>(new_config);
            new_entry.term = ps.current_term;

            leader_append_log_entry(new_entry, mutex_acq, interruptor);
        }
    } else if (!committed_config.is_valid_leader(this_member_id)) {
        /* Raft paper, Section 6: "...the leader steps down (returns to follower state)
        once it has committed the C_new log entry."
        `candidate_or_leader_note_term()` isn't designed for the purpose of making us
        intentionally step down, but it contains all the right logic. This has a side
        effect of incrementing `current_term`, but I don't think that's a problem. */
        candidate_or_leader_note_term(ps.current_term + 1, mutex_acq);
    }
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::candidate_or_leader_note_term(
        raft_term_t term, const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode != mode_t::follower);
    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        raft_term_t local_current_term = ps.current_term;
        /* We have to spawn this in a separate coroutine because
        `candidate_or_leader_become_follower()` blocks until
        `candidate_and_leader_coro()` exits, so calling
        `candidate_or_leader_become_follower()` directly would cause a deadlock. */
        auto_drainer_t::lock_t keepalive(drainer.get());
        coro_t::spawn_sometime([this, local_current_term, term,
                keepalive /* important to capture */]() {
            try {
                new_mutex_acq_t mutex_acq_2(&mutex, keepalive.get_drain_signal());
                DEBUG_ONLY_CODE(check_invariants(&mutex_acq_2));
                /* Check that term hasn't already been updated between when
                `candidate_or_leader_note_term()` was called and when this coroutine ran.
                */
                if (ps.current_term == local_current_term) {
                    this->update_term(term, &mutex_acq_2);
                    this->candidate_or_leader_become_follower(&mutex_acq_2);
                    interface->write_persistent_state(ps, keepalive.get_drain_signal());
                }
                DEBUG_ONLY_CODE(check_invariants(&mutex_acq_2));
            } catch (interrupted_exc_t) {
                /* If `keepalive.get_drain_signal()` is pulsed, then the `raft_member_t`
                is being destroyed, so it doesn't matter if we update our term; the
                `candidate_or_leader_coro` will stop on its own. */
            }
        });
        return true;
    } else {
        return false;
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::leader_append_log_entry(
        const raft_log_entry_t<change_t> &log_entry,
        const new_mutex_acq_t *mutex_acq,
        signal_t *interruptor) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::leader);
    guarantee(log_entry.term == ps.current_term);

    /* Raft paper, Section 5.3: "The leader appends the command to its log as a new
    entry..." */
    ps.log.append(log_entry);

    interface->write_persistent_state(ps, interruptor);

    /* Raft paper, Section 5.3: "...then issues AppendEntries RPCs in parallel to each of
    the other servers to replicate the entry."
    This wakes up sleeping instances of `leader_send_updates()`, which will do the actual
    work of sending append-entries RPCs. It also wakes up `candidate_and_leader_coro()`,
    which will spawn or kill instances of `leader_send_updates()` as necessary if the
    configuration has been changed. */
    for (cond_t *c : update_watchers) {
        c->pulse_if_not_already_pulsed();
    }
}

template<class state_t, class change_t>
const raft_complex_config_t &raft_member_t<state_t, change_t>::get_configuration_ref() {
    guarantee(static_cast<bool>(ps.snapshot_configuration), "We haven't been "
        "initialized yet, so we should be a non-voting member; why are we calling "
            "get_configuration_ref()?");
    /* Raft paper, Section 6: "a server always uses the latest configuration in its log,
    regardless of whether the entry is committed"
    We recompute this on-the-fly every time it's needed. We could cache it, but we'd have
    to be careful about invalidating the cache, so it's not worth doing unless there's a
    good reason. The log will usually be very short so it shouldn't be a performance
    issue. */
    const raft_complex_config_t *c = &*ps.snapshot_configuration;
    for (raft_log_index_t i = ps.log.prev_index + 1;
            i <= ps.log.get_latest_index(); ++i) {
        const raft_log_entry_t<change_t> &entry = ps.log.get_entry_ref(i);
        if (entry.type == raft_log_entry_t<change_t>::type_t::configuration) {
            c = &*entry.configuration;
        }
    }
    return *c;
}

#endif /* CLUSTERING_GENERIC_RAFT_CORE_TCC_ */

