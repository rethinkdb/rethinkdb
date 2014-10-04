// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <map>

#include "unittest/gtest.hpp"

#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_core.tcc"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* `dummy_raft_state_t` is meant to be used as the `state_t` parameter to
`raft_member_t`, with the `change_t` parameter set to `uuid_u`. It just records all the
changes it receives and their order. */
class dummy_raft_state_t {
public:
    std::vector<uuid_u> state;
    void apply_change(const uuid_u &uuid) {
        state.push_back(uuid);
    }
    bool operator==(const dummy_raft_state_t &other) const {
        return state == other.state;
    }
    bool operator!=(const dummy_raft_state_t &other) const {
        return state != other.state;
    }
};

typedef raft_member_t<dummy_raft_state_t, uuid_u> dummy_raft_member_t;

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
            alive_members(std::set<raft_member_id_t>()),
            check_invariants_timer(100, [this]() {
                coro_t::spawn_sometime(boost::bind(
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
                raft_persistent_state_t<dummy_raft_state_t, uuid_u>::make_initial(
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

    /* `join()` adds a new non-voting member to the cluster. The caller is responsible
    for running a Raft transaction to modify the config to include the new member. */
    raft_member_id_t join() {
        raft_member_id_t member_id = generate_uuid();
        add_member(
            member_id,
            raft_persistent_state_t<dummy_raft_state_t, uuid_u>::make_join());
        return member_id;
    }

    /* `set_live()` puts the given member into the given state. */
    void set_live(const raft_member_id_t &member_id, live_t live) {
        member_info_t *i = members.at(member_id).get();
        if (i->drainer.has() && live != live_t::alive) {
            alive_members.apply_atomic_op(
                [&](std::set<raft_member_id_t> *alive_set) -> bool {
                    alive_set->erase(member_id);
                    return true;
                });
            scoped_ptr_t<auto_drainer_t> dummy;
            std::swap(i->drainer, dummy);
            dummy.reset();
        }
        {
            rwlock_acq_t lock_acq(&i->lock, access_t::write);
            if (i->member.has() && live == live_t::dead) {
                i->member.reset();
            }
            if (!i->member.has() && live != live_t::dead) {
                i->member.init(new dummy_raft_member_t(
                    member_id, i, i, i->stored_state));
            }
        }
        if (!i->drainer.has() && live == live_t::alive) {
            i->drainer.init(new auto_drainer_t);
            alive_members.apply_atomic_op(
                [&](std::set<raft_member_id_t> *alive_set) -> bool {
                    alive_set->insert(member_id);
                    return true;
                });
        }
    }

    /* Tries to perform the given change, using an algorithm that mimics a client trying
    to find the leader of the Raft cluster and performing an operation on it. */
    void try_change(const uuid_u &change) {
        /* Search for a node that is alive */
        raft_member_id_t leader = nil_uuid();
        for (const auto &pair : members) {
            if (pair.second->drainer.has()) {
                leader = pair.first;
            }
        }
        /* Follow redirects until we find a node that identifies itself as leader */
        size_t max_redirects = 2;
        while (true) {
            if (leader.is_nil()) {
                return;
            }
            raft_member_id_t new_leader;
            run_on_member(leader, [&](dummy_raft_member_t *member) {
                    if (member == nullptr) {
                        new_leader = nil_uuid();
                    } else {
                        new_leader = member->get_leader();
                    }
                });
            if (leader == new_leader) {
                break;
            } else if (max_redirects == 0) {
                return;
            } else {
                leader = new_leader;
                --max_redirects;
            }
        }
        /* Try to run our change on that leader */
        run_on_member(leader, [&](dummy_raft_member_t *member) {
                if (member != nullptr) {
                    cond_t non_interruptor;
                    member->propose_change_if_leader(change, &non_interruptor);
                }
            });
    }

    /* Blocks until the cluster commits the given change. Call this function at a time
    when a majority of the cluster is alive, and don't bring nodes up or down while this
    function is running. */
    void wait_for_commit(const uuid_u &change) {
        raft_member_id_t chosen = nil_uuid();
        for (const auto &pair : members) {
            if (pair.second->drainer.has()) {
                chosen = pair.first;
            }
        }
        guarantee(!chosen.is_nil(), "wait_for_commit() couldn't find a living member");
        run_on_member(chosen, [&](dummy_raft_member_t *member) {
                guarantee(member != nullptr, "wait_for_commit() lost contact with "
                    "member");
                cond_t non_interruptor;
                member->get_state_machine()->run_until_satisfied(
                    [&](const dummy_raft_state_t &state) {
                        for (const uuid_u &c : state.state) {
                            if (c == change) {
                                return true;
                            }
                        }
                        return false;
                    }, &non_interruptor);
            });
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
    void run_on_member(const raft_member_id_t &member_id,
                       const std::function<void(dummy_raft_member_t *)> &fun) {
        member_info_t *i = members.at(member_id).get();
        rwlock_acq_t acq(&i->lock, access_t::read);
        if (i->member.has()) {
            fun(i->member.get());
        } else {
            fun(nullptr);
        }
    }

private:
    class member_info_t :
        public raft_storage_interface_t<dummy_raft_state_t, uuid_u>,
        public raft_network_interface_t<dummy_raft_state_t, uuid_u> {
    public:
        member_info_t() { }
        member_info_t(member_info_t &&) = default;
        member_info_t &operator=(member_info_t &&) = default;

        /* These are the methods for `raft_network_and_storage_interface_t`. */
        bool send_request_vote_rpc(
                const raft_member_id_t &dest,
                const raft_request_vote_rpc_t &rpc,
                signal_t *interruptor,
                raft_request_vote_reply_t *reply_out) {
            return do_rpc(dest, rpc, &dummy_raft_member_t::on_request_vote_rpc,
                interruptor, reply_out);
        }
        bool send_install_snapshot_rpc(
                const raft_member_id_t &dest,
                const raft_install_snapshot_rpc_t<dummy_raft_state_t> &rpc,
                signal_t *interruptor,
                raft_install_snapshot_reply_t *reply_out) {
            return do_rpc(dest, rpc, &dummy_raft_member_t::on_install_snapshot_rpc,
                interruptor, reply_out);
        }
        bool send_append_entries_rpc(
                const raft_member_id_t &dest,
                const raft_append_entries_rpc_t<uuid_u> &rpc,
                signal_t *interruptor,
                raft_append_entries_reply_t *reply_out) {
            return do_rpc(dest, rpc, &dummy_raft_member_t::on_append_entries_rpc,
                interruptor, reply_out);
        }
        clone_ptr_t<watchable_t<std::set<raft_member_id_t> > > get_connected_members() {
            return parent->alive_members.get_watchable();
        }
        void write_persistent_state(
                const raft_persistent_state_t<dummy_raft_state_t, uuid_u> &
                    persistent_state,
                signal_t *interruptor) {
            block(interruptor);
            stored_state = persistent_state;
            block(interruptor);
        }

        /* `do_rpc()` is a helper for `send_*_rpc()`. */
        template<class rpc_t, class reply_t>
        bool do_rpc(
                const raft_member_id_t &dest,
                const rpc_t &rpc,
                void (dummy_raft_member_t::*call)(const rpc_t &, signal_t *, reply_t *),
                signal_t *interruptor,
                reply_t *reply_out) {
            /* This is convoluted because if `interruptor` is pulsed, we want to return
            immediately but we don't want to pulse the interruptor for `on_*_rpc()`. So
            we have to spawn a separate coroutine for `on_*_rpc()`. The coroutine
            communicates with `do_rpc()` through `reply_info_t`, which is stored on the
            heap so it remains valid even if `do_rpc()` is interrupted. */
            class reply_info_t {
            public:
                cond_t done;
                bool ok;
                reply_t reply;
            };
            std::shared_ptr<reply_info_t> reply_info(new reply_info_t);
            block(interruptor);
            coro_t::spawn_sometime(
                [this, dest, rpc, call, reply_info] () {
                    member_info_t *other = parent->members.at(dest).get();
                    if (other->drainer.has()) {
                        auto_drainer_t::lock_t keepalive(other->drainer.get());
                        try {
                            block(keepalive.get_drain_signal());
                            (other->member.get()->*call)(
                                rpc, keepalive.get_drain_signal(), &reply_info->reply);
                            block(keepalive.get_drain_signal());
                            reply_info->ok = true;
                        } catch (interrupted_exc_t) {
                            reply_info->ok = false;
                        }
                    } else {
                        reply_info->ok = false;
                    }
                    reply_info->done.pulse();
                });
            wait_interruptible(&reply_info->done, interruptor);
            block(interruptor);
            if (reply_info->ok) {
                *reply_out = reply_info->reply;
            }
            return reply_info->ok;
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
        raft_persistent_state_t<dummy_raft_state_t, uuid_u> stored_state;
        /* If the member is alive, `member` and `drainer` are set. If the member is
        isolated, `member` is set but `drainer` is empty. If the member is dead, both are
        empty. `lock` should be acquired in read mode to access `member` in any way, and
        in write mode to create or destroy `member`. */
        scoped_ptr_t<dummy_raft_member_t> member;
        scoped_ptr_t<auto_drainer_t> drainer;
        rwlock_t lock;
    };

    void add_member(
            const raft_member_id_t &member_id,
            raft_persistent_state_t<dummy_raft_state_t, uuid_u> initial_state) {
        scoped_ptr_t<member_info_t> i(new member_info_t);
        i->parent = this;
        i->member_id = member_id;
        i->stored_state = initial_state;
        members[member_id] = std::move(i);
        set_live(member_id, live_t::alive);
    }

    void check_invariants(UNUSED auto_drainer_t::lock_t keepalive) {
        std::set<dummy_raft_member_t *> member_ptrs;
        std::vector<scoped_ptr_t<rwlock_acq_t> > rwlock_acqs;
        for (auto &pair : members) {
            rwlock_acqs.push_back(scoped_ptr_t<rwlock_acq_t>(
                new rwlock_acq_t(&pair.second->lock, access_t::read)));
            if (pair.second->member.has()) {
                member_ptrs.insert(pair.second->member.get());
            }
        }
        dummy_raft_member_t::check_invariants(member_ptrs);
    }

    std::map<raft_member_id_t, scoped_ptr_t<member_info_t> > members;
    watchable_variable_t<std::set<raft_member_id_t> > alive_members;
    auto_drainer_t drainer;
    repeating_timer_t check_invariants_timer;
};

/* `dummy_raft_traffic_generator_t` tries to send operations to the given Raft cluster at
a fixed rate. */
class dummy_raft_traffic_generator_t {
public:
    dummy_raft_traffic_generator_t(dummy_raft_cluster_t *_cluster, int ms) :
        cluster(_cluster),
        timer(ms, [this]() {
            coro_t::spawn_sometime(boost::bind(
                &dummy_raft_traffic_generator_t::do_change, this,
                auto_drainer_t::lock_t(&drainer)));
            })
        { }
    
private:
    void do_change(UNUSED auto_drainer_t::lock_t keepalive) {
        uuid_u change = generate_uuid();
        cluster->try_change(change);
    }
    dummy_raft_cluster_t *cluster;
    auto_drainer_t drainer;
    repeating_timer_t timer;
};

/* TODO: These unit tests are inadequate, in several ways:
  * They use timeouts instead of getting a notification when the Raft cluster is ready.
  * They should assert that transactions go through when the Raft cluster says it is
    ready.
  * They don't test configuration changes.
It will be much easier to fix these issues once `raft_member_t` exposes a good
user-facing API. So I don't want to implement real unit tests until after the user-facing
API is overhauled. */

TPTEST(ClusteringRaft, Basic) {
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 10);
    /* Election timeouts are 1000-2000ms, so we want to wait comfortably longer than
    that. */
    nap(5000);
    cluster.run_on_member(member_ids[0], [](dummy_raft_member_t *member) {
        /* Make sure at least some transactions got through */
        dummy_raft_state_t state = member->get_state_machine()->get();
        guarantee(state.state.size() > 100);
    });
}

TPTEST(ClusteringRaft, Failover) {
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 10);
    nap(5000);
    cluster.set_live(member_ids[0], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[1], dummy_raft_cluster_t::live_t::dead);
    nap(5000);
    cluster.set_live(member_ids[2], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[3], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[0], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[1], dummy_raft_cluster_t::live_t::alive);
    nap(5000);
    cluster.set_live(member_ids[4], dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(member_ids[2], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[3], dummy_raft_cluster_t::live_t::alive);
    nap(5000);
    cluster.run_on_member(member_ids[0], [](dummy_raft_member_t *member) {
        /* Make sure at least some transactions got through */
        dummy_raft_state_t state = member->get_state_machine()->get();
        guarantee(state.state.size() > 100);
    });
}

}   /* namespace unittest */

