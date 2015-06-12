// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <map>

#include "unittest/gtest.hpp"

#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.hpp"
#include "clustering/generic/raft_network.tcc"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* `dummy_raft_state_t` is meant to be used as the `state_t` parameter to
`raft_member_t`, with the `change_t` parameter set to `uuid_u`. It just records all the
changes it receives and their order. */
class dummy_raft_state_t {
public:
    typedef uuid_u change_t;
    std::vector<uuid_u> state;
    void apply_change(const change_t &uuid) {
        state.push_back(uuid);
    }
    bool operator==(const dummy_raft_state_t &other) const {
        return state == other.state;
    }
    bool operator!=(const dummy_raft_state_t &other) const {
        return state != other.state;
    }
};
RDB_MAKE_SERIALIZABLE_1(dummy_raft_state_t, state);

typedef raft_member_t<dummy_raft_state_t> dummy_raft_member_t;

/* `dummy_raft_cluster_t` manages a collection of `dummy_raft_member_t`s. It handles
passing RPCs between them, and it can simulate crashes and netsplits. It periodically
automatically calls `check_invariants()` on its members. */
class dummy_raft_cluster_t {
public:
    /* An `alive` member is a `dummy_raft_member_t` that can communicate with other alive
    members. An `isolated` member is a `dummy_raft_member_t` that cannot communicate with
    any other members. A `dead` member is just a stored `raft_persistent_state_t`. */
    enum class live_t { alive, isolated, dead };

#ifdef ENABLE_RAFT_DEBUG
    const char *show_live(live_t l) {
        switch (l) {
            case live_t::alive: return "ALIVE";
            case live_t::isolated: return "ISOLATED";
            case live_t::dead: return "DEAD";
            default: unreachable();
        }
    }
#endif /* ENABLE_RAFT_DEBUG */

    /* The constructor starts a cluster of `num` alive members with the given initial
    state. */
    dummy_raft_cluster_t(
                size_t num,
                const dummy_raft_state_t &initial_state,
                std::vector<raft_member_id_t> *member_ids_out) :
            mailbox_manager(&connectivity_cluster, 'M'),
            connectivity_cluster_run(&connectivity_cluster, get_unittest_addresses(),
                peer_address_t(), ANY_PORT, 0),
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

    ~dummy_raft_cluster_t() {
        RAFT_DEBUG("dummy_raft_cluster_t shutting down\n");
        /* We could just let the destructors run, but then we'd have to worry about
        destructor order, so this is safer and clearer */
        for (const auto &pair : members) {
            set_live(pair.first, live_t::dead);
        }
    }

