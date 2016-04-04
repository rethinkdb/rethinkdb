#include "unittest/clustering_utils_raft.hpp"

namespace unittest {

const char *dummy_raft_cluster_t::show_live(dummy_raft_cluster_t::live_t l) {
    switch (l) {
        case live_t::alive: return "ALIVE";
        case live_t::isolated: return "ISOLATED";
        case live_t::dead: return "DEAD";
        default: unreachable();
    }
}

void dummy_raft_cluster_t::print_state() {
    RAFT_DEBUG("dummy_raft_cluster_t state:\n");
    for (RAFT_DEBUG_VAR auto const &pair : members) {
        RAFT_DEBUG("  member %s: %s\n",
                   show_member_id(pair.second->member_id).c_str(),
                   show_live(pair.second->live));
    }
}

/* The constructor starts a cluster of `num` alive members with the given initial
state. */
dummy_raft_cluster_t::dummy_raft_cluster_t(
        size_t num,
        const dummy_raft_state_t &initial_state,
        std::vector<raft_member_id_t> *member_ids_out) :
    mailbox_manager(&connectivity_cluster, 'M'),
    connectivity_cluster_run(&connectivity_cluster),
    check_invariants_timer(100, [this]() {
        coro_t::spawn_sometime(std::bind(
            &dummy_raft_cluster_t::check_invariants,
            this,
            auto_drainer_t::lock_t(&drainer)));
        })
{
    raft_config_t initial_config;
    for (size_t i = 0; i < num; ++i) {
        raft_member_id_t member_id(generate_uuid());
        if (member_ids_out) {
            member_ids_out->push_back(member_id);
        }
        initial_config.voting_members.insert(member_id);
    }
    for (const raft_member_id_t &member_id : initial_config.voting_members) {
        add_member(
            member_id,
            raft_persistent_state_t<dummy_raft_state_t>::make_initial(
                initial_state, initial_config));
    }
    RAFT_DEBUG("dummy_raft_cluster_t started up\n");
}

dummy_raft_cluster_t::~dummy_raft_cluster_t() {
    RAFT_DEBUG("dummy_raft_cluster_t shutting down\n");
    /* We could just let the destructors run, but then we'd have to worry about
    destructor order, so this is safer and clearer */
    for (const auto &pair : members) {
        set_live(pair.first, live_t::dead);
    }
}

/* `join()` adds a new member to the cluster. The caller is responsible for running a
Raft transaction to modify the config to include the new member. */
raft_member_id_t dummy_raft_cluster_t::join() {
    raft_persistent_state_t<dummy_raft_state_t> init_state;
    bool found_init_state = false;
    for (const auto &pair : members) {
        if (pair.second->member_drainer.has()) {
            cond_t non_interruptor;
            raft_member_t<dummy_raft_state_t>::change_lock_t raft_change_lock(
                pair.second->member->get_raft(), &non_interruptor);
            init_state =
                pair.second->member->get_raft()->get_state_for_init(raft_change_lock);
            found_init_state = true;
            break;
        }
    }
    guarantee(found_init_state, "Can't add a new node to a cluster with no living "
        "members.");
    raft_member_id_t member_id(generate_uuid());
    add_member(member_id, init_state);
    return member_id;
}

dummy_raft_cluster_t::live_t dummy_raft_cluster_t::get_live(
        const raft_member_id_t &member_id) const {
    return members.at(member_id)->live;
}

/* `set_live()` puts the given member into the given state. */
void dummy_raft_cluster_t::set_live(const raft_member_id_t &member_id, live_t live) {
    member_info_t *i = members.at(member_id).get();
    RAFT_DEBUG("%s: liveness %s -> %s\n", show_member_id(member_id).c_str(),
        show_live(i->live), show_live(live));
    if (i->live == live_t::alive && live != live_t::alive) {
        i->bcard_subs.reset();
        for (const auto &pair : members) {
            if (pair.second->live == live_t::alive) {
                pair.second->member_directory.delete_key(member_id);
                i->member_directory.delete_key(pair.first);
            }
        }
    }
    {
        if (i->live != live_t::dead && live == live_t::dead) {
            scoped_ptr_t<auto_drainer_t> dummy;
            std::swap(i->member_drainer, dummy);
            dummy.reset();
            i->member.reset();
        }
        if (i->live == live_t::dead && live != live_t::dead) {
            i->member.init(new raft_networked_member_t<dummy_raft_state_t>(
                member_id, &mailbox_manager, &i->member_directory, i, "",
                raft_start_election_immediately_t::NO));
            i->member_drainer.init(new auto_drainer_t);
        }
    }
    if (i->live != live_t::alive && live == live_t::alive) {
        i->bcard_subs.init(new watchable_t<raft_business_card_t<dummy_raft_state_t> >
            ::subscription_t(
                [this, member_id, i]() {
                    raft_business_card_t<dummy_raft_state_t> bcard =
                        i->member->get_business_card()->get();
                    for (const auto &pair : members) {
                        if (pair.second->live == live_t::alive) {
                            pair.second->member_directory.set_key(member_id, bcard);
                        }
                    }
                },
                i->member->get_business_card(),
                initial_call_t::YES));
        for (const auto &pair : members) {
            if (pair.second->live == live_t::alive) {
                i->member_directory.set_key(pair.first,
                    pair.second->member->get_business_card()->get());
            }
        }
        i->member_directory.set_key(
            member_id, i->member->get_business_card()->get());
    }
    i->live = live;
}

/* Blocks until it finds a cluster member which is advertising itself as ready for
changes, then returns that member's ID. */
raft_member_id_t dummy_raft_cluster_t::find_leader(signal_t *interruptor) {
    while (true) {
        for (const auto &pair : members) {
            if (pair.second->member_drainer.has() &&
                    pair.second->member->get_raft()->
                        get_readiness_for_change()->get()) {
                return pair.first;
            }
        }
        signal_timer_t timer;
        timer.start(10);
        wait_interruptible(&timer, interruptor);
    }
}

raft_member_id_t dummy_raft_cluster_t::find_leader(int timeout) {
    signal_timer_t timer;
    timer.start(timeout);
    try {
        return find_leader(&timer);
    } catch (const interrupted_exc_t &) {
        crash("find_leader() timed out");
    }
}

/* Tries to perform the given change on the member with the given ID. */
bool dummy_raft_cluster_t::try_change(
        raft_member_id_t id,
        const uuid_u &change,
        signal_t *interruptor) {
    bool res;
    run_on_member(id, [&](dummy_raft_member_t *member, signal_t *interruptor2) {
        res = false;
        if (member != nullptr) {
            /* `interruptor2` is only pulsed when the member is being destroyed, so
            it's safe to pass as the `hard_interruptor` parameter */
            try {
                scoped_ptr_t<raft_member_t<dummy_raft_state_t>::change_token_t> tok;
                {
                    raft_member_t<dummy_raft_state_t>::change_lock_t change_lock(
                        member, interruptor);
                    tok = member->propose_change(&change_lock, change);
                }
                if (!tok.has()) {
                    return;
                }
                wait_interruptible(tok->get_ready_signal(), interruptor);
                res = tok->wait();
            } catch (const interrupted_exc_t &) {
                if (interruptor2->is_pulsed()) {
                    throw;
                }
            }
        }
    });
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    return res;
}

/* Like `try_change()` but for Raft configuration changes */
bool dummy_raft_cluster_t::try_config_change(
        raft_member_id_t id,
        const raft_config_t &new_config,
        signal_t *interruptor) {
    bool res;
    run_on_member(id, [&](dummy_raft_member_t *member, signal_t *interruptor2) {
        res = false;
        if (member != nullptr) {
            /* `interruptor2` is only pulsed when the member is being destroyed, so
            it's safe to pass as the `hard_interruptor` parameter */
            try {
                scoped_ptr_t<raft_member_t<dummy_raft_state_t>::change_token_t> tok;
                {
                    raft_member_t<dummy_raft_state_t>::change_lock_t change_lock(
                        member, interruptor);
                    tok = member->propose_config_change(&change_lock, new_config);
                }
                if (!tok.has()) {
                    return;
                }
                wait_interruptible(tok->get_ready_signal(), interruptor);
                res = tok->wait();
            } catch (const interrupted_exc_t &) {
                if (interruptor2->is_pulsed()) {
                    throw;
                }
            }
        }
    });
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    return res;
}

/* `get_all_member_ids()` returns the member IDs of all the members of the cluster,
alive or dead.  */
std::set<raft_member_id_t> dummy_raft_cluster_t::get_all_member_ids() {
    std::set<raft_member_id_t> member_ids;
    for (const auto &pair : members) {
        member_ids.insert(pair.first);
    }
    return member_ids;
}

/* `run_on_member()` calls the given function for the `dummy_raft_member_t *` with
the given ID. If the member is currently dead, it calls the function with a NULL
pointer. */
void dummy_raft_cluster_t::run_on_member(
        const raft_member_id_t &member_id,
        const std::function<void(dummy_raft_member_t *, signal_t *)> &fun) {
    member_info_t *i = members.at(member_id).get();
    if (i->member_drainer.has()) {
        auto_drainer_t::lock_t keepalive = i->member_drainer->lock();
        try {
            fun(i->member->get_raft(), keepalive.get_drain_signal());
        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    } else {
        cond_t non_interruptor;
        fun(nullptr, &non_interruptor);
    }
}

void dummy_raft_cluster_t::member_info_t::write_current_term_and_voted_for(
        raft_term_t current_term,
        raft_member_id_t voted_for) {
    block();
    stored_state.current_term = current_term;
    stored_state.voted_for = voted_for;
    block();
}

void dummy_raft_cluster_t::member_info_t::write_commit_index(
        raft_log_index_t commit_index) {
    block();
    stored_state.commit_index = commit_index;
    block();
}

void dummy_raft_cluster_t::member_info_t::write_log_replace_tail(
        const raft_log_t<dummy_raft_state_t> &log,
        raft_log_index_t first_replaced) {
    block();
    guarantee(first_replaced > stored_state.log.prev_index);
    guarantee(first_replaced <= stored_state.log.get_latest_index() + 1);
    if (first_replaced != stored_state.log.get_latest_index() + 1) {
        stored_state.log.delete_entries_from(first_replaced);
    }
    for (raft_log_index_t i = first_replaced; i <= log.get_latest_index(); ++i) {
        stored_state.log.append(log.get_entry_ref(i));
    }
    block();
}

void dummy_raft_cluster_t::member_info_t::write_log_append_one(
        const raft_log_entry_t<dummy_raft_state_t> &entry) {
    block();
    stored_state.log.append(entry);
    block();
}

void dummy_raft_cluster_t::member_info_t::write_snapshot(
        const dummy_raft_state_t &snapshot_state,
        const raft_complex_config_t &snapshot_config,
        bool clear_log,
        raft_log_index_t log_prev_index,
        raft_term_t log_prev_term,
        raft_log_index_t commit_index) {
    block();
    stored_state.snapshot_state = snapshot_state;
    stored_state.snapshot_config = snapshot_config;
    if (clear_log) {
        stored_state.log.entries.clear();
        stored_state.log.prev_index = log_prev_index;
        stored_state.log.prev_term = log_prev_term;
    } else {
        stored_state.log.delete_entries_to(log_prev_index, log_prev_term);
    }
    stored_state.commit_index = commit_index;
    block();
}

void dummy_raft_cluster_t::member_info_t::block() {
    if (randint(10) != 0) {
        coro_t::yield();
    }
    if (randint(10) == 0) {
        signal_timer_t timer;
        timer.start(randint(30));
        timer.wait_lazily_unordered();
    }
}

void dummy_raft_cluster_t::add_member(
        const raft_member_id_t &member_id,
        raft_persistent_state_t<dummy_raft_state_t> initial_state) {
    scoped_ptr_t<member_info_t> i(new member_info_t(/* this, */ member_id, initial_state));
    members[member_id] = std::move(i);
    set_live(member_id, live_t::alive);
}

void dummy_raft_cluster_t::check_invariants(UNUSED auto_drainer_t::lock_t keepalive) {
    std::set<dummy_raft_member_t *> member_ptrs;
    std::vector<auto_drainer_t::lock_t> keepalives;
    for (auto &pair : members) {
        if (pair.second->member_drainer.has()) {
            keepalives.push_back(pair.second->member_drainer->lock());
            member_ptrs.insert(pair.second->member->get_raft());
        }
    }
    DEBUG_ONLY_CODE(dummy_raft_member_t::check_invariants(member_ptrs));
}

dummy_raft_traffic_generator_t::dummy_raft_traffic_generator_t(
        dummy_raft_cluster_t *_cluster,
        int num_threads) :
    cluster(_cluster) {
    for (int i = 0; i < num_threads; ++i) {
        coro_t::spawn_sometime(std::bind(
            &dummy_raft_traffic_generator_t::do_changes, this, drainer.lock()));
    }
}

size_t dummy_raft_traffic_generator_t::get_num_changes() {
    return committed_changes.size();
}

void dummy_raft_traffic_generator_t::check_changes_present() {
    raft_member_id_t leader = cluster->find_leader(60000);
    cluster->run_on_member(leader, [&](dummy_raft_member_t *m, signal_t *) {
        std::set<uuid_u> all_changes;
        for (const uuid_u &change : m->get_committed_state()->get().state.state) {
            all_changes.insert(change);
        }
        for (const uuid_u &change : committed_changes) {
            ASSERT_EQ(1, all_changes.count(change));
        }
    });
}

void dummy_raft_traffic_generator_t::do_changes(auto_drainer_t::lock_t keepalive) {
    try {
        while (true) {
            uuid_u change = generate_uuid();
            raft_member_id_t leader = cluster->find_leader(
                keepalive.get_drain_signal());
            bool ok = cluster->try_change(
                leader, change, keepalive.get_drain_signal());
            if (ok) {
                committed_changes.insert(change);
            }
        }
    } catch (const interrupted_exc_t &) {
        /* We're shutting down. No action is necessary. */
    }
}

void do_writes_raft(dummy_raft_cluster_t *cluster, int expect, int ms) {
#ifdef ENABLE_RAFT_DEBUG
    RAFT_DEBUG("begin do_writes(%d, %d)\n", expect, ms);
    microtime_t start = current_microtime();
#endif /* ENABLE_RAFT_DEBUG */

    std::set<uuid_u> committed_changes;
    signal_timer_t timer;
    timer.start(ms);
    try {
        while (static_cast<int>(committed_changes.size()) < expect) {
            uuid_u change = generate_uuid();
            raft_member_id_t leader = cluster->find_leader(&timer);
            bool ok = cluster->try_change(leader, change, &timer);
            if (ok) {
                committed_changes.insert(change);
            }
        }
        raft_member_id_t leader = cluster->find_leader(&timer);
        cluster->run_on_member(leader, [&](dummy_raft_member_t *m, signal_t *) {
            std::set<uuid_u> all_changes;
            for (const uuid_u &change : m->get_committed_state()->get().state.state) {
                all_changes.insert(change);
            }
            for (const uuid_u &change : committed_changes) {
                ASSERT_EQ(1, all_changes.count(change));
            }
        });
    } catch (const interrupted_exc_t &) {
        ADD_FAILURE() << "completed only " << committed_changes.size() << "/" << expect
            << " changes in " << ms << "ms";
    }
    RAFT_DEBUG("end do_writes() in %" PRIu64 "ms\n", (current_microtime() - start) / 1000);
}

}   /* namespace unittest */

