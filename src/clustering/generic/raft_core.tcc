// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_CORE_TCC_
#define CLUSTERING_GENERIC_RAFT_CORE_TCC_

/* RSI: Argue for generalizations being safe. */

/* `temporarily_release_mutex_acq_t` releases the given `new_mutex_acq_t` in its
constructor, but recreates it in its destructor. */
class temporarily_release_mutex_acq_t {
public:
    temporarily_release_mutex_acq_t(
            new_mutex_t *mutex, scoped_ptr_t<new_mutex_acq_t> *acq) :
            mutex_(mutex), acq_(acq) {
        guarantee(acq_->has());
        *acq_->assert_is_holding(mutex);
        acq_->reset();
    }
    ~temporarily_release_mutex_acq_t() {
        acq_->init(new new_mutex_acq_t(mutex));
    }
private:
    new_mutex_t *mutex;
    scoped_ptr_t<new_mutex_acq_t> *acq;
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::propose_change_if_leader(
        const change_t &change,
        signal_t *interruptor) {
    assert_thread();
    guarantee(change.get_change_type() == raft_change_type_t::regular);
    new_mutex_acq_t mutex_acq(&mutex);

    if (mode != mode_t::leader) {
        return false;
    }

    if (!get_state_including_log().consider_change(change)) {
        return false;
    }

    propose_change_internal(change, &mutex_acq);

    /* RSI: Wait for change to be committed */
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::propose_config_change_if_leader(
        const change_t &change,
        signal_t *interruptor) {
    assert_thread();
    guarantee(change.get_change_type() == raft_change_type_t::config_first);
    new_mutex_acq_t mutex_acq(&mutex);

    if (mode != mode_t::leader) {
        return false;
    }

    if (!get_state_including_log().consider_change(change)) {
        return false;
    }

    if (state_machine.in_config_change() ||
            get_state_including_log().in_config_change()) {
        /* We forbid starting a new config change before the old one is completely
        finished. The Raft paper doesn't explicitly say anything about multiple
        interleaved configuration changes; but it's safer and simpler to forbid multiple
        interleaved configuration changes. */
    }

    propose_change_internal(change, &mutex_acq);

    /* RSI: Wait for change to be committed */
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_request_vote(
        raft_term_t term,
        const raft_member_id_t &candidate_id,
        raft_log_index_t last_log_index,
        raft_term_t last_log_term,
        signal_t *interruptor,
        raft_term_t *term_out,
        bool *vote_granted_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex);

    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        update_term(term, &mutex_acq);
        if (mode != mode_t::follower) {
            become_follower(&mutex_acq);
        }
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 2: "Reply false if term < currentTerm" */
    if (term < ps.current_term) {
        *term_out = ps.current_term;
        *vote_granted_out = false;
        return;
    }

    /* Sanity checks, not explicitly described in the Raft paper. */
    guarantee(candidate_id != member_id, "We shouldn't be requesting a vote from "
        "ourself.");
    if (mode != mode_t::follower) {
        guarantee(voted_for == member_id, "We should have voted for ourself already.");
    }

    /* Raft paper, Figure 2: "If votedFor is null or candidateId, and candidate's log is
    at least as up-to-date as receiver's log, grant vote */

    /* So if `voted_for` is neither `nil_uuid()` nor `candidate_id`, we don't grant the
    vote */
    if (!ps.voted_for.is_nil() && ps.voted_for != candidate_id) {
        *term_out = ps.current_term;
        *vote_granted_out = false;
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
        return;
    }

    ps.voted_for = candidate_id;

    /* Raft paper, Figure 2: "Persistent state [is] updated on stable storage before
    responding to RPCs" */
    interface->write_persistent_state(ps, interruptor);

    *term_out = ps.current_term;
    *vote_granted_out = true;
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::on_install_snapshot(
        raft_term_t term,
        const raft_member_id_t &leader_id,
        raft_log_index_t last_included_index,
        raft_term_t last_included_term,
        const state_t &snapshot,
        signal_t *interruptor,
        raft_term_t *term_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex);

    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        update_term(term, &mutex_acq);
        if (mode != mode_t::follower) {
            become_follower(&mutex_acq);
        }
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 13: "Reply immediately if term < currentTerm" */
    if (term < ps.current_term) {
        *term_out = ps.current_term;
        return;
    }

    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* Raft paper, Figure 13: "Save snapshot file"
    Remember that `log.prev_log_index` and `log.prev_log_term` correspond to the snapshot
    metadata. */
    ps.snapshot = snapshot;
    ps.log.prev_log_index = last_included_index;
    ps.log.prev_log_term = last_included_term;

    /* Raft paper, Figure 13: "If existing log entry has same index and term as
    snapshot's last included entry, retain log entries following it and reply" */
    if (last_included_index <= ps.log.prev_log_index) {
        /* The proposed snapshot starts at or before our current snapshot. It's
        impossible to check if an existing log entry has the same index and term because
        the snapshot's last included entry is before our most recent entry. But if that's
        the case, we don't need this snapshot, so we can safely ignore it. */
        *term_out = ps.current_term;
        return;
    } else if (last_included_index <= ps.log.get_latest_index() &&
            ps.log.get_entry_term(last_included_index) == last_included_term) {
        *term_out = ps.current_term;
        return;
    }

    /* Raft paper, Figure 13: "Discard the entire log" */
    ps.log.entries.clear();

    /* Raft paper, Figure 13: "Reset state machine using snapshot contents" */
    state_machine = ps.snapshot;
    commit_index = last_applied = ps.log.prev_log_index;

    /* Recall that `this_term_leader_id` is set to `nil_uuid()` if we haven't seen a
    leader yet this term. */
    if (this_term_leader_id.is_nil()) {
        this_term_leader_id = leader_id;
    } else {
        /* Raft paper, Section 5.2: "at most one candidate can win the election for a
        particular term" */
        guarantee(this_term_leader_id == leader_id);
    }

    /* Raft paper, Figure 2: "Persistent state [is] updated on stable storage before
    responding to RPCs" */
    interface->write_persistent_state(ps, interruptor);

    *term_out = ps.current_term;
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
    new_mutex_acq_t mutex_acq(&mutex);

    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        update_term(term, &mutex_acq);
        if (mode != mode_t::follower) {
            become_follower(&mutex_acq);
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
        return;
    }

    guarantee(term == ps.current_term);   /* sanity check */

    /* Raft paper, Section 5.2: "While waiting for votes, a candidate may receive an
    AppendEntries RPC from another server claiming to be leader. If the leader's term
    (included in its RPC) is at least as large as the candidate's current term, then the
    candidate recognizes the leader as legitimate and returns to follower state. If the
    term in the RPC is smaller than the candidate's current term, then the candidate
    rejects the RPC and continues in candidate state." */
    if (mode == mode_t::candidate) {
        become_follower(&mutex_acq);
    }

    /* Raft paper, Section 5.2: "at most one candidate can win the election for a
    particular term"
    If we're leader, then we won the election, so it makes no sense for us to receive an
    RPC from another member that thinks it's leader. */
    guarantee(mode != mode_t::leader);

    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* Raft paper, Figure 2: "Reply false if log doesn't contain an entry at prevLogIndex
    whose term matches prevLogTerm" */
    if (entries.prev_log_index > ps.log.get_latest_index() ||
            ps.log.get_entry_term(prev_log_index) != entries.prev_log_term) {
        *term_out = current_term;
        *success_out = false;
        return;
    }

    /* Raft paper, Figure 2: "If an existing entry conflicts with a new one (same index
    but different terms), delete the existing entry and all that follow it" */
    for (raft_log_index_t i = entries.prev_log_index + 1;
            i <= min(ps.log.get_latest_index(), entries.get_latest_index());
            ++i) {
        if (log.get_entry_term(i) != entries.get_entry_term(i)) {
            log.delete_entries_from(i);
            break;
        }
    }

    /* Raft paper, Figure 2: "Append any new entries not already in the log" */
    for (raft_log_index_t i = ps.log.get_latest_index() + 1;
            i <= entries.get_latest_index();
            ++i) {
        ps.log.append(entries.get_entry(i));
    }

    /* Raft paper, Figure 2: "If leaderCommit > commitIndex, set commitIndex = min(
    leaderCommit, index of last new entry)" */
    while (leader_commit > commit_index) {
        update_commit_index(min(leader_commit, entries.get_latest_index()), &mutex_acq);
    }

    /* Recall that `this_term_leader_id` is set to `nil_uuid()` if we haven't seen a
    leader yet this term. */
    if (this_term_leader_id.is_nil()) {
        this_term_leader_id = leader_id;
    } else {
        /* Raft paper, Section 5.2: "at most one candidate can win the election for a
        particular term" */
        guarantee(this_term_leader_id == leader_id);
    }

    /* Raft paper, Figure 2: "Persistent state [is] updated on stable storage before
    responding to RPCs" */
    interface->write_persistent_state(ps, interruptor);

    *term_out = current_term;
    *success_out = true;
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

    /* The same logic applies to `this_term_leader_id`. */
    this_term_leader_id = nil_uuid();

    /* RSI: Ensure that we flush to stable storage, because we updated `ps.voted_for` and
    `ps.current_term` */
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
        guarantee(state_machine.consider_change(ps.log.get_entry(last_applied).first),
            "We somehow committed a change that's not valid for the state.");
        state_machine.apply_change(ps.log.get_entry(last_applied).first);
    }

    /* Take a snapshot as described in Section 7. We can snapshot any time we like; this
    implementation currently snapshots after every change. If the `state_t` ever becomes
    large enough that flushing it to disk is expensive, we could delay snapshotting until
    many changes have accumulated. */
    if (last_applied > ps.log.prev_log_index) {
        ps.snapshot = state_machine;
        /* This automatically updates `ps.log.prev_log_index` and `ps.log.prev_log_term`,
        which are equivalent to the "last included index" and "last included term"
        described in Section 7 of the Raft paper. */
        ps.log.delete_entries_to(last_applied);
    }

    /* RSI: Do second phase of a config change if necessary. */

    /* RSI: Ensure that we flush to stable storage. */
}

/* RSI: Ensure that we flush to stable storage whenever we make changes to ps, even
outside of an RPC reply method. */

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::update_match_index(
        std::map<raft_member_id_t, raft_log_index_t> *match_index,
        raft_member_id_t key,
        raft_log_index_t new_value,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);

