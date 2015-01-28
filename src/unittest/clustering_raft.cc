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
            raft_member_id_t member_id = generate_uuid();
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
    }

    ~dummy_raft_cluster_t() {
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
        raft_member_id_t member_id = generate_uuid();
        add_member(member_id, init_state);
        return member_id;
    }

    /* `set_live()` puts the given member into the given state. */
    void set_live(const raft_member_id_t &member_id, live_t live) {
        switch (live) {
            case live_t::alive:
                debugf("%s state alive\n", uuid_to_str(member_id).substr(0,4).c_str());
                break;
            case live_t::isolated:
                debugf("%s state isolated\n", uuid_to_str(member_id).substr(0,4).c_str());
                break;
            case live_t::dead:
                debugf("%s state dead\n", uuid_to_str(member_id).substr(0,4).c_str());
                break;
            default: unreachable();
        }
        member_info_t *i = members.at(member_id).get();
        if (i->rpc_drainer.has() && live != live_t::alive) {
            member_directory.delete_key(member_id);
            scoped_ptr_t<auto_drainer_t> dummy;
            std::swap(i->rpc_drainer, dummy);
            dummy.reset();
        }
        {
            if (i->member.has() && live == live_t::dead) {
                scoped_ptr_t<auto_drainer_t> dummy;
                std::swap(i->member_drainer, dummy);
                dummy.reset();
                i->member.reset();
            }
            if (!i->member.has() && live != live_t::dead) {
                i->member.init(new raft_networked_member_t<dummy_raft_state_t>(
                    member_id, &mailbox_manager, &member_directory, i, i->stored_state));
                i->member_drainer.init(new auto_drainer_t);
            }
        }
        if (!i->rpc_drainer.has() && live == live_t::alive) {
            i->rpc_drainer.init(new auto_drainer_t);
            member_directory.set_key_no_equals(
                member_id, i->member->get_business_card());
        }
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
        debugf("%s propose_change(%s) begin\n",
            uuid_to_str(id).substr(0,4).c_str(),
            uuid_to_str(change).substr(0,2).c_str());
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
            } else {
                debugf("%s propose_change(%s) failing because dead\n",
                    uuid_to_str(id).substr(0,4).c_str(),
                    uuid_to_str(change).substr(0,2).c_str());
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
        member_info_t() { }
        member_info_t(member_info_t &&) = default;
        member_info_t &operator=(member_info_t &&) = default;

        void write_persistent_state(
                const raft_persistent_state_t<dummy_raft_state_t> &persistent_state,
                signal_t *interruptor) {
            block(interruptor);
            stored_state = persistent_state;
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
        /* If the member is alive, `member`, `member_drainer`, and `rpc_drainer` are set.
        If the member is isolated, `member` and `member_drainer` are set but
        `rpc_drainer` is empty. If the member is dead, all are empty. */
        scoped_ptr_t<raft_networked_member_t<dummy_raft_state_t> > member;
        scoped_ptr_t<auto_drainer_t> member_drainer, rpc_drainer;
    };

    void add_member(
            const raft_member_id_t &member_id,
            raft_persistent_state_t<dummy_raft_state_t> initial_state) {
        scoped_ptr_t<member_info_t> i(new member_info_t);
        i->parent = this;
        i->member_id = member_id;
        i->stored_state = initial_state;
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
        dummy_raft_member_t::check_invariants(member_ptrs);
    }

    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    std::map<raft_member_id_t, scoped_ptr_t<member_info_t> > members;
    watchable_map_var_t<raft_member_id_t, raft_business_card_t<dummy_raft_state_t> >
        member_directory;
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

    void check_changes_present(const dummy_raft_state_t &state) {
        std::set<uuid_u> all_changes;
        for (const uuid_u &change : state.state) {
            all_changes.insert(change);
        }
        for (const uuid_u &change : committed_changes) {
            ASSERT_TRUE(all_changes.count(change) == 1);
        }
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

void do_writes(dummy_raft_cluster_t *cluster, raft_member_id_t leader, int ms, int expect) {
    dummy_raft_traffic_generator_t traffic_generator(cluster, 3);
    cond_t non_interruptor;
    nap(ms, &non_interruptor);
    ASSERT_LT(expect, traffic_generator.get_num_changes());
    cluster->run_on_member(leader, [&](dummy_raft_member_t *member, signal_t *) {
        dummy_raft_state_t state = member->get_committed_state()->get().state;
        traffic_generator.check_changes_present(state);
    });
}

TPTEST(ClusteringRaft, Basic) {
    /* Spin up a Raft cluster and wait for it to elect a leader */
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), nullptr);
    raft_member_id_t leader = cluster.find_leader(60000);
    /* Do some writes and check the result */
    do_writes(&cluster, leader, 2000, 100);
}

