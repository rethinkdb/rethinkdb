// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_CORE_TCC_
#define CLUSTERING_GENERIC_RAFT_CORE_TCC_

#include "clustering/generic/raft_core.hpp"

#include "arch/compiler.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/exponential_backoff.hpp"
#include "containers/map_sentries.hpp"
#include "logger.hpp"

/* After finding myself repeatedly adding debug logging to Raft and then tearing it out
when I didn't need it anymore, I automated the process. Define `ENABLE_RAFT_DEBUG` to
print messages to stderr whenever important events happen in Raft. */
// #define ENABLE_RAFT_DEBUG
#ifdef ENABLE_RAFT_DEBUG
#include "debug.hpp"
inline std::string show_member_id(const raft_member_id_t &mid) {
    return uuid_to_str(mid.uuid).substr(0, 4);
}
#define RAFT_DEBUG(...) debugf(__VA_ARGS__)
#define RAFT_DEBUG_THIS(...) debugf("%s: %s", \
    show_member_id(this_member_id).c_str(), \
    strprintf(__VA_ARGS__).c_str())
#define RAFT_DEBUG_VAR
#else
#define RAFT_DEBUG(...) ((void)0)
#define RAFT_DEBUG_THIS(...) ((void)0)
#define RAFT_DEBUG_VAR UNUSED
#endif /* ENABLE_RAFT_DEBUG */

template<class state_t>
raft_persistent_state_t<state_t>
raft_persistent_state_t<state_t>::make_initial(
        const state_t &initial_state,
        const raft_config_t &initial_config) {
    raft_persistent_state_t<state_t> ps;
    /* The Raft paper indicates that `current_term` should be initialized to 0 and the
    first log index is 1. */
    ps.current_term = 0;
    ps.voted_for = raft_member_id_t();
    ps.snapshot_state = initial_state;
    raft_complex_config_t complex_config;
    complex_config.config = initial_config;
    ps.snapshot_config = complex_config;
    ps.log.prev_index = 0;
    ps.log.prev_term = 0;
    ps.commit_index = 0;
    return ps;
}