    auto it = match_index->find(key)
    guarantee(it != match_index->end());
    guarantee(it->second <= new_value);
    it->second = new_value;

    /* Raft paper, Section 6: "a server always uses the latest configuration in its log,
    regardless of whether the entry is committed" */
    state_t state_for_config = get_state_including_log();

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
        if (state_for_config.is_quorum_for_change(
                approving_members, ps.log.get_entry(i).first)) {
            new_commit_index = i;
        } else {
            /* If this log entry isn't approved, no later log entry can possibly be
            approved either */
            break;
        }
    }
    if (new_commit_index != commit_index) {
        update_commit_index(new_commit_index, mutex_acq);
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::become_follower(const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::candidate || mode == mode_t::leader);
    guarantee(leader_drainer.has());

    /* This will interrupt `run_candidate_and_leader()` and block until it exits */
    leader_drainer.reset();

    /* `run_candidate_and_leader()` should have reset `mode` when it exited */
    guarantee(mode == mode_t::follower);
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::become_candidate(
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::follower);
    guarantee(!leader_drainer.has());
    leader_drainer.init(new auto_drainer_t);
    cond_t pulse_when_done_with_setup;
    coro_t::spawn_sometime(boost::bind(&raft_member_t::run_candidate_and_leader, this,
        mutex_acq,
        &pulse_when_done_with_setup,
        auto_drainer_t::lock_t(leader_drainer.get())));
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::run_candidate_and_leader(
        const new_mutex_acq_t *mutex_acq_for_setup,
        cond_t *pulse_when_done_with_setup,        
        auto_drainer_t::lock_t leader_keepalive) {
    guarantee(mode == mode_t::follower);
    keepalive.assert_is_holding(drainer.get());
    mutex_acq_for_setup->assert_is_holding(&mutex);

    /* Raft paper, Section 5.2: "To begin an election, a follower increments its current
    term and transitions to candidate state." */
    update_term(ps.current_term + 1, mutex_acq_for_setup);
    mode = mode_t::candidate;

    /* While we're candidate or leader, we'll never update our log in response to an RPC.
    */
    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    pulse_when_done_with_setup->pulse();   /* let `become_candidate()` return */
    mutex_acq_for_setup = NULL;
    scoped_ptr_t<new_mutex_acq_t> mutex_acq(new new_mutex_acq_t(&mutex));

    /* This `try` block is to catch `interrupted_exc_t` */
    try {
        /* The first election won't necessarily succeed. So we loop until either we
        become leader or we are interrupted. */
        while (true) {
            /* RSI: Randomize election timeouts */
            signal_timer_t election_timeout(100);   /* RSI: tune this number */
            election_timeout.start();
            try {
                wait_any_t interruptor(
                    &election_timeout,
                    leader_keepalive.get_drain_signal());
                run_election(&mutex_acq, &interruptor);
                /* If `run_election()` returns, then we won the election */
                guarantee(mode == mode_t::leader);
                break;   /* exit the loop */
            } catch (interrupted_exc_t) {
                if (leader_keepalive.get_drain_signal()->is_pulsed()) {
                    /* This can happen for two reasons. One is that the `raft_member_t`
                    is being destroyed. The other is that another server established
                    itself as leader so `become_follower()` was called. In either case,
                    we exit. */
                    throw interrupted_exc_t();
                } else {
                    guarantee(election_is_stalled.is_pulsed());
                    /* Raft paper, Section 5.2: "... if many followers become candidates
                    at the same time, votes could be split so that no candidate obtains a
                    majority. When this happens, each candidate will time out and start a
                    new election by incrementing its term and initiating another round of
                    RequestVote RPCs." */
                    update_term(ps.current_term + 1, mutex_acq.get());
                    /* Go around the `while`-loop again. */
                }
            }
        }

        /* We got elected. */
        guarantee(mode == mode_t::leader);

        /* `match_indexes` corresponds to the `matchIndex` array described in Figure 2 of
        the Raft paper. */
        std::map<raft_member_id_t, raft_log_index_t> match_indexes;

        std::map<raft_member_id_t, scoped_ptr_t<auto_drainer_t> > update_drainers;
        while (true) {
            /* Raft paper, Section 6: "a server always uses the latest configuration in
            its log, regardless of whether the entry is committed" */
            state_t state_for_config = get_state_including_log();
            std::set<raft_member_id_t> peers = state_for_config.get_all_members();

            /* Spawn or kill update coroutines to reflect changes in `state_for_config`
            since the last time we went around the loop */
            for (const raft_member_id_t &peer : peers) {
                if (peer == member_id) {
                    /* We don't need to send updates to ourself */
                    continue;
                }
                if (update_drainers.count(member_id) == 1) {
                    /* We already have a coroutine for this one */
                    continue;
                }
                /* Raft paper, Figure 2: "[matchIndex is] initialized to 0" */
                match_indexes.emplace(peer, 0);

                scoped_ptr_t<auto_drainer_t> update_drainer(new auto_drainer_t);
                auto_drainer_t::lock_t update_keepalive(update_drainer.get());
                update_drainers.emplace(peer, std::move(update_drainer));
                coro_t::spawn_sometime(boost::bind(&raft_member_t::run_updates, this,
                    peer,
                    /* Raft paper, Section 5.3: "When a leader first comes to power, it
                    initializes all nextIndex values to the index just after the last one
                    in its log" */
                    ps.log.get_latest_index() + 1,
                    &match_indexes,
                    update_keepalive));
            }
            for (auto it = update_drainers.begin(); it != update_drainers.end(); ++it) {
                raft_member_id_t peer = it->first;
                if (peers.count(peer) == 1) {
                    /* This peer is still valid, so the coroutine should stay alive */
                    continue;
                }
                auto jt = it;
                ++jt;
                /* This will block until the update coroutine stops */
                update_drainers.erase(it);
                it = jt;
                match_indexes.erase(peer);
            }

            /* Block until the next update. */
            /* TODO: As an optimization, we could wake only on updates that change the
            config instead of just any old update. */
            {
                cond_t cond;
                set_insertion_sentry_t<cond_t *> cond_sentry(&proposal_watchers, &cond);

                /* We release the mutex while blocking so that we can receive RPCs */
                temporarily_release_new_mutex_acq_t mutex_unacq(&mutex, &mutex_acq);

                wait_interruptible(&cond, leader_keepalive.get_drain_signal());
            }
        }

    } catch (interrupted_exc_t) {
        /* This means either the `raft_member_t` is being destroyed, or we were told to
        revert to follower state. In either case we do the same thing. */
    }

    mode = mode_t::follower;
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::run_election(
        /* Note that `run_election()` may temporarily release `mutex_acq`, but it will
        always be holding the lock when `run_election()` exits. */
        scoped_ptr_t<new_mutex_acq_t> *mutex_acq,
        signal_t *interruptor) {
    (*mutex_acq)->assert_is_holding(&mutex);
    guarantee(mode == mode_t::candidate);

    /* Raft paper, Section 6: "a server always uses the latest configuration in
    its log, regardless of whether the entry is committed" */
    state_t state_for_config = get_state_including_log();

    std::set<raft_member_id_t> votes_for_us;
    cond_t we_won_the_election;

    /* Raft paper, Section 5.2: "It then votes for itself." */
    ps.voted_for = member_id;
    votes_for_us.insert(member_id);
    if (state_for_config.is_quorum_for_all(votes_for_us)) {
        /* In some degenerate cases, like if the cluster has only one node, then
        our own vote might be enough to be elected. */
        we_won_the_election.pulse();
    }

    /* Raft paper, Section 5.2: "[The candidate] issues RequestVote RPCs in
    parallel to each of the other servers in the cluster." */
    std::set<raft_member_id_t> peers = state_for_config.get_all_members();
    auto_drainer_t request_vote_drainer;
    for (const raft_member_id_t &peer : peers) {
        if (peer == member_id) {
            /* Don't request a vote from ourself */
            continue;
        }

        auto_drainer_t::lock_t request_vote_keepalive(&request_vote_drainer);
        coro_t::spawn_sometime([this, &state_for_config, &votes_for_us,
                &we_won_the_election, &we_lost_the_election, peer,
                request_vote_keepalive /* important to capture */]() {
            try {
                /* Send the RPC and wait for a response */
                raft_term_t reply_term;
                bool reply_vote_granted;
                interface->send_request_vote_rpc(
                    peer,
                    ps.current_term,
                    member_id,
                    ps.log.get_latest_index(),
                    ps.log.get_entry_term(ps.log.get_latest_index()),
                    request_vote_keepalive.get_drain_signal(),
                    &reply_term,
                    &reply_vote_granted);

                new_mutex_acq_t mutex_acq(&mutex);

                if (note_term_as_leader(reply_term, &mutex_acq)) {
                    /* We got a response with a higher term than our current term.
                    `run_candidate_or_leader()` will be interrupted soon. */
                    return;
                }

                if (vote_granted) {
                    votes.insert(peer);
                    /* Raft paper, Section 5.2: "A candidate wins an election if it
                    receives votes from a majority of the servers in the full cluster for
                    the same term."
                    Recall that we implement a generalization of Raft where the set of
                    members that vote on a change is not necessarily all the members in
                    the cluster. Because of this, we must use `is_quorum_for_all()`
                    instead of checking if we have more than half of the members
                    numerically. */
                    if (state_for_config.is_quorum_for_all(votes)) {
                        we_won_the_election->pulse_if_not_already_pulsed();
                    }
                }

            } catch (interrupted_exc_t) {
                /* Ignore since we're in a coroutine */
                return;
            }
        });
    }

    {
        /* Release the mutex while we block. This is important because we might get an
        append-entries RPC, and we need to let it acquire the mutex so it can call
        `become_follower()` if necessary. */
        temporarily_release_new_mutex_acq_t mutex_unacq(&mutex, mutex_acq);

        /* Raft paper, Section 5.2: "A candidate continues in this state until one of
        three things happens: (a) it wins the election, (b) another server establishes
        itself as leader, or (c) a period of time goes by with no winner."
        If (b) or (c) happens then `run_candidate_or_leader()` will pulse `interruptor`.
        */
        wait_interruptible(&we_won_the_election, interruptor);
    }

    mode = mode_t::leader;

    /* Here `request_vote_drainer` is destroyed, so we won't exit until all of the
    coroutines we spawned have stopped. */
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::run_updates(
        const raft_member_id_t &peer,
        raft_log_index_t initial_next_index,
        std::map<raft_member_id_t, raft_log_index_t> *match_indexes,
        auto_drainer_t::lock_t update_keepalive) {
    /* RSI: Send initial empty RPC */
    try {
        guarantee(peer != member_id);
        guarantee(mode == mode_t::leader);

        /* `next_index` corresponds to the entry for this peer in the `nextIndex` map
        described in Figure 2 of the Raft paper. */
        raft_log_index_t next_index = initial_next_index;

        scoped_ptr_t<new_mutex_acq_t> mutex_acq(new new_mutex_acq_t(&mutex));
        while (true) {
            if (next_index <= ps.log.prev_log_index) {
                /* The peer's log ends before our log begins. So we have to send an
                install-snapshot RPC instead of an append-entries RPC. */

                /* Make local copies of the RPC parameters before releasing the lock.
                This is important because `send_install_snapshot_rpc()` takes some
                parameters by reference, and the originals might change. */
                raft_term_t term_to_send = ps.current_term;
                raft_log_index_t last_included_index_to_send = ps.log.prev_log_index;
                raft_term_t last_included_term_to_send = ps.log.prev_log_term;
                state_t snapshot_to_send = ps.snapshot;

                raft_log_term_t reply_term;
                {
                    /* Release the lock while we block */
                    temporarily_release_new_mutex_acq_t mutex_unacq(&mutex, mutex_acq);
                    interface->send_install_snapshot_rpc(
                        peer,
                        term_to_send,
                        member_id,
                        last_included_index_to_send,
                        last_included_term_to_send,
                        snapshot_to_send,
                        update_keepalive.get_drain_signal(),
                        &reply_term);
                }
                if (note_term_as_leader(reply_term, mutex_acq.get())) {
                    /* We got a reply with a higher term than our term.
                    `run_candidate_and_leader()` will be interrupted soon. */
                    return;
                }

                next_index = ps.log.prev_log_index + 1;
                *match_index = ps.log.prev_log_index;

            } else if (next_index <= ps.log.get_latest_index()) {
                /* The peer's log ends right where our log begins, or in the middle of
                our log. Send an append-entries RPC. */

                /* Just like above, copy everything we need to local variables before
                releasing the lock */
                raft_term_t term_to_send = ps.current_term;
                raft_log_t<change_t> entries_to_send = ps.log;
                if (next_index > ps.log.prev_log_index + 1) {
                    entries_to_send.delete_entries_to(next_index - 1);
                }
                raft_log_index_t leader_commit_to_send = commit_index;
                
                raft_log_term_t reply_term;
                bool reply_success;
                {
                    /* Release the lock while we block */
                    temporarily_release_new_mutex_acq_t mutex_unacq(&mutex, mutex_acq);
                    interface->send_append_entries_rpc(
                        peer,
                        term_to_send,
                        member_id,
                        entries_to_send,
                        leader_commit_to_send,
                        update_keepalive.get_drain_signal(),
                        &reply_term,
                        &reply_success);
                }
                if (note_term_as_leader(reply_term, mutex_acq.get())) {
                    /* We got a reply with a higher term than our term.
                    `run_candidate_and_leader()` will be interrupted soon. */
                    return;
                }

                if (reply_success) {
                    /* Raft paper, Figure 2: "If successful: update nextIndex and
                    matchIndex for follower */
                    next_index = entries_to_send.get_latest_index() + 1;
                    if (match_indexes.at(peer) < entries_to_send.get_latest_index()) {
                        update_match_index(
                            match_indexes,
                            peer,
                            entries_to_send.get_latest_index(),
                            mutex_acq.get());
                    }
                } else {
                    /* Raft paper, Section 5.3: "After a rejection, the leader decrements
                    nextIndex and retries the AppendEntries RPC. */
                    --next_index;
                }
            } else {
                guarantee(next_index == ps.log.get_latest_index() + 1);
                /* OK, the peer is completely up-to-date. Wait until we get more changes
                from the client and then go around the loop again. */
                /* RSI wait for changes */
                /* RSI send an extra message if there are uncommitted changes */
            }
        }
    } catch (interrupted_exc_t) {
        /* The leader interrupted us. This could be because the `raft_member_t` is being
        destroyed; because the leader is no longer leader; or because a config change
        removed `peer` from the cluster. In any case, we just return. */
    }
}

template<class state_t, class change_t>
bool raft_member_t<state_t, change_t>::note_term_as_leader(
        raft_term_t term, const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps.current_term) {
        raft_term_t local_current_term = ps.current_term;
        /* We have to spawn this in a separate coroutine because
        `become_follower()` blocks until `run_candidate_and_leader()` exits,
        so calling `become_follower()` directly would cause a deadlock. */
        auto_drainer_t::lock_t keepalive(&drainer);
        coro_t::spawn_sometime([this, local_current_term, term,
                keepalive /* important to capture */]() {
            new_mutex_acq_t mutex_acq_2(&mutex);
            /* Check that term hasn't already been updated between when
            `note_term_as_leader()` was called and when this coroutine ran */
            if (ps.current_term == local_current_term) {
                this->update_term(term, &mutex_acq_2);
                this->become_follower(&mutex_acq_2);
            }
        });
        return true;
    } else {
        return false;
    }
}

template<class state_t, class change_t>
void raft_member_t<state_t, change_t>::propose_change_internal(
        const change_t &change,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->assert_is_holding(&mutex);
    guarantee(mode == mode_t::leader);

    /* Raft paper, Section 5.3: "The leader appends the commend to its log as a new
    entry..." */
    ps.log.append(std::make_pair(change, ps.current_term));

    /* Raft paper, Section 5.3: "...then issues AppendEntries RPCs in parallel to each of
    the other servers to replicate the entry."
    This wakes up sleeping instances of `run_update()`, which will do the actual work of
    sending append-entries RPCs. */
    for (cond_t *c : proposal_watchers) {
        c->pulse_if_not_already_pulsed();
    }
}

template<class state_t, class change_t>
state_t raft_member_t<state_t, change_t>::get_state_including_log() {
    log_mutex_acq->assert_is_holding(&log_mutex);
    state_t s = state_machine;
    for (raft_log_index_t i = last_applied+1; i <= ps.log.get_latest_index(); ++i) {
        guarantee(s.consider_change(ps.log.get_entry(i).first),
            "We somehow got a change that's not valid for the state.");
        s.apply_change(ps.log.get_entry(i).first);
    }
    return s;
}

#endif /* CLUSTERING_GENERIC_RAFT_CORE_TCC_ */