TPTEST(ClusteringRaft, Failover) {
    debugf("failover test begin\n");
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    raft_member_id_t leader = cluster.find_leader(60000);
    debugf("failover elected 1st leader: %s\n",
        uuid_to_str(leader).substr(0,4).c_str());
    do_writes(&cluster, leader, 2000, 100);
    debugf("failover did 1st writes\n");
    cluster.set_live(member_ids[0], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[1], dummy_raft_cluster_t::live_t::dead);
    leader = cluster.find_leader(60000);
    debugf("failover elected 2nd leader: %s\n",
        uuid_to_str(leader).substr(0,4).c_str());
    do_writes(&cluster, leader, 2000, 100);
    debugf("failover did 2nd writes\n");
    cluster.set_live(member_ids[2], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[3], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[0], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[1], dummy_raft_cluster_t::live_t::alive);
    leader = cluster.find_leader(60000);
    debugf("failover elected 3rd leader: %s\n",
        uuid_to_str(leader).substr(0,4).c_str());
    do_writes(&cluster, leader, 2000, 100);
    debugf("failover did 3rd writes\n");
    cluster.set_live(member_ids[4], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[2], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[3], dummy_raft_cluster_t::live_t::alive);
    leader = cluster.find_leader(60000);
    debugf("failover elected 4th leader: %s\n",
        uuid_to_str(leader).substr(0,4).c_str());
    do_writes(&cluster, leader, 2000, 100);
    debugf("failover did 4th writes\n");
    ASSERT_LT(100, traffic_generator.get_num_changes());
    cluster.run_on_member(leader, [&](dummy_raft_member_t *member, signal_t *) {
        dummy_raft_state_t state = member->get_committed_state()->get().state;
        traffic_generator.check_changes_present(state);
    });
}

TPTEST(ClusteringRaft, MemberChange) {
    std::vector<raft_member_id_t> member_ids;
    size_t cluster_size = 5;
    dummy_raft_cluster_t cluster(cluster_size, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    for (size_t i = 0; i < 10; ++i) {
        /* Do some test writes */
        raft_member_id_t leader = cluster.find_leader(10000);
        do_writes(&cluster, leader, 2000, 10);

        /* Kill one member and do some more test writes */
        cluster.set_live(member_ids[i], dummy_raft_cluster_t::live_t::dead);
        leader = cluster.find_leader(10000);
        do_writes(&cluster, leader, 2000, 10);

        /* Add a replacement member and do some more test writes */
        member_ids.push_back(cluster.join());
        do_writes(&cluster, leader, 2000, 10);

        /* Update the configuration and do some more test writes */
        raft_config_t new_config;
        for (size_t n = i+1; n < i+1+cluster_size; ++n) {
            new_config.voting_members.insert(member_ids[n]);
        }
        signal_timer_t timeout;
        timeout.start(10000);
        cluster.try_config_change(leader, new_config, &timeout);
        do_writes(&cluster, leader, 2000, 10);
    }
    ASSERT_LT(100, traffic_generator.get_num_changes());
}

}   /* namespace unittest */