    /* `join()` adds a new member to the cluster. The caller is responsible for running a
    Raft transaction to modify the config to include the new member. */
    raft_member_id_t join() {
        raft_persistent_state_t<dummy_raft_state_t> init_state;
        bool found_init_state = false;
        for (const auto &pair : members) {
            if (pair.second->member_drainer.has()) {
                init_state = pair.second->member->get_raft()->get_state_for_init();
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

    /* `set_live()` puts the given member into the given state. */
    void set_live(const raft_member_id_t &member_id, live_t live) {
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
                    member_id, &mailbox_manager, &i->member_directory, i, ""));
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
    raft_member_id_t find_leader(signal_t *interruptor) {
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

    raft_member_id_t find_leader(int timeout) {
        signal_timer_t timer;
        timer.start(timeout);
        try {
            return find_leader(&timer);
        } catch (const interrupted_exc_t &) {
            crash("find_leader() timed out");
        }
    }

    /* Tries to perform the given change on the member with the given ID. */
    bool try_change(raft_member_id_t id, const uuid_u &change,
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
                        tok = member->propose_change(&change_lock, change, interruptor2);
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
    bool try_config_change(raft_member_id_t id, const raft_config_t &new_config,
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
                        tok = member->propose_config_change(
                            &change_lock, new_config, interruptor2);
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
    std::set<raft_member_id_t> get_all_member_ids() {
        std::set<raft_member_id_t> member_ids;
        for (const auto &pair : members) {
            member_ids.insert(pair.first);
        }
        return member_ids;
    }

    /* `run_on_member()` calls the given function for the `dummy_raft_member_t *` with
    the given ID. If the member is currently dead, it calls the function with a NULL
    pointer. */
    void run_on_member(
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

private:
    class member_info_t :
        public raft_storage_interface_t<dummy_raft_state_t> {
    public:
        member_info_t(dummy_raft_cluster_t *p, const raft_member_id_t &mid,
                const raft_persistent_state_t<dummy_raft_state_t> &ss) :
            parent(p), member_id(mid), stored_state(ss), live(live_t::dead) { }
        member_info_t(member_info_t &&) = default;
        member_info_t &operator=(member_info_t &&) = default;

        const raft_persistent_state_t<dummy_raft_state_t> *get() {
            return &stored_state;
        }
        void write_current_term_and_voted_for(
                raft_term_t current_term,
                raft_member_id_t voted_for,
                signal_t *interruptor) {
            block(interruptor);
            stored_state.current_term = current_term;
            stored_state.voted_for = voted_for;
            block(interruptor);
        }
        void write_log_replace_tail(
                const raft_log_t<dummy_raft_state_t> &log,
                raft_log_index_t first_replaced,
                signal_t *interruptor) {
            block(interruptor);
            guarantee(first_replaced > stored_state.log.prev_index);
            guarantee(first_replaced <= stored_state.log.get_latest_index() + 1);
            if (first_replaced != stored_state.log.get_latest_index() + 1) {
                stored_state.log.delete_entries_from(first_replaced);
            }
            for (raft_log_index_t i = first_replaced; i <= log.get_latest_index(); ++i) {
                stored_state.log.append(log.get_entry_ref(i));
            }
            block(interruptor);
        }
        void write_log_append_one(
                const raft_log_entry_t<dummy_raft_state_t> &entry,
                signal_t *interruptor) {
            block(interruptor);
            stored_state.log.append(entry);
            block(interruptor);
        }
        void write_snapshot(
                const dummy_raft_state_t &snapshot_state,
                const raft_complex_config_t &snapshot_config,
                raft_log_index_t log_prev_index,
                raft_term_t log_prev_index_term,
                signal_t *interruptor) {
            block(interruptor);
            stored_state.snapshot_state = snapshot_state;
            stored_state.snapshot_config = snapshot_config;
            stored_state.log.delete_entries_to(log_prev_index, log_prev_index_term);
            block(interruptor);
        }
        void block(signal_t *interruptor) {
            if (randint(10) != 0) {
                coro_t::yield();
            }
            if (randint(10) == 0) {
                signal_timer_t timer;
                timer.start(randint(30));
                wait_interruptible(&timer, interruptor);
            }
        }

        dummy_raft_cluster_t *parent;
        raft_member_id_t member_id;
        raft_persistent_state_t<dummy_raft_state_t> stored_state;
        watchable_map_var_t<raft_member_id_t, raft_business_card_t<dummy_raft_state_t> >
            member_directory;
        live_t live;
        scoped_ptr_t<raft_networked_member_t<dummy_raft_state_t> > member;
        scoped_ptr_t<auto_drainer_t> member_drainer;
        scoped_ptr_t<watchable_t<raft_business_card_t<dummy_raft_state_t> >
            ::subscription_t> bcard_subs;
    };

    void add_member(
            const raft_member_id_t &member_id,
            raft_persistent_state_t<dummy_raft_state_t> initial_state) {
        scoped_ptr_t<member_info_t> i(new member_info_t(this, member_id, initial_state));
        members[member_id] = std::move(i);
        set_live(member_id, live_t::alive);
    }

    void check_invariants(UNUSED auto_drainer_t::lock_t keepalive) {
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

    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    std::map<raft_member_id_t, scoped_ptr_t<member_info_t> > members;
    auto_drainer_t drainer;
    repeating_timer_t check_invariants_timer;
};

/* `dummy_raft_traffic_generator_t` tries to send operations to the given Raft cluster at
a fixed rate. */
class dummy_raft_traffic_generator_t {
public:
    dummy_raft_traffic_generator_t(dummy_raft_cluster_t *_cluster, int num_threads) :
        cluster(_cluster) {
        for (int i = 0; i < num_threads; ++i) {
            coro_t::spawn_sometime(std::bind(
                &dummy_raft_traffic_generator_t::do_changes, this, drainer.lock()));
        }
    }

    size_t get_num_changes() {
        return committed_changes.size();
    }

    void check_changes_present() {
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

private:
    void do_changes(auto_drainer_t::lock_t keepalive) {
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
    std::set<uuid_u> committed_changes;
    dummy_raft_cluster_t *cluster;
    auto_drainer_t drainer;
};

void do_writes(dummy_raft_cluster_t *cluster, int expect, int ms) {
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

TPTEST(ClusteringRaft, Basic) {
    /* Spin up a Raft cluster and wait for it to elect a leader */
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), nullptr);
    /* Do some writes and check the result */
    do_writes(&cluster, 100, 60000);
}

void failover_test(dummy_raft_cluster_t::live_t failure_type) {
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    do_writes(&cluster, 100, 60000);
    cluster.set_live(member_ids[0], failure_type);
    cluster.set_live(member_ids[1], failure_type);
    do_writes(&cluster, 100, 60000);
    cluster.set_live(member_ids[2], failure_type);
    cluster.set_live(member_ids[3], failure_type);
    cluster.set_live(member_ids[0], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[1], dummy_raft_cluster_t::live_t::alive);
    do_writes(&cluster, 100, 60000);
    cluster.set_live(member_ids[4], failure_type);
    cluster.set_live(member_ids[2], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[3], dummy_raft_cluster_t::live_t::alive);
    do_writes(&cluster, 100, 60000);
    ASSERT_LT(100, traffic_generator.get_num_changes());
    traffic_generator.check_changes_present();
}

TPTEST(ClusteringRaft, Failover) {
    failover_test(dummy_raft_cluster_t::live_t::dead);
}

TPTEST(ClusteringRaft, FailoverIsolated) {
    failover_test(dummy_raft_cluster_t::live_t::isolated);
}

TPTEST(ClusteringRaft, MemberChange) {
    std::vector<raft_member_id_t> member_ids;
    size_t cluster_size = 5;
    dummy_raft_cluster_t cluster(cluster_size, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    for (size_t i = 0; i < 10; ++i) {
        /* Do some test writes */
        do_writes(&cluster, 10, 60000);

        /* Kill one member and do some more test writes */
        cluster.set_live(member_ids[i], dummy_raft_cluster_t::live_t::dead);
        do_writes(&cluster, 10, 60000);

        /* Add a replacement member and do some more test writes */
        member_ids.push_back(cluster.join());
        do_writes(&cluster, 10, 60000);

        /* Update the configuration and do some more test writes */
        raft_config_t new_config;
        for (size_t n = i+1; n < i+1+cluster_size; ++n) {
            new_config.voting_members.insert(member_ids[n]);
        }
        signal_timer_t timeout;
        timeout.start(10000);
        raft_member_id_t leader = cluster.find_leader(&timeout);
        cluster.try_config_change(leader, new_config, &timeout);
        do_writes(&cluster, 10, 60000);
    }
    ASSERT_LT(100, traffic_generator.get_num_changes());
    traffic_generator.check_changes_present();
}

TPTEST(ClusteringRaft, NonVoting) {
    dummy_raft_cluster_t cluster(1, dummy_raft_state_t(), nullptr);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    do_writes(&cluster, 10, 60000);

    raft_config_t new_config;
    new_config.voting_members.insert(cluster.find_leader(1000));
    new_config.voting_members.insert(cluster.join());
    new_config.voting_members.insert(cluster.join());
    new_config.non_voting_members.insert(cluster.join());
    new_config.non_voting_members.insert(cluster.join());
    new_config.non_voting_members.insert(cluster.join());

    signal_timer_t timeout;
    timeout.start(10000);
    raft_member_id_t leader = cluster.find_leader(&timeout);
    cluster.try_config_change(leader, new_config, &timeout);

    do_writes(&cluster, 10, 60000);

    for (const raft_member_id_t &member : new_config.non_voting_members) {
        cluster.set_live(member, dummy_raft_cluster_t::live_t::dead);
    }

    do_writes(&cluster, 10, 60000);

    traffic_generator.check_changes_present();
}

TPTEST(ClusteringRaft, Regression4234) {
    cond_t non_interruptor;
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(2, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    do_writes(&cluster, 10, 60000);
    raft_member_id_t leader = cluster.find_leader(1000);

    raft_config_t new_config;
    for (const raft_member_id_t &member : member_ids) {
        if (member != leader) {
            new_config.voting_members.insert(member);
        }
    }

    cluster.run_on_member(leader,
    [&](dummy_raft_member_t *member, signal_t *interruptor2) {
        guarantee(member != nullptr);
        raft_member_t<dummy_raft_state_t>::change_lock_t change_lock(
            member, &non_interruptor);
        auto tok = member->propose_config_change(
            &change_lock, new_config, interruptor2);
        guarantee(tok.has());
    });

    /* This test is probabilistic. When the bug was present, the test failed about
    20% of the time. */
    nap(randint(30), &non_interruptor);
    cluster.set_live(leader, dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(leader, dummy_raft_cluster_t::live_t::alive);

    do_writes(&cluster, 10, 60000);
    traffic_generator.check_changes_present();
}

}   /* namespace unittest */