template<class state_t>
raft_member_t<state_t>::raft_member_t(
        const raft_member_id_t &_this_member_id,
        raft_storage_interface_t<state_t> *_storage,
        raft_network_interface_t<state_t> *_network,
        const std::string &_log_prefix,
        const raft_start_election_immediately_t start_election_immediately) :
    this_member_id(_this_member_id),
    storage(_storage),
    network(_network),
    log_prefix(_log_prefix),
    /* Restore state machine from snapshot. */
    committed_state(state_and_config_t(
        ps().log.prev_index,
        ps().snapshot_state,
        ps().snapshot_config)),
    /* We'll update `latest_state` with the contents of the log shortly */
    latest_state(committed_state.get_ref()),
    /* Raft paper, Section 5.2: "When servers start up, they begin as followers." */
    mode(mode_t::follower),
    /* These must be false initially because we're a follower. */
    readiness_for_change(false),
    readiness_for_config_change(false),
    drainer(new auto_drainer_t),
    /* Initialize `watchdog` in the `NOT_TRIGGERED` state, so that it will wait for an
    election timeout to elapse before starting a new election, unless
    _start_election_immediately is true, in which case we start an election
    immediately. */
    watchdog(new watchdog_timer_t(
        election_timeout_min_ms, election_timeout_max_ms,
        [this]() { this->on_watchdog(); },
        start_election_immediately == raft_start_election_immediately_t::YES
            ? watchdog_timer_t::state_t::TRIGGERED
            : watchdog_timer_t::state_t::NOT_TRIGGERED)),
    /* Initialize `watchdog_leader_only` in the `TRIGGERED` state, so that we will accept
    valid RequestVote RPCs. */
    watchdog_leader_only(new watchdog_timer_t(
        /* Note that we wait exactly `election_timeout_min_ms`. See the note in
        `on_request_vote_rpc()` for an explanation of why this is. */
        election_timeout_min_ms, election_timeout_min_ms,
        nullptr, watchdog_timer_t::state_t::TRIGGERED)),
    connected_members_subs(
        new watchable_map_t<raft_member_id_t, boost::optional<raft_term_t> >::all_subs_t(
            network->get_connected_members(),
            std::bind(&raft_member_t::on_connected_members_change, this, ph::_1, ph::_2)
            ))
{
    new_mutex_acq_t mutex_acq(&mutex);
    /* Finish initializing `latest_state` */
    latest_state.apply_atomic_op([&](state_and_config_t *s) -> bool {
        this->apply_log_entries(
            s, this->ps().log, s->log_index + 1, this->ps().log.get_latest_index());
        return !this->ps().log.entries.empty();
    });
    /* Finish initializing `committed_state` */
    committed_state.apply_atomic_op([&](state_and_config_t *s) -> bool {
        this->apply_log_entries(
            s, this->ps().log, s->log_index + 1, this->ps().commit_index);
        return true;
    });
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t>
raft_member_t<state_t>::~raft_member_t() {
    assert_thread();
    /* Now that the destructor has been called, we can safely assume that our public
    methods will not be called. */

    /* In addition unsubscribe from member change events, so we're sure to not get
    those either while destructing. */
    connected_members_subs.reset();

    new_mutex_acq_t mutex_acq(&mutex);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    /* Kill `candidate_and_leader_coro()`, if it's running. We have to destroy this
    before destroying the watchdogs because it may hold `watchdog_timer_t::blocker_t`s
    that point to the watchdogs. */
    if (mode != mode_t::follower) {
        candidate_or_leader_become_follower(&mutex_acq);
    }

    /* Destroy the watchdogs so that they don't start any new actions while we're
    cleaning up. We have to do this before destroying `drainer` because they might
    otherwise try to lock `drainer`. We have to destroy the blockers before destroying
    the watchdogs because they hold pointers to the watchdogs. */
    virtual_heartbeat_watchdog_blockers[0].reset();
    virtual_heartbeat_watchdog_blockers[1].reset();
    watchdog.reset();
    watchdog_leader_only.reset();

    /* Destroy `drainer` to kill any miscellaneous coroutines that aren't related to
    `candidate_and_leader_coro()`. */
    drainer.reset();

    /* All the coroutines have stopped, so we can start calling member destructors. */
}

template<class state_t>
raft_persistent_state_t<state_t> raft_member_t<state_t>::get_state_for_init(
        const change_lock_t &change_lock_proof) {
    change_lock_proof.mutex_acq.guarantee_is_holding(&mutex);
    /* This implementation deviates from the Raft paper in that we initialize new peers
    joining the cluster by copying the log, snapshot, etc. from an existing peer, instead
    of starting the new peer with a blank state. */
    raft_persistent_state_t<state_t> ps_copy = ps();
    /* Clear `voted_for` since the newly-joining peer will not have voted for anything */
    ps_copy.voted_for = raft_member_id_t();
    return ps_copy;
}

template<class state_t>
raft_member_t<state_t>::change_lock_t::change_lock_t(
        raft_member_t *parent,
        signal_t *interruptor) :
    mutex_acq(&parent->mutex, interruptor) {
    DEBUG_ONLY_CODE(parent->check_invariants(&mutex_acq));
}

template<class state_t>
raft_member_t<state_t>::change_token_t::change_token_t(
        raft_member_t *_parent,
        raft_log_index_t _index,
        bool _is_config) :
    is_config(_is_config), sentry(&_parent->change_tokens, _index, this) { }

template<class state_t>
scoped_ptr_t<typename raft_member_t<state_t>::change_token_t>
raft_member_t<state_t>::propose_change(
        change_lock_t *change_lock,
        const typename state_t::change_t &change) {
    assert_thread();
    change_lock->mutex_acq.guarantee_is_holding(&mutex);

    if (!readiness_for_change.get_ref()) {
        return scoped_ptr_t<change_token_t>();
    }
    guarantee(mode == mode_t::leader);

    /* We have to construct the change token before we create the log entry. If we are
    the only member of the cluster, then `leader_append_log_entry()` will commit the
    entry as well as create it; by creating the change token first, we ensure that it
    will get notified if this happens. */
    raft_log_index_t log_index = ps().log.get_latest_index() + 1;
    scoped_ptr_t<change_token_t> change_token(
        new change_token_t(this, log_index, false));

    raft_log_entry_t<state_t> new_entry;
    new_entry.type = raft_log_entry_type_t::regular;
    new_entry.change = boost::optional<typename state_t::change_t>(change);
    new_entry.term = ps().current_term;

    leader_append_log_entry(new_entry, &change_lock->mutex_acq);
    guarantee(ps().log.get_latest_index() == log_index);

    DEBUG_ONLY_CODE(check_invariants(&change_lock->mutex_acq));
    return change_token;
}

template<class state_t>
scoped_ptr_t<typename raft_member_t<state_t>::change_token_t>
raft_member_t<state_t>::propose_config_change(
        change_lock_t *change_lock,
        const raft_config_t &new_config) {
    assert_thread();
    change_lock->mutex_acq.guarantee_is_holding(&mutex);

    if (!readiness_for_config_change.get_ref()) {
        return scoped_ptr_t<change_token_t>();
    }
    guarantee(mode == mode_t::leader);
    guarantee(!committed_state.get_ref().config.is_joint_consensus());
    guarantee(!latest_state.get_ref().config.is_joint_consensus());

    raft_log_index_t log_index = ps().log.get_latest_index() + 1;
    scoped_ptr_t<change_token_t> change_token(
        new change_token_t(this, log_index, true));

    /* Raft paper, Section 6: "... the cluster first switches to a [joint consensus
    configuration]" */
    raft_complex_config_t new_complex_config;
    new_complex_config.config = latest_state.get_ref().config.config;
    new_complex_config.new_config = boost::optional<raft_config_t>(new_config);

    raft_log_entry_t<state_t> new_entry;
    new_entry.type = raft_log_entry_type_t::config;
    new_entry.config = boost::optional<raft_complex_config_t>(new_complex_config);
    new_entry.term = ps().current_term;

    leader_append_log_entry(new_entry, &change_lock->mutex_acq);
    guarantee(ps().log.get_latest_index() == log_index);

    /* Now that we've put a config entry into the log, we'll have to flip
    `readiness_for_config_change` */
    update_readiness_for_change();
    guarantee(!readiness_for_config_change.get_ref());

    /* When the joint consensus is committed, `leader_continue_reconfiguration()`
    will take care of initiating the second step of the reconfiguration. */

    DEBUG_ONLY_CODE(check_invariants(&change_lock->mutex_acq));
    return change_token;
}

template<class state_t>
scoped_ptr_t<typename raft_member_t<state_t>::change_token_t>
raft_member_t<state_t>::propose_noop(
        change_lock_t *change_lock) {
    assert_thread();
    change_lock->mutex_acq.guarantee_is_holding(&mutex);

    if (!readiness_for_change.get_ref()) {
        return scoped_ptr_t<change_token_t>();
    }
    guarantee(mode == mode_t::leader);

    raft_log_index_t log_index = ps().log.get_latest_index() + 1;
    scoped_ptr_t<change_token_t> change_token(
        new change_token_t(this, log_index, false));

    raft_log_entry_t<state_t> new_entry;
    new_entry.type = raft_log_entry_type_t::noop;
    new_entry.term = ps().current_term;

    leader_append_log_entry(new_entry, &change_lock->mutex_acq);
    guarantee(ps().log.get_latest_index() == log_index);

    DEBUG_ONLY_CODE(check_invariants(&change_lock->mutex_acq));
    return change_token;
}

template<class state_t>
void raft_member_t<state_t>::on_rpc(
        const raft_rpc_request_t<state_t> &request,
        raft_rpc_reply_t *reply_out) {
    typedef raft_rpc_request_t<state_t> req_t;
    if (const typename req_t::request_vote_t *request_vote_req =
            boost::get<typename req_t::request_vote_t>(&request.request)) {
        raft_rpc_reply_t::request_vote_t rep;
        on_request_vote_rpc(*request_vote_req, &rep);
        reply_out->reply = rep;
    } else if (const typename req_t::install_snapshot_t *install_snapshot_req =
            boost::get<typename req_t::install_snapshot_t>(&request.request)) {
        raft_rpc_reply_t::install_snapshot_t rep;
        on_install_snapshot_rpc(*install_snapshot_req, &rep);
        reply_out->reply = rep;
    } else if (const typename req_t::append_entries_t *append_entries_req =
            boost::get<typename req_t::append_entries_t>(&request.request)) {
        raft_rpc_reply_t::append_entries_t rep;
        on_append_entries_rpc(*append_entries_req, &rep);
        reply_out->reply = rep;
    } else {
        unreachable();
    }
}

#ifndef NDEBUG
template<class state_t>
void raft_member_t<state_t>::check_invariants(
        const std::set<raft_member_t<state_t> *> &members) {
    /* We acquire each member's mutex to ensure we don't catch them in invalid states */
    std::vector<scoped_ptr_t<new_mutex_acq_t> > mutex_acqs;
    for (raft_member_t<state_t> *member : members) {
        signal_timer_t timeout;
        timeout.start(10000);
        try {
            scoped_ptr_t<new_mutex_acq_t> mutex_acq(
                new new_mutex_acq_t(&member->mutex, &timeout));
            /* Check each member's invariants individually */
            member->check_invariants(mutex_acq.get());
            mutex_acqs.push_back(std::move(mutex_acq));
        } catch (const interrupted_exc_t &) {
            crash("Raft member mutex is gridlocked");
        }
    }

    {
        /* Raft paper, Figure 3: "Election Safety: at most one leader can be elected in
        a given term" */
        std::set<raft_term_t> claimed;
        for (raft_member_t<state_t> *member : members) {
            if (member->mode == mode_t::leader) {
                auto pair = claimed.insert(member->ps().current_term);
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
        for (raft_member_t<state_t> *m1 : members) {
            for (raft_member_t<state_t> *m2 : members) {
                bool match_so_far = true;
                for (raft_log_index_t i = std::max(m1->ps().log.prev_index + 1,
                                                   m2->ps().log.prev_index + 1);
                        i <= std::min(m1->ps().log.get_latest_index(),
                                      m2->ps().log.get_latest_index());
                        ++i) {
                    raft_log_entry_t<state_t> e1 = m1->ps().log.get_entry_ref(i);
                    raft_log_entry_t<state_t> e2 = m2->ps().log.get_entry_ref(i);
                    if (e1.term == e2.term) {
                        guarantee(e1.type == e2.type,
                            "Log matching property violated");
                        guarantee(e1.change == e2.change,
                            "Log matching property violated");
                        guarantee(e1.config == e2.config,
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

    /* Raft paper, Figure 3: "State Machine Safety: if a server has applied a log entry
    at a given index to its state machine, no other server will ever apply a different
    log entry for the same index."
    Since we snapshot log entries immediately, in practice the part of this that's worth
    testing is that if two members have the same snapshot index then they will have the
    same snapshot contents. */
    for (raft_member_t<state_t> *m1 : members) {
        for (raft_member_t<state_t> *m2 : members) {
            if (m1 <= m2) {
                /* Don't make unnecessary checks */
                continue;
            }
            if (m1->ps().log.prev_index == m2->ps().log.prev_index) {
                guarantee(m1->ps().snapshot_state == m2->ps().snapshot_state);
                guarantee(m1->ps().snapshot_config == m2->ps().snapshot_config);
            }
        }
    }
}
#endif /* NDEBUG */

template<class state_t>
void raft_member_t<state_t>::on_request_vote_rpc(
        const typename raft_rpc_request_t<state_t>::request_vote_t &request,
        raft_rpc_reply_t::request_vote_t *reply_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    /* Raft paper, Section 6: "Servers disregard RequestVote RPCs when they believe a
    current leader exists... Specifically, if a server receives a RequestVote RPC within
    the minimum election timeout of hearing from a current leader, it does not update its
    term or grant its vote."

    We detect whether we've heard from a leader or not by checking if
    `watchdog_leader_only` is triggered or not. This is the purpose of
    `watchdog_leader_only`. This is also the reason why `watchdog_leader_only` is set to
    wait exactly `election_timeout_min_ms` instead of a random period between
    `election_timeout_min_ms` and `election_timeout_max_ms`.

    This implementation deviates from the Raft paper in that a leader will accept a
    RequestVote RPC from the candidate as long as the candidate is in the latest config.
    Normally this should be redundant, because if the candidate is in the latest config
    then the leader should be sending it AppendEntries RPCs, and so the leader would find
    out about the newer term via the AppendEntry RPC reply from the candidate. But this
    implementation sends "virtual heartbeats" instead of real heartbeats, and it does not
    receive replies for its virtual heartbeats. So we need another mechanism for a leader
    to detect when another member has started a new term, and we're using RequestVote
    RPCs for that. */
    if ((mode == mode_t::leader &&
                !latest_state.get_ref().config.is_member(request.candidate_id))
            || (mode == mode_t::follower && watchdog_leader_only->get_state() ==
                watchdog_timer_t::state_t::NOT_TRIGGERED)) {
        RAFT_DEBUG_THIS("RequestVote from %s for %" PRIu64 " ignored (leader exists)\n",
            show_member_id(request.candidate_id).c_str(), request.term);
        reply_out->term = ps().current_term;
        reply_out->vote_granted = false;
        return;
    }

    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (request.term > ps().current_term) {
        if (mode != mode_t::follower) {
            candidate_or_leader_become_follower(&mutex_acq);
        }
        update_term(request.term, raft_member_id_t(), &mutex_acq);
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 2: "Reply false if term < currentTerm" */
    if (request.term < ps().current_term) {
        reply_out->term = ps().current_term;
        reply_out->vote_granted = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    /* Sanity checks, not explicitly described in the Raft paper. */
    guarantee(request.candidate_id != this_member_id, "We shouldn't be requesting a "
        "vote from ourself.");
    if (mode != mode_t::follower) {
        guarantee(ps().voted_for == this_member_id, "We should have voted for ourself "
            "already.");
    }

    /* Raft paper, Figure 2: "If votedFor is null or candidateId, and candidate's log is
    at least as up-to-date as receiver's log, grant vote */

    /* So if `voted_for` is neither `nil_uuid()` nor `candidate_id`, we don't grant the
    vote */
    if (!ps().voted_for.is_nil() && ps().voted_for != request.candidate_id) {
        reply_out->term = ps().current_term;
        reply_out->vote_granted = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    /* Raft paper, Section 5.4.1: "Raft determines which of two logs is more up-to-date
    by comparing the index and term of the last entries in the logs. If the logs have
    last entries with different terms, then the log with the later term is more
    up-to-date. If the logs end with the same term, then whichever log is longer is more
    up-to-date." */
    bool candidate_is_at_least_as_up_to_date =
        request.last_log_term > ps().log.get_entry_term(ps().log.get_latest_index()) ||
        (request.last_log_term == ps().log.get_entry_term(ps().log.get_latest_index()) &&
            request.last_log_index >= ps().log.get_latest_index());
    if (!candidate_is_at_least_as_up_to_date) {
        RAFT_DEBUG_THIS("RequestVote from %s for %" PRIu64 " ignored (not up-to-date)\n",
            show_member_id(request.candidate_id).c_str(), request.term);
        reply_out->term = ps().current_term;
        reply_out->vote_granted = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }
    guarantee(ps().log.get_latest_index() >= ps().commit_index);
    guarantee(request.last_log_index >= ps().commit_index);

    RAFT_DEBUG_THIS("RequestVote from %s for %" PRIu64 " granted\n",
            show_member_id(request.candidate_id).c_str(), request.term);

    storage->write_current_term_and_voted_for(
        ps().current_term, request.candidate_id);

    /* Raft paper, Section 5.2: "A server remains in follower state as long as it
    receives valid RPCs from a leader or candidate."
    So candidate RPCs are sufficient to delay the watchdog timer. It's also important
    that we don't delay the watchdog timer for RPCs that we reject because they aren't
    up-to-date; otherwise, you can get a situation where there is only one cluster member
    that is sufficiently up-to-date to be elected leader, but it never stands for
    election because it keeps hearing from other candidates. This is why this line is
    below the `if (!candidate_is_at_least_as_up_to_date)` instead of above it. */
    watchdog->notify();

    reply_out->term = ps().current_term;
    reply_out->vote_granted = true;

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t>
void raft_member_t<state_t>::on_install_snapshot_rpc(
        const typename raft_rpc_request_t<state_t>::install_snapshot_t &request,
        raft_rpc_reply_t::install_snapshot_t *reply_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    if (!on_rpc_from_leader(request.leader_id, request.term, &mutex_acq)) {
        /* Raft paper, Figure 2: term should be set to "currentTerm, for leader to update
        itself" */
        reply_out->term = ps().current_term;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* This implementation deviates from the Raft paper in the order it does things. The
    Raft paper says that the first step should be to save the snapshot, which for us
    would correspond to setting `ps().snapshot_state` and `ps().snapshot_config`. But
    because we only store one snapshot at a time and require that snapshot to be right
    before the start of the log, that doesn't make sense for us. Instead, we save the
    snapshot if and when we update the log. */

    /* Raft paper, Figure 13: "If existing log entry has same index and term as
    snapshot's last included entry, retain log entries following it and reply" */
    if (request.last_included_index <= ps().log.prev_index) {
        /* The proposed snapshot starts at or before our current snapshot. It's
        impossible to check if an existing log entry has the same index and term because
        the snapshot's last included entry is before our most recent entry. But if that's
        the case, we don't need this snapshot, so we can safely ignore it. */
        if (request.last_included_index == ps().log.prev_index) {
            guarantee(request.last_included_term == ps().log.prev_term, "The entry we "
                "shapshotted at was committed, so we shouldn't be receiving a different "
                "committed entry at the same index.");
        }
        reply_out->term = ps().current_term;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    } else if (request.last_included_index <= ps().log.get_latest_index() &&
            ps().log.get_entry_term(request.last_included_index) ==
                request.last_included_term) {
        /* Raft paper, Section 7: "If ... the follower receives a snapshot that describes
        a prefix of its log (due to retransmission or by mistake), then log entries
        covered by the snapshot are deleted but entries following the snapshot are still
        valid and must be retained.
        Raft paper, Figure 13: "Save snapshot file"
        (We're going slightly out of order, as described above) */
        storage->write_snapshot(
            request.snapshot_state,
            request.snapshot_config,
            false,
            request.last_included_index,
            request.last_included_term,
            std::max(request.last_included_index, ps().commit_index));
        guarantee(ps().log.prev_index == request.last_included_index);
        guarantee(ps().log.prev_term == request.last_included_term);

        /* This implementation deviates from the Raft paper in that we may update the
        state machine in this situation. The paper implies that we should reply without
        updating the state machine in this case, but it's obviously safe to update the
        committed index in this case, so we do that. */
    } else {
        /* Raft paper, Figure 13: "Discard the entire log"
        Raft paper, Figure 13: "Save snapshot file"
        (We're going slightly out of order, as described above) */
        storage->write_snapshot(
            request.snapshot_state,
            request.snapshot_config,
            true,
            request.last_included_index,
            request.last_included_term,
            request.last_included_index);
        guarantee(ps().log.prev_index == request.last_included_index);
        guarantee(ps().log.prev_term == request.last_included_term);

        /* Because we modified `ps().log`, we need to update `latest_state` */
        latest_state.set_value_no_equals(state_and_config_t(
            ps().log.prev_index, ps().snapshot_state, ps().snapshot_config));
    }

    /* Raft paper, Figure 13: "Reset state machine using snapshot contents"
    This conditional doesn't appear in the Raft paper. It will always be true if we
    discarded the entire log, but it will sometimes be false if we only discarded part of
    the log, which is why this implementation needs it.*/
    if (request.last_included_index > committed_state.get_ref().log_index) {
        /* Note that we already updated `ps().commit_index` earlier, as part of the same
        storage write operation that changed the snapshot. */
        guarantee(ps().commit_index == request.last_included_index);
        committed_state.apply_atomic_op([&](state_and_config_t *sc) -> bool {
            sc->log_index = request.last_included_index;
            sc->state = this->ps().snapshot_state;
            sc->config = this->ps().snapshot_config;
            return true;
        });
    }

    reply_out->term = ps().current_term;

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

template<class state_t>
void raft_member_t<state_t>::on_append_entries_rpc(
        const typename raft_rpc_request_t<state_t>::append_entries_t &request,
        raft_rpc_reply_t::append_entries_t *reply_out) {
    assert_thread();
    new_mutex_acq_t mutex_acq(&mutex);
    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));

    if (!on_rpc_from_leader(request.leader_id, request.term, &mutex_acq)) {
        /* Raft paper, Figure 2: term should be set to "currentTerm, for leader to update
        itself" */
        reply_out->term = ps().current_term;
        reply_out->success = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    mutex_assertion_t::acq_t log_mutex_acq(&log_mutex);

    /* Raft paper, Figure 2: "Reply false if log doesn't contain an entry at prevLogIndex
    whose term matches prevLogTerm"
    The Raft paper doesn't explicitly say what to do if our snapshot is later than
    `prevLogIndex`. But by the Leader Completeness Property, we know that the log entry
    would have matched if it had not been snapshotted. */
    if (request.entries.prev_index > ps().log.get_latest_index() ||
            (request.entries.prev_index >= ps().log.prev_index &&
                ps().log.get_entry_term(request.entries.prev_index) !=
                    request.entries.prev_term)) {
        reply_out->term = ps().current_term;
        reply_out->success = false;
        DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
        return;
    }

    /* Raft paper, Figure 2: "If an existing entry conflicts with a new one (same index
    but different terms), delete the existing entry and all that follow it" */
    bool conflict = false;
    raft_log_index_t first_nonmatching_index;
    const raft_log_index_t min_index =
        std::max(request.entries.prev_index, ps().log.prev_index) + 1;
    const raft_log_index_t max_index =
        std::min(ps().log.get_latest_index(), request.entries.get_latest_index());
    for (first_nonmatching_index = min_index;
            first_nonmatching_index <= max_index;
            ++first_nonmatching_index) {
        if (ps().log.get_entry_term(first_nonmatching_index) !=
                request.entries.get_entry_term(first_nonmatching_index)) {
            conflict = true;
            break;
        }
    }

    /* Raft paper, Figure 2: "Append any new entries not already in the log"

    Our implementation performs both the truncation of a conflicting log suffix and
    appending any new entries in the same atomic operation through
    `write_log_replace_tail`.

    There are three valid cases that we need to consider:
    * Our own log is an extension of the log the leader sent us, or the logs are equal.
     In that case we will have `conflict == false` and
     `first_nonmatching_index == request.entries.get_latest_index()`
     and `first_nonmatching_index <= ps().log.get_latest_index()`.
     There is nothing to do for us.
    * The log the leader sent us is an extension of our own log.
     We will have `conflict == false` and
     `first_nonmatching_index == ps().log.get_latest_index() + 1`.
     In this case `write_log_replace_tail` is going to append the entries from `request`
     starting at `first_nonmatching_index`.
    * The log the leader sent us and our own log have some matching prefix (potentially
     of length zero), followed by entries that are different between both logs.
     We will have `conflict == true` and
     `ps().log.prev_index < first_nonmatching_index <= ps().log.get_latest_index()`.
     In this case we truncate our own log starting at `first_nonmatching_index`, and
     replace it by the suffix of the `request` log starting at `first_nonmatching_index`.
    */
    if (conflict || first_nonmatching_index > ps().log.get_latest_index()) {
        /* The Leader Completeness property ensures that the leader has all committed log
        entries. We must never truncate our log to become shorter than the current
        `commit_index`. */
        guarantee(first_nonmatching_index > ps().commit_index);
        storage->write_log_replace_tail(
            request.entries, first_nonmatching_index);
    }

    /* Because we modified `ps().log`, we need to update `latest_state`. */
    latest_state.apply_atomic_op([&](state_and_config_t *s) -> bool {
        if (conflict) {
            /* We deleted some log entries and then appended some. Since we have no way
            of reverting a log entry on a `state_and_config_t`, we have to rewind to the
            last committed state and then apply log entries from there. */
            *s = committed_state.get_ref();
        }
        this->apply_log_entries(
            s, this->ps().log, s->log_index + 1, this->ps().log.get_latest_index());
        return true;
    });

    /* Raft paper, Figure 2: "If leaderCommit > commitIndex, set commitIndex = min(
    leaderCommit, index of last new entry)" */
    if (request.leader_commit > committed_state.get_ref().log_index) {
        update_commit_index(
            std::min(request.leader_commit, request.entries.get_latest_index()),
            &mutex_acq);
    }

    reply_out->term = ps().current_term;
    reply_out->success = true;

    DEBUG_ONLY_CODE(check_invariants(&mutex_acq));
}

#ifndef NDEBUG
template<class state_t>
void raft_member_t<state_t>::check_invariants(
        const new_mutex_acq_t *mutex_acq) {
    assert_thread();
    mutex_acq->guarantee_is_holding(&mutex);

    raft_log_index_t commit_index = committed_state.get_ref().log_index;
    guarantee(commit_index == ps().commit_index, "Commit index stored in persistent "
        "state should match commit index stored in committed_state watchable.");

    /* Some of these checks are copied directly from LogCabin's list of invariants. */

    /* Checks related to newly-initialized states */
    if (ps().current_term == 0) {
        guarantee(ps().voted_for.is_nil(), "If we're still in term 0, nobody has "
            "started an election, so we shouldn't have voted for anybody.");
        guarantee(ps().log.entries.empty(), "If we're still in term 0, we shouldn't "
            "have any real log entries.");
        guarantee(ps().log.get_latest_index() == 0, "If we're still in term 0, the log "
            "should be empty");
        guarantee(mode == mode_t::follower, "If we're still in term 0, there shouldn't "
            "have been an election, so we should be follower.");
        guarantee(current_term_leader_id.is_nil(), "Term 0 shouldn't have a leader.");
    }
    guarantee((ps().log.prev_index == 0) == (ps().log.prev_term == 0), "There shouldn't "
        "be any log entries in term 0.");

    /* Checks related to the log */
    raft_term_t latest_term_in_log = ps().log.get_entry_term(ps().log.prev_index);
    for (raft_log_index_t i = ps().log.prev_index + 1;
            i <= ps().log.get_latest_index();
            ++i) {
        guarantee(ps().log.get_entry_ref(i).term != 0,
            "There shouldn't be any log entries in term 0.");
        guarantee(ps().log.get_entry_ref(i).term >= latest_term_in_log,
            "Terms of log entries should monotonically increase");
        const raft_log_entry_t<state_t> &entry = ps().log.get_entry_ref(i);
        latest_term_in_log = entry.term;

        switch (entry.type) {
        case raft_log_entry_type_t::regular:
            guarantee(static_cast<bool>(entry.change), "Regular log entries should "
                "carry changes");
            guarantee(!static_cast<bool>(entry.config), "Regular log entries shouldn't "
                "carry configurations.");
            break;
        case raft_log_entry_type_t::config:
            guarantee(!static_cast<bool>(entry.change), "Configuration log entries "
                "shouldn't carry changes");
            guarantee(static_cast<bool>(entry.config), "Configuration log entries "
                "should carry configurations.");
            break;
        case raft_log_entry_type_t::noop:
            guarantee(!static_cast<bool>(entry.change), "Noop log entries shouldn't "
                "carry changes");
            guarantee(!static_cast<bool>(entry.config), "Noop log entries"
                "shouldn't carry configurations.");
            break;
        default:
            unreachable();
        }
    }
    guarantee(latest_term_in_log <= ps().current_term, "There shouldn't be any log "
        "entries with terms greater than current_term");

    /* Checks related to reconfigurations */
    bool last_is_joint_consensus = ps().snapshot_config.is_joint_consensus();
    for (raft_log_index_t i = ps().log.prev_index + 1;
            i <= ps().log.get_latest_index(); ++i) {
        const raft_log_entry_t<state_t> &entry = ps().log.get_entry_ref(i);
        if (entry.type == raft_log_entry_type_t::config) {
            guarantee(entry.config->is_joint_consensus() != last_is_joint_consensus,
                "Joint consensus configurations should alternate with "
                "non-joint-consensus configurations in the log.");
            last_is_joint_consensus = entry.config->is_joint_consensus();
        }
    }

    /* Checks related to the state machine and commits */
    guarantee(commit_index >= ps().log.prev_index, "All of the log entries in the "
        "snapshot should be committed.");
    guarantee(commit_index <= ps().log.get_latest_index(), "We shouldn't have committed "
        "any entries that aren't in the log yet.");
#ifdef ENABLE_RAFT_DEBUG
    /* This is potentially really slow, so we normally disable it. */
    {
        state_and_config_t s(
            ps().log.prev_index, ps().snapshot_state, ps().snapshot_config);
        apply_log_entries(&s, ps().log, ps().log.prev_index + 1, commit_index);
        guarantee(s.state == committed_state.get_ref().state, "State machine should be "
            "equal to snapshot with log entries applied.");
        guarantee(s.config == committed_state.get_ref().config, "Config stored with "
            "state machine should be equal to snapshot with log entries applied.");
        guarantee(s.log_index == committed_state.get_ref().log_index, "Applying the log "
            "entries up to commit_index to the snapshot should cause its log_index to "
            "be equal to commit_index.");
        apply_log_entries(&s, ps().log, commit_index + 1, ps().log.get_latest_index());
        guarantee(s.state == latest_state.get_ref().state);
        guarantee(s.config == latest_state.get_ref().config);
        guarantee(s.log_index == latest_state.get_ref().log_index);
    }
#endif /* ENABLE_RAFT_DEBUG */

    /* Checks related to the follower/candidate/leader roles */

    guarantee((mode == mode_t::leader) == (current_term_leader_id == this_member_id),
        "mode should say we're leader iff current_term_leader_id says we're leader");
    guarantee(virtual_heartbeat_sender.is_nil() ||
            virtual_heartbeat_sender == current_term_leader_id,
        "we shouldn't be getting virtual heartbeats from a non-leader");
    guarantee(leader_drainer.has() == (mode != mode_t::follower),
        "candidate_and_leader_coro() should be running unless we're a follower");
    guarantee(mode == mode_t::leader || !readiness_for_change.get(),
        "we shouldn't be accepting changes if we're not leader");
    guarantee(readiness_for_change.get() || !readiness_for_change.get(),
        "we shouldn't be accepting config changes but not regular changes");

    switch (mode) {
    case mode_t::follower:
        guarantee(match_indexes.empty(), "If we're a follower, `match_indexes` should "
            "be empty.");
        break;
    case mode_t::candidate:
        guarantee(current_term_leader_id.is_nil(), "if we have a leader we shouldn't be "
            "having an election");
        guarantee(ps().voted_for == this_member_id, "if we're a candidate we should "
            "have voted for ourself");
        guarantee(match_indexes.empty(), "If we're a candidate, `match_indexes` should "
            "be empty.");
        break;
    case mode_t::leader:
        guarantee(ps().voted_for == this_member_id, "if we're a leader we should have "
            "voted for ourself");
        guarantee(latest_term_in_log == ps().current_term, "if we're a leader we should "
            "have put at least the initial noop entry in the log");
        guarantee(match_indexes.at(this_member_id) == ps().log.get_latest_index(),
            "If we're a leader, `match_indexes` should contain an up-to-date entry for "
            "ourselves.");
        break;
    default:
        unreachable();
    }

    /* We could test that `current_term_leader_id` is non-nil if there are any changes in
    the log for the current term. But this would be false immediately after startup, so
    we'd need an extra flag to detect that, and that's more work than it's worth. */
}
#endif /* NDEBUG */

template<class state_t>
void raft_member_t<state_t>::on_connected_members_change(
        const raft_member_id_t &member_id,
        const boost::optional<raft_term_t> *value) {
    assert_thread();
    ASSERT_NO_CORO_WAITING;
    update_readiness_for_change();
    if (value != nullptr && static_cast<bool>(*value) && member_id != this_member_id) {
        /* We've received a "start virtual heartbeats" message. We process the term just
        like for an AppendEntries or InstallSnapshot RPC, but we don't actually append
        any entries or install any snapshots. */
        raft_term_t term = **value;
        auto_drainer_t::lock_t keepalive(drainer.get());
        coro_t::spawn_sometime(
        [this, term, member_id, keepalive /* important to capture */]() {
            try {
                new_mutex_acq_t mutex_acq(&mutex, keepalive.get_drain_signal());
                DEBUG_ONLY_CODE(this->check_invariants(&mutex_acq));

                if (!this->on_rpc_from_leader(member_id, term, &mutex_acq)) {
                    /* If this were a real AppendEntries or InstallSnapshot RPC, we would
                    send an RPC reply to the leader informing it that our term is higher
                    than its term. However, there's no way to send an RPC reply to a
                    virtual heartbeat. So the leader needs some other way to find out
                    that our term is higher than its term. In order to make this work,
                    this implementation deviates from the Raft paper in how leaders
                    handle RequestVote RPCs; see the comment in `on_request_vote_rpc()`
                    for more information. */
                    DEBUG_ONLY_CODE(this->check_invariants(&mutex_acq));
                    return;
                }

                /* Check that the `get_connected_members()` state is still what it was
                when this coroutine was spawned; we mustn't set
                `virtual_heartbeat_sender` unless we are actually getting virtual
                heartbeats. */
                if (network->get_connected_members()->get_key(member_id)
                        == boost::make_optional(boost::make_optional(term))) {
                    /* Sometimes we are called twice within the same term. */
                    if (member_id != virtual_heartbeat_sender) {
                        guarantee(virtual_heartbeat_sender.is_nil());
                        virtual_heartbeat_sender = member_id;

                        /* As long as we're receiving valid virtual heartbeats, we won't
                        start a new election */
                        virtual_heartbeat_watchdog_blockers[0].init(
                            new watchdog_timer_t::blocker_t(watchdog.get()));
                        virtual_heartbeat_watchdog_blockers[1].init(
                            new watchdog_timer_t::blocker_t(watchdog_leader_only.get()));
                    }
                }

                DEBUG_ONLY_CODE(this->check_invariants(&mutex_acq));
            } catch (const interrupted_exc_t &) {
                /* ignore */
            }
        });
    } else if (virtual_heartbeat_sender == member_id) {
        /* We've received a "stop virtual heartbeats" message, or lost contact with the
        member that was sending virtual heartbeats. */
        virtual_heartbeat_sender = raft_member_id_t();

        /* Now that we're no longer receiving virtual heartbeats, unblock the watchdogs.
        */
        virtual_heartbeat_watchdog_blockers[0].reset();
        virtual_heartbeat_watchdog_blockers[1].reset();
    }
}

template<class state_t>
bool raft_member_t<state_t>::on_rpc_from_leader(
        const raft_member_id_t &request_leader_id,
        raft_term_t request_term,
        const new_mutex_acq_t *mutex_acq) {
    assert_thread();
    mutex_acq->guarantee_is_holding(&mutex);

    guarantee(request_leader_id != this_member_id);

    /* Raft paper, Figure 2: "If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower" */
    if (request_term > ps().current_term) {
        RAFT_DEBUG_THIS("Install/Append from %s for %" PRIu64 "\n",
            show_member_id(request_leader_id).c_str(), request_term);
        if (mode != mode_t::follower) {
            candidate_or_leader_become_follower(mutex_acq);
        }
        /* Note that we become a follower before we update the term.
        We do this so that if we're currently a candidate in `candidate_run_election`,
        we don't change the Raft state in the middle of its execution.
        Doing so might not actually do any harm in our current implementation, but
        it's probably safer to avoid the risk.
        `candidate_or_leader_become_follower` will force `candidate_run_election` to
        finish before we get here. */
        update_term(request_term, raft_member_id_t(), mutex_acq);
        /* Continue processing the RPC as follower */
    }

    /* Raft paper, Figure 2: "Reply false if term < currentTerm (SE 5.1)"
    Raft paper, Section 5.1: "If a server receives a request with a stale term number, it
    rejects the request" */
    if (request_term < ps().current_term) {
        return false;
    }

    guarantee(request_term == ps().current_term);   /* sanity check */

    /* Raft paper, Section 5.2: "While waiting for votes, a candidate may receive an
    AppendEntries RPC from another server claiming to be leader. If the leader's term
    (included in its RPC) is at least as large as the candidate's current term, then the
    candidate recognizes the leader as legitimate and returns to follower state."

    Note that this implementation also do this for install-snapshot RPCs, since it's
    conceivably possible that the first RPC we receive from the leader may be an
    install-snapshot RPC rather than an append-entries RPC. */
    if (mode == mode_t::candidate) {
        RAFT_DEBUG_THIS("Install/Append from %s for %" PRIu64 "\n",
            show_member_id(request_leader_id).c_str(), request_term);
        candidate_or_leader_become_follower(mutex_acq);
    }

    /* Raft paper, Section 5.2: "at most one candidate can win the election for a
    particular term"
    If we're leader, then we won the election, so it makes no sense for us to receive an
    RPC from another member that thinks it's leader. */
    guarantee(mode != mode_t::leader);

    /* Raft paper, Section 5.2: "A server remains in follower state as long as it
    receives valid RPCs from a leader or candidate."
    So we should make a note that we received an RPC. */
    watchdog->notify();
    watchdog_leader_only->notify();

    /* Recall that `current_term_leader_id` is set to `nil_uuid()` if we haven't seen a
    leader yet this term. */
    if (current_term_leader_id.is_nil()) {
        current_term_leader_id = request_leader_id;
    } else {
        /* Raft paper, Section 5.2: "at most one candidate can win the election for a
        particular term" */
        guarantee(current_term_leader_id == request_leader_id);
    }

    return true;
}

template<class state_t>
void raft_member_t<state_t>::on_watchdog() {
    if (mode != mode_t::follower) {
        /* If we're already a candidate or leader, there's no need for this. Candidates
        have their own mechanism for retrying stuck elections. */
        return;
    }

    /* Note that we may begin an election even if we are not a voter in our own latest
    configuration. This is necessary to prevent deadlock in some configuration change
    scenarios. This corner case is explicitly addressed in the Raft dissertation, section
    4.2.2: "A server that is not part of its own latest configuration should still start
    new elections, as it might still be needed until the C_new entry is committed." */

    /* We shouldn't block in this callback, so we immediately spawn a coroutine */
    auto_drainer_t::lock_t keepalive(drainer.get());
    coro_t::spawn_sometime([this, keepalive /* important to capture */]() {
        try {
            scoped_ptr_t<new_mutex_acq_t> mutex_acq(
                new new_mutex_acq_t(&mutex, keepalive.get_drain_signal()));
            DEBUG_ONLY_CODE(this->check_invariants(mutex_acq.get()));
            /* Double-check that nothing has changed while the coroutine was
            spawning. */
            if (mode != mode_t::follower ||
                    watchdog->get_state() == watchdog_timer_t::state_t::NOT_TRIGGERED) {
                return;
            }
            /* Begin an election */
            guarantee(!leader_drainer.has());
            leader_drainer.init(new auto_drainer_t);
            coro_t::spawn_sometime(std::bind(
                &raft_member_t<state_t>::candidate_and_leader_coro,
                this,
                mutex_acq.release(),
                auto_drainer_t::lock_t(leader_drainer.get())));
        } catch (const interrupted_exc_t &) {
            /* If `keepalive.get_drain_signal()` fires, the `raft_member_t` is being
            destroyed, so don't start an election. */
        }
    });
}

template<class state_t>
void raft_member_t<state_t>::apply_log_entries(
        state_and_config_t *state_and_config,
        const raft_log_t<state_t> &log,
        raft_log_index_t first,
        raft_log_index_t last) {
    guarantee(last >= first - 1);
    guarantee(state_and_config->log_index + 1 == first);
    for (raft_log_index_t i = first; i <= last; ++i) {
        const raft_log_entry_t<state_t> &e = log.get_entry_ref(i);
        switch (e.type) {
            case raft_log_entry_type_t::regular:
                state_and_config->state.apply_change(*e.change);
                break;
            case raft_log_entry_type_t::config:
                state_and_config->config = *e.config;
                break;
            case raft_log_entry_type_t::noop:
                break;
            default: unreachable();
        }
        ++state_and_config->log_index;
    }
    guarantee(state_and_config->log_index == last);
}

template<class state_t>
void raft_member_t<state_t>::update_term(
        raft_term_t new_term,
        raft_member_id_t new_voted_for,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);

    guarantee(new_term > ps().current_term);

    /* In Figure 2, `votedFor` is defined as "candidateId that received vote in
    current term (or null if none)". So when the current term changes, we have to
    update `voted_for`. Normally we update it to null, but if this is a term that we
    created, we updated it to our own member ID. The reason for doing this as a single
    step is to save a call to `write_current_term_and_voted_for()`. */
    storage->write_current_term_and_voted_for(new_term, new_voted_for);

    /* The same logic applies to `current_term_leader_id` and
    `virtual_heartbeat_sender`. */
    current_term_leader_id = raft_member_id_t();
    virtual_heartbeat_sender = raft_member_id_t();
    virtual_heartbeat_watchdog_blockers[0].reset();
    virtual_heartbeat_watchdog_blockers[1].reset();
}

template<class state_t>
void raft_member_t<state_t>::update_commit_index(
        raft_log_index_t new_commit_index,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);

    raft_log_index_t old_commit_index = committed_state.get_ref().log_index;
    guarantee(new_commit_index > old_commit_index);

    /* This implementation deviates from the Raft paper in that we persist the commit
    index to disk whenever it changes. This ensures that the state machine never appears
    to go backwards. */
    storage->write_commit_index(new_commit_index);

    /* Raft paper, Figure 2: "If commitIndex > lastApplied: increment lastApplied, apply
    log[lastApplied] to state machine"
    If we are leader, updating `committed_state` will trigger several events:
      * It will notify any running instances of `leader_send_updates()` to send the new
        commit index to the followers.
      * If the newly-committed state is a joint consensus state, it will wake
        `candidate_and_leader_coro()` to start the second phase of the reconfiguration.
    */
    committed_state.apply_atomic_op([&](state_and_config_t *state_and_config) -> bool {
        this->apply_log_entries(state_and_config, this->ps().log,
            state_and_config->log_index + 1, new_commit_index);
        return true;
    });
    guarantee(committed_state.get_ref().log_index == new_commit_index);

    /* Notify any change tokens that were waiting on this commit */
    for (auto it = change_tokens.begin();
            it != change_tokens.upper_bound(new_commit_index);) {
        /* We might remove the entry from the map, which would invalidate the iterator.
        So we have to increment the iterator now. */
        change_token_t *token = it->second;
        ++it;
        /* If the change token corresponds to a configuration change, it's not finished
        until both phases of the config change have been committed. */
        if (token->is_config && committed_state.get_ref().config.is_joint_consensus()) {
            /* We committed the first part of the config change, but not the second. */
            continue;
        }
        token->pulse(true);
        /* This removes it from the map */
        token->sentry.reset();
    }

#ifndef NDEBUG
    /* In debug mode, snapshot randomly with 1/3 probability after each change. This is
    so that the tests will exercise many different code paths. */
    bool should_take_snapshot = (randint(3) == 0);
#else
    /* In release mode, snapshot when the log grows beyond a certain margin. */
    size_t num_committed_entries = new_commit_index - ps().log.prev_index;
    bool should_take_snapshot = (num_committed_entries > snapshot_threshold);
#endif /* NDEBUG */
    if (should_take_snapshot) {
        /* Take a snapshot as described in Section 7.

        This automatically updates `ps().log.prev_index` and `ps().log.prev_term`, which
        are equivalent to the "last included index" and "last included term" described in
        Section 7 of the Raft paper.*/
        storage->write_snapshot(
            committed_state.get_ref().state,
            committed_state.get_ref().config,
            false,
            new_commit_index,
            ps().log.get_entry_term(new_commit_index),
            new_commit_index);
    }

    /* If we just committed the second step of a config change, then we might need to
    flip `readiness_for_config_change` */
    update_readiness_for_change();
}

template<class state_t>
void raft_member_t<state_t>::leader_update_match_index(
        raft_member_id_t key,
        raft_log_index_t new_value,
        const new_mutex_acq_t *mutex_acq) {
    guarantee(mode == mode_t::leader);
    mutex_acq->guarantee_is_holding(&mutex);

    auto it = match_indexes.find(key);
    guarantee(it != match_indexes.end());
    guarantee(it->second <= new_value);
    it->second = new_value;

    /* Raft paper, Figure 2: "If there exists an N such that N > commitIndex, a majority
    of matchIndex[i] >= N, and log[N].term == currentTerm: set commitIndex = N" */
    raft_log_index_t old_commit_index = committed_state.get_ref().log_index;
    raft_log_index_t new_commit_index = old_commit_index;
    for (raft_log_index_t n = old_commit_index + 1;
            n <= ps().log.get_latest_index(); ++n) {
        if (ps().log.get_entry_term(n) != ps().current_term) {
            continue;
        }
        std::set<raft_member_id_t> approving_members;
        for (auto const &pair : match_indexes) {
            if (pair.second >= n) {
                approving_members.insert(pair.first);
            }
        }
        if (latest_state.get_ref().config.is_quorum(approving_members)) {
            new_commit_index = n;
        } else {
            /* If this log entry isn't approved, no later log entry can possibly be
            approved either */
            break;
        }
    }
    if (new_commit_index != old_commit_index) {
        update_commit_index(new_commit_index, mutex_acq);
    }
}

template<class state_t>
void raft_member_t<state_t>::update_readiness_for_change() {
    /* We're ready for changes if we're the leader and in contact with a majority of the
    cluster. We're ready for config changes if the above conditions are true plus we're
    also not in the middle of a config change. */
    if (mode == mode_t::leader) {
        std::set<raft_member_id_t> peers;
        network->get_connected_members()->read_all(
            [&](const raft_member_id_t &member_id,
                    const boost::optional<raft_term_t> *) {
                peers.insert(member_id);
            });
        if (latest_state.get_ref().config.is_quorum(peers)) {
            readiness_for_change.set_value(true);
            readiness_for_config_change.set_value(
                !committed_state.get_ref().config.is_joint_consensus() &&
                !latest_state.get_ref().config.is_joint_consensus());
            return;
        }
    }
    readiness_for_change.set_value(false);
    readiness_for_config_change.set_value(false);
    while (!change_tokens.empty()) {
        change_tokens.begin()->second->pulse(false);
        /* This will remove the change token from the map */
        change_tokens.begin()->second->sentry.reset();
    }
}

template<class state_t>
void raft_member_t<state_t>::candidate_or_leader_become_follower(
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);
    guarantee(mode == mode_t::candidate || mode == mode_t::leader);
    guarantee(leader_drainer.has());

    /* This will interrupt `candidate_and_leader_coro()` and block until it exits */
    leader_drainer.reset();

    /* `candidate_and_leader_coro()` should have reset `mode` when it exited */
    guarantee(mode == mode_t::follower);
}

template<class state_t>
void raft_member_t<state_t>::candidate_and_leader_coro(
        new_mutex_acq_t *mutex_acq_on_heap,
        auto_drainer_t::lock_t leader_keepalive) {
    scoped_ptr_t<new_mutex_acq_t> mutex_acq(mutex_acq_on_heap);
    guarantee(mode == mode_t::follower);
    mutex_acq->guarantee_is_holding(&mutex);
    leader_keepalive.assert_is_holding(leader_drainer.get());
    signal_t *interruptor = leader_keepalive.get_drain_signal();

    /* Raft paper, Section 5.2: "To begin an election, a follower increments its current
    term and transitions to candidate state."
    `candidate_run_election()` is what actually increments the current term. */
    mode = mode_t::candidate;

    if (!log_prefix.empty()) {
        logINF("%s: Starting a new Raft election for term %" PRIu64 ".",
            log_prefix.c_str(), ps().current_term + 1);
    }
    RAFT_DEBUG_THIS("election begun for %" PRIu64 "\n", ps().current_term + 1);

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
            /* `candidate_run_election` might temporarily reset `mutex_acq`, but it
            should always re-acquire it before it returns. */
            bool elected = candidate_run_election(
                &mutex_acq, &election_timeout, interruptor);
            guarantee(mutex_acq.has());
            mutex_acq->guarantee_is_holding(&mutex);
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

                if (!log_prefix.empty()) {
                    logINF("%s: Raft election timed out. Starting a new election for "
                        "term %" PRIu64 ".", log_prefix.c_str(), ps().current_term + 1);
                }
                RAFT_DEBUG_THIS("election restarted for %" PRIu64 "\n",
                    ps().current_term + 1);

                /* Go around the `while`-loop again. */
            }
        }

        /* We got elected. */
        guarantee(mode == mode_t::leader);

        if (!log_prefix.empty()) {
            logINF("%s: This server is Raft leader for term %" PRIu64 ". Latest log "
                "index is %" PRIu64 ".",
                log_prefix.c_str(), ps().current_term, ps().log.get_latest_index());
        }
        RAFT_DEBUG_THIS("won election for %" PRIu64 "\n", ps().current_term);

        guarantee(current_term_leader_id.is_nil());
        current_term_leader_id = this_member_id;

        /* Now that `mode` has switched to `mode_leader`, we might need to flip
        `readiness_for_change`. */
        update_readiness_for_change();

        /* Raft paper, Section 5.3: "When a leader first comes to power, it initializes
        all nextIndex values to the index just after the last one in its log"
        It's important to do this before we append the no-op to the log, so that we
        replicate the no-op on the first try instead of having to try twice. */
        raft_log_index_t initial_next_index = ps().log.get_latest_index() + 1;

        /* Now that we are leader, initialize `match_indexes`. For now we just fill in
        the entry for ourselves; `leader_spawn_update_coros()` will handle filling in the
        entries for the other members of the cluster. */
        guarantee(match_indexes.empty(), "When we were not leader, `match_indexes` "
            "should have been empty.");
        match_indexes[this_member_id] = ps().log.get_latest_index();

        /* Raft paper, Section 8: "[Raft has] each leader commit a blank no-op entry into
        the log at the start of its term."
        This is to ensure that we'll commit any entries that are possible to commit,
        since we can't commit entries from earlier terms except by committing an entry
        from our own term. */
        {
            raft_log_entry_t<state_t> new_entry;
            new_entry.type = raft_log_entry_type_t::noop;
            new_entry.term = ps().current_term;
            leader_append_log_entry(new_entry, mutex_acq.get());
        }

        /* Raft paper, Section 5.2: "Leaders send periodic heartbeats (AppendEntries RPCs
        that carry no log entries) to all followers in order to maintain their
        authority."
        We deviate from the Raft paper in that we send a single "start virtual
        heartbeats" message and another "stop virtual heartbeats" message instead of a
        repeated AppendEntries RPCs. If the network connection is lost, then the
        `connectivity_cluster_t` will detect it using its own heartbeats and then the
        `raft_network_interface_t` will interrupt the virtual heartbeat stream. */
        network->send_virtual_heartbeats(boost::make_optional(ps().current_term));

        /* Prevent the watchdog timers from being triggered until we are no longer
        leader. */
        watchdog_timer_t::blocker_t watchdog_blocker(watchdog.get());
        watchdog_timer_t::blocker_t watchdog_leader_only_blocker(
            watchdog_leader_only.get());

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
                &update_drainers,
                mutex_acq.get());

            /* Check if there is a committed joint consensus configuration but no entry
            in the log for the second phase of the config change. If this is the case,
            then we will add an entry. Also check if we have just committed a config in
            which we are no longer leader, in which case we need to step down. */
            leader_continue_reconfiguration(mutex_acq.get());

            /* Block until either a new entry is appended to the log or a new entry is
            committed. If a new entry is appended to the log, we might need to re-run
            `leader_spawn_update_coros()`; if a new entry is committed, we might need to
            re-run `leader_continue_reconfiguration()`. */
            {
                raft_log_index_t latest_log_index = latest_state.get_ref().log_index;

                /* Release the mutex while blocking so that we can receive RPCs */
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();

                run_until_satisfied_2(
                    committed_state.get_watchable(),
                    latest_state.get_watchable(),
                    [&](const state_and_config_t &cs, const state_and_config_t &ls)
                            -> bool {
                        if (cs.config.is_joint_consensus() &&
                                ls.config.is_joint_consensus()) {
                            /* The fact that `cs.config` is a joint consensus indicates
                            that we recently committed a joint consensus configuration.
                            The fact that `ls.config` is a joint consensus means that we
                            haven't put the second phase of the reconfiguration into the
                            log yet. So we should do that now. */
                            return true;
                        }
                        if (ls.log_index > latest_log_index) {
                            /* Something new was appended to the log. We should go check
                            if it was a configuration change. */
                            return true;
                        }
                        return false;
                    },
                    interruptor);

                /* Reacquire the mutex. Note that if `wait_interruptible()` throws
                `interrupted_exc_t`, we won't reacquire the mutex. This is by design;
                probably `candidate_or_leader_become_follower()` is holding the mutex. */
                mutex_acq.init(new new_mutex_acq_t(&mutex, interruptor));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
            }
        }

    } catch (const interrupted_exc_t &) {
        /* This means either the `raft_member_t` is being destroyed, or we were told to
        revert to follower state. In either case we do the same thing. */
    }

    /* Stop sending heartbeats now that we're no longer the leader. */
    network->send_virtual_heartbeats(boost::none);

    mode = mode_t::follower;

    /* `match_indexes` is supposed to be empty when we're not leader. This rule isn't
    strictly necessary; it's basically a sanity check. */
    match_indexes.clear();

    /* Now that `mode` is no longer `mode_leader`, we might need to flip
    `readiness_for_change`. */
    update_readiness_for_change();
}

template<class state_t>
bool raft_member_t<state_t>::candidate_run_election(
        scoped_ptr_t<new_mutex_acq_t> *mutex_acq,
        signal_t *cancel_signal,
        signal_t *interruptor) {
    (*mutex_acq)->guarantee_is_holding(&mutex);
    guarantee(mode == mode_t::candidate);

    std::set<raft_member_id_t> votes_for_us;
    cond_t we_won_the_election;

    /* Raft paper, Section 5.2: "To begin an election, a follower increments its current
    term and transitions to candidate state. It then votes for itself." */
    update_term(ps().current_term + 1, this_member_id, mutex_acq->get());
    votes_for_us.insert(this_member_id);
    if (latest_state.get_ref().config.is_quorum(votes_for_us)) {
        /* In some degenerate cases, like if the cluster has only one node, then
        our own vote might be enough to be elected. */
        we_won_the_election.pulse();
    }

    typename raft_rpc_request_t<state_t>::request_vote_t request;
    request.term = this->ps().current_term;
    request.candidate_id = this_member_id;
    request.last_log_index = this->ps().log.get_latest_index();
    request.last_log_term =
        this->ps().log.get_entry_term(this->ps().log.get_latest_index());

    /* Raft paper, Section 5.2: "[The candidate] issues RequestVote RPCs in parallel to
    each of the other servers in the cluster." */
    std::set<raft_member_id_t> peers = latest_state.get_ref().config.get_all_members();
    scoped_ptr_t<auto_drainer_t> request_vote_drainer(new auto_drainer_t);
    for (const raft_member_id_t &peer : peers) {
        if (peer == this_member_id) {
            /* Don't request a vote from ourself */
            continue;
        }

        auto_drainer_t::lock_t request_vote_keepalive(request_vote_drainer.get());
        coro_t::spawn_sometime([this, &votes_for_us, &we_won_the_election, peer,
                &request, request_vote_keepalive /* important to capture */]() {
            try {
                exponential_backoff_t backoff(100, 1000);
                while (true) {
                    /* Don't bother trying to send an RPC until the peer is present in
                    `get_connected_members()`. */
                    network->get_connected_members()->run_key_until_satisfied(peer,
                        [](const boost::optional<raft_term_t> *x) {
                            return x != nullptr;
                        },
                        request_vote_keepalive.get_drain_signal());

                    guarantee(this->mode == mode_t::candidate);
                    raft_rpc_request_t<state_t> request_wrapper;
                    request_wrapper.request = request;

                    raft_rpc_reply_t reply_wrapper;
                    bool ok = network->send_rpc(peer, request_wrapper,
                        request_vote_keepalive.get_drain_signal(), &reply_wrapper);

                    if (!ok) {
                        /* Raft paper, Section 5.1: "Servers retry RPCs if they do not
                        receive a response in a timely manner" */
                        backoff.failure(request_vote_keepalive.get_drain_signal());
                        continue;
                    }

                    const raft_rpc_reply_t::request_vote_t *reply =
                        boost::get<raft_rpc_reply_t::request_vote_t>(
                            &reply_wrapper.reply);
                    guarantee(reply != nullptr, "Got wrong type of RPC response");

                    {
                        new_mutex_acq_t mutex_acq_2(
                            &mutex, request_vote_keepalive.get_drain_signal());
                        DEBUG_ONLY_CODE(this->check_invariants(&mutex_acq_2));
                        /* We might soon be leader, but we shouldn't be leader quite yet
                        */
                        guarantee(mode == mode_t::candidate);

                        if (this->candidate_or_leader_note_term(
                                reply->term, &mutex_acq_2)) {
                            /* We got a response with a higher term than our current
                            term. `candidate_and_leader_coro()` will be interrupted soon.
                            */
                            RAFT_DEBUG_THIS(
                                "got rpc reply with term %" PRIu64 " from %s\n",
                                reply->term, show_member_id(peer).c_str());
                            return;
                        }

                        if (reply->vote_granted) {
                            votes_for_us.insert(peer);
                            /* Raft paper, Section 5.2: "A candidate wins an election if
                            it receives votes from a majority of the servers in the full
                            cluster for the same term." */
                            if (latest_state.get_ref().config.is_quorum(votes_for_us)) {
                                we_won_the_election.pulse_if_not_already_pulsed();
                            }
                            break;
                        }

                        /* Mutex is released here, so we don't hold it while sleeping */
                    }

                    /* If they didn't grant the vote, just keep pestering them until they
                    do or we are interrupted. This is necessary because followers will
                    temporarily reject votes if they have heard from a current leader
                    recently, so they might reject a vote and then later accept it in the
                    same term. But before we retry, we wait a bit, to avoid putting too
                    much traffic on the network. */
                    backoff.failure(request_vote_keepalive.get_drain_signal());
                }

            } catch (const interrupted_exc_t &) {
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

template<class state_t>
void raft_member_t<state_t>::leader_spawn_update_coros(
        raft_log_index_t initial_next_index,
        std::map<raft_member_id_t, scoped_ptr_t<auto_drainer_t> > *update_drainers,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);
    guarantee(mode == mode_t::leader);

    /* Calculate the new configuration */
    std::set<raft_member_id_t> peers = latest_state.get_ref().config.get_all_members();

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
        match_indexes.insert(std::make_pair(peer, 0));

        scoped_ptr_t<auto_drainer_t> update_drainer(new auto_drainer_t);
        auto_drainer_t::lock_t update_keepalive(update_drainer.get());
        (*update_drainers)[peer] = std::move(update_drainer);
        coro_t::spawn_sometime(std::bind(&raft_member_t::leader_send_updates, this,
            peer,
            initial_next_index,
            update_keepalive));
    }

    /* Kill coroutines as necessary */
    for (auto it = update_drainers->begin(); it != update_drainers->end(); ) {
        raft_member_id_t peer = it->first;
        guarantee(peer != this_member_id, "We shouldn't have spawned a coroutine to "
            "send updates to ourself");
        if (peers.count(peer) == 1) {
            /* `peer` is still a member of the cluster, so the coroutine should stay
            alive */
            ++it;
            continue;
        } else {
            /* This will block until the update coroutine stops */
            update_drainers->erase(it++);
            match_indexes.erase(peer);
        }
    }
}

template<class state_t>
void raft_member_t<state_t>::leader_send_updates(
        const raft_member_id_t &peer,
        raft_log_index_t initial_next_index,
        auto_drainer_t::lock_t update_keepalive) {
    try {
        guarantee(peer != this_member_id);
        guarantee(mode == mode_t::leader);
        guarantee(match_indexes.count(peer) == 1);
        scoped_ptr_t<new_mutex_acq_t> mutex_acq(
            new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
        DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

        /* `next_index` corresponds to the entry for this peer in the `nextIndex` map
        described in Figure 2 of the Raft paper. Note that this is the index of the first
        log entry that is not yet on the peer, not the index of the last log entry that
        is currently on the peer. */
        raft_log_index_t next_index = initial_next_index;

        /* `member_commit_index` tracks the last value of `commit_index` we sent to the
        member. It doesn't correspond to anything in the Raft paper. This implementation
        deviates from the Raft paper slightly in that when the leader commits a log
        entry, it immediately sends an append-entries RPC so that clients can commit the
        log entry too. This is necessary because we use "virtual heartbeats" in place of
        regular heartbeats, and virtual heartbeats don't update the commit index. */
        raft_log_index_t member_commit_index = 0;

        /* Raft paper, Figure 2: "Upon election: send initial empty AppendEntries RPCs
        (heartbeat) to each server"
        Setting `send_even_if_empty` will ensure that we send an RPC immediately. */
        bool send_even_if_empty = true;

        /* If RPCs fail we'll wait a bit before trying again, in addition to waiting for
        the peer to appear in `get_connected_members()`. This prevents us from locking up
        the CPU if the peer is in `get_connected_members()` but always fails RPCs
        immediately. */
        exponential_backoff_t backoff(100, 1000);

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
            /* Don't bother trying to send an RPC until the peer is present in
            `get_connected_members()`. */
            DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
            mutex_acq.reset();
            network->get_connected_members()->run_key_until_satisfied(peer,
                [](const boost::optional<raft_term_t> *x) {
                    return x != nullptr;
                },
                update_keepalive.get_drain_signal());
            mutex_acq.init(
                new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
            DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));

            if (next_index <= ps().log.prev_index) {
                /* The peer's log ends before our log begins. So we have to send an
                install-snapshot RPC instead of an append-entries RPC. */

                typename raft_rpc_request_t<state_t>::install_snapshot_t request;
                request.term = ps().current_term;
                request.leader_id = this_member_id;
                request.last_included_index = ps().log.prev_index;
                request.last_included_term = ps().log.prev_term;
                /* TODO: Maybe we should send `committed_state` instead of the snapshot.
                Under the current implementation, they will always be the same; but if we
                were to allow the snapshot to lag behind the committed state, sending
                `committed_state` would help the follower catch up faster. */
                request.snapshot_state = ps().snapshot_state;
                request.snapshot_config = ps().snapshot_config;
                raft_rpc_request_t<state_t> request_wrapper;
                request_wrapper.request = request;

                raft_rpc_reply_t reply_wrapper;
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();

                bool ok = network->send_rpc(peer, request_wrapper,
                    update_keepalive.get_drain_signal(), &reply_wrapper);

                /* If the RPC failed, do the backoff before reacquiring the mutex */
                if (!ok) {
                    backoff.failure(update_keepalive.get_drain_signal());
                }

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

                backoff.success();

                const raft_rpc_reply_t::install_snapshot_t *reply =
                    boost::get<raft_rpc_reply_t::install_snapshot_t>(
                        &reply_wrapper.reply);
                guarantee(reply != nullptr, "Got wrong type of RPC response");

                if (candidate_or_leader_note_term(reply->term, mutex_acq.get())) {
                    /* We got a reply with a higher term than our term.
                    `candidate_and_leader_coro()` will be interrupted soon. */
                    RAFT_DEBUG_THIS("got rpc reply with term %" PRIu64 " from %s\n",
                                    reply->term, show_member_id(peer).c_str());
                    return;
                }

                next_index = request.last_included_index + 1;
                leader_update_match_index(
                    peer,
                    request.last_included_index,
                    mutex_acq.get());
                send_even_if_empty = false;

            } else if (next_index <= ps().log.get_latest_index() ||
                    member_commit_index < committed_state.get_ref().log_index ||
                    send_even_if_empty) {
                /* The peer's log ends right where our log begins, or in the middle of
                our log. Send an append-entries RPC. */

                typename raft_rpc_request_t<state_t>::append_entries_t request;
                request.term = ps().current_term;
                request.leader_id = this_member_id;
                request.entries.prev_index = next_index - 1;
                request.entries.prev_term =
                    ps().log.get_entry_term(request.entries.prev_index);
                for (raft_log_index_t i = next_index;
                        i <= ps().log.get_latest_index(); ++i) {
                    request.entries.append(ps().log.get_entry_ref(i));
                }
                guarantee(request.entries.get_latest_index()
                    == ps().log.get_latest_index());
                request.leader_commit = committed_state.get_ref().log_index;
                raft_rpc_request_t<state_t> request_wrapper;
                request_wrapper.request = request;

                raft_rpc_reply_t reply_wrapper;
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();

                bool ok = network->send_rpc(peer, request_wrapper,
                    update_keepalive.get_drain_signal(), &reply_wrapper);

                /* If the RPC failed, do the backoff before reacquiring the mutex */
                if (!ok) {
                    backoff.failure(update_keepalive.get_drain_signal());
                }

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

                backoff.success();

                const raft_rpc_reply_t::append_entries_t *reply =
                    boost::get<raft_rpc_reply_t::append_entries_t>(
                        &reply_wrapper.reply);
                guarantee(reply != nullptr, "Got wrong type of RPC response");

                if (candidate_or_leader_note_term(reply->term, mutex_acq.get())) {
                    /* We got a reply with a higher term than our term.
                    `candidate_and_leader_coro()` will be interrupted soon. */
                    RAFT_DEBUG_THIS("got rpc reply with term %" PRIu64 " from %s\n",
                                    reply->term, show_member_id(peer).c_str());
                    return;
                }

                if (reply->success) {
                    /* Raft paper, Figure 2: "If successful: update nextIndex and
                    matchIndex for follower */
                    next_index = request.entries.get_latest_index() + 1;
                    if (match_indexes.at(peer) < request.entries.get_latest_index()) {
                        leader_update_match_index(
                            peer,
                            request.entries.get_latest_index(),
                            mutex_acq.get());
                    }
                    member_commit_index = request.leader_commit;
                } else {
                    /* Raft paper, Section 5.3: "After a rejection, the leader decrements
                    nextIndex and retries the AppendEntries RPC. */
                    --next_index;
                }
                send_even_if_empty = false;

            } else {
                guarantee(next_index == ps().log.get_latest_index() + 1);
                guarantee(member_commit_index == committed_state.get_ref().log_index);
                /* OK, the peer is completely up-to-date. Wait until either an entry is
                appended to the log or our commit index advances, and then go around the
                loop again. */

                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
                mutex_acq.reset();

                run_until_satisfied_2(
                    committed_state.get_watchable(),
                    latest_state.get_watchable(),
                    [&](const state_and_config_t &cs, const state_and_config_t &ls) {
                        return cs.log_index > member_commit_index ||
                            ls.log_index >= next_index;
                    },
                    update_keepalive.get_drain_signal());

                mutex_acq.init(
                    new new_mutex_acq_t(&mutex, update_keepalive.get_drain_signal()));
                DEBUG_ONLY_CODE(check_invariants(mutex_acq.get()));
            }
        }
    } catch (const interrupted_exc_t &) {
        /* The leader interrupted us. This could be because the `raft_member_t` is being
        destroyed; because the leader is no longer leader; or because a config change
        removed `peer` from the cluster. In any case, we just return. */
    }
}

template<class state_t>
void raft_member_t<state_t>::leader_continue_reconfiguration(
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);
    guarantee(mode == mode_t::leader);
    /* Check if we recently committed a joint consensus configuration, or a configuration
    in which we are no longer leader. */
    if (!committed_state.get_ref().config.is_valid_leader(this_member_id) &&
            !latest_state.get_ref().config.is_valid_leader(this_member_id)) {
        /* Raft paper, Section 6: "...the leader steps down (returns to follower state)
        once it has committed the C_new log entry."
        `candidate_or_leader_note_term()` isn't designed for the purpose of making us
        intentionally step down, but it contains all the right logic. This has a side
        effect of incrementing `current_term`, but I don't think that's a problem. */
        RAFT_DEBUG_THIS("stepping down, new term: %" PRIu64 "\n", ps().current_term + 1);
        candidate_or_leader_note_term(ps().current_term + 1, mutex_acq);
    } else if (committed_state.get_ref().config.is_joint_consensus() &&
            latest_state.get_ref().config.is_joint_consensus()) {
        /* OK, we recently committed a joint consensus configuration, but we haven't yet
        appended the second part of the reconfiguration to the log (or else
        `latest_state` wouldn't be a joint consensus). */

        /* Raft paper, Section 6: "Once C_old,new has been committed... it is now safe
        for the leader to create a log entry describing C_new and replicate it to the
        cluster. */
        raft_complex_config_t new_config;
        new_config.config = *committed_state.get_ref().config.new_config;

        raft_log_entry_t<state_t> new_entry;
        new_entry.type = raft_log_entry_type_t::config;
        new_entry.config = boost::optional<raft_complex_config_t>(new_config);
        new_entry.term = ps().current_term;

        leader_append_log_entry(new_entry, mutex_acq);
    }
}

template<class state_t>
bool raft_member_t<state_t>::candidate_or_leader_note_term(
        raft_term_t term, const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);
    guarantee(mode != mode_t::follower);
    /* Raft paper, Figure 2: If RPC request or response contains term T > currentTerm:
    set currentTerm = T, convert to follower */
    if (term > ps().current_term) {
        raft_term_t local_current_term = ps().current_term;
        /* We have to spawn this in a separate coroutine because
        `candidate_or_leader_become_follower()` blocks until
        `candidate_and_leader_coro()` exits, so calling
        `candidate_or_leader_become_follower()` directly would cause a deadlock. */
        auto_drainer_t::lock_t keepalive(drainer.get());
        coro_t::spawn_sometime([this, local_current_term, term,
                keepalive /* important to capture */]() {
            try {
                new_mutex_acq_t mutex_acq_2(&mutex, keepalive.get_drain_signal());
                DEBUG_ONLY_CODE(this->check_invariants(&mutex_acq_2));
                /* Check that term hasn't already been updated between when
                `candidate_or_leader_note_term()` was called and when this coroutine ran.
                */
                if (this->ps().current_term == local_current_term) {
                    /* It's unlikely that `mode` will be `follower` at this point, but
                    it's possible. Suppose that we are a candidate for term T, and we
                    receive an RPC reply for term T+1, so we call `..._note_term()` and
                    this coroutine is spawned. But before this coroutine acquires the
                    mutex, we receive an AppendEntries RPC from a leader for term T, so
                    we transition to follower state. Then when this coroutine actually
                    executes, it will find that `current_term` has not changed, but we
                    are in follower state. */
                    if (mode != mode_t::follower) {
                        this->candidate_or_leader_become_follower(&mutex_acq_2);
                    }
                    this->update_term(term, raft_member_id_t(), &mutex_acq_2);
                }
                DEBUG_ONLY_CODE(this->check_invariants(&mutex_acq_2));
            } catch (const interrupted_exc_t &) {
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

template<class state_t>
void raft_member_t<state_t>::leader_append_log_entry(
        const raft_log_entry_t<state_t> &log_entry,
        const new_mutex_acq_t *mutex_acq) {
    mutex_acq->guarantee_is_holding(&mutex);
    guarantee(mode == mode_t::leader);
    guarantee(log_entry.term == ps().current_term);

    /* Raft paper, Section 5.3: "The leader appends the command to its log as a new
    entry..."
    This will block until the log entry is safely on disk. This might be important for
    correctness; it might be dangerous for us to send an AppendEntries RPC for log
    entries that are not safely on disk yet. I'm not sure. */
    storage->write_log_append_one(log_entry);

    /* Raft paper, Section 5.3: "...then issues AppendEntries RPCs in parallel to each of
    the other servers to replicate the entry."
    Because we modified `ps().log`, we have to update `latest_state`. But this will also
    have the side-effect of notifying anything that's waiting on `latest_state`; in
    particular, instances of `leader_send_updates()` will wait on `latest_state` so they
    can be notified when there are new log entries to send to the followers. */
    latest_state.apply_atomic_op([&](state_and_config_t *s) -> bool {
        guarantee(s->log_index + 1 == this->ps().log.get_latest_index());
        this->apply_log_entries(s, this->ps().log, s->log_index + 1, s->log_index + 1);
        return true;
    });

    /* Figure 2 of the Raft paper defines `matchIndexes` as the "index of highest log
    entry known to be replicated on each server". Although it's not explicitly stated
    anywhere, this means the leader needs to increment its entry in `match_indexes`
    whenever it appends to its own log. */
    guarantee(match_indexes.at(this_member_id) + 1 == ps().log.get_latest_index());
    leader_update_match_index(this_member_id, ps().log.get_latest_index(), mutex_acq);
}

#endif /* CLUSTERING_GENERIC_RAFT_CORE_TCC_ */

