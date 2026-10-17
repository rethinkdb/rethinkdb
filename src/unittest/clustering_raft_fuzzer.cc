// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.hpp"
#include "clustering/generic/raft_network.tcc"
#include "unittest/clustering_utils.hpp"
#include "unittest/clustering_utils_raft.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void print_config(RAFT_DEBUG_VAR const raft_config_t &config) {
    for (RAFT_DEBUG_VAR auto const &id : config.voting_members) {
        RAFT_DEBUG("  member %s: voting\n", show_member_id(id).c_str());
    }
    for (RAFT_DEBUG_VAR auto const &id : config.non_voting_members) {
        RAFT_DEBUG("  member %s: non-voting\n", show_member_id(id).c_str());
    }
}

void print_config(RAFT_DEBUG_VAR const raft_complex_config_t &config) {
    RAFT_DEBUG("raft_complex_config_t state (%s):\n", config.is_joint_consensus() ? "joint consensus" : "normal");
    print_config(config.config);
    if (config.is_joint_consensus()) {
        RAFT_DEBUG(" new config:\n");
        print_config(config.new_config.get());
    }
}

void add_voting_members(const raft_complex_config_t &config,
                        std::set<raft_member_id_t> *voters) {
    voters->insert(config.config.voting_members.begin(),
                   config.config.voting_members.end());

    if (config.is_joint_consensus()) {
        voters->insert(config.new_config->voting_members.begin(),
                       config.new_config->voting_members.end());
    }
}

/* Configuration at `index`, replaying config entries after the snapshot. `index` must
be present in the log (or be the snapshot's last included index). */
raft_complex_config_t config_at_index(
        const raft_persistent_state_t<dummy_raft_state_t> &state,
        raft_log_index_t index) {
    guarantee(index >= state.log.prev_index);
    guarantee(index <= state.log.get_latest_index());

    raft_complex_config_t config = state.snapshot_config;
    for (raft_log_index_t i = state.log.prev_index + 1; i <= index; ++i) {
        const raft_log_entry_t<dummy_raft_state_t> &entry = state.log.get_entry_ref(i);
        if (entry.type == raft_log_entry_type_t::config) {
            config = entry.config.get();
        }
    }

    return config;
}

/* `active_config` can miss a change that was still in flight when the fuzzer timer
fired, because `run_on_member` swallows `interrupted_exc_t`. Freeze every member, then
revive voters from the highest persisted commit index plus any later uncommitted config
entries. Leaving everyone else dead stops removed members from starting elections, and
recreating the survivors clears the in-memory election backoff. */
void revive_voters_from_log(dummy_raft_cluster_t *cluster) {
    std::set<raft_member_id_t> all_members = cluster->get_all_member_ids();
    for (auto const &id : all_members) {
        cluster->set_live(id, dummy_raft_cluster_t::live_t::dead);
    }

    std::vector<raft_persistent_state_t<dummy_raft_state_t> > states;
    raft_log_index_t max_commit_index = 0;
    for (auto const &id : all_members) {
        states.push_back(cluster->copy_persistent_state(id));
        if (states.back().commit_index > max_commit_index) {
            max_commit_index = states.back().commit_index;
        }
    }

    std::set<raft_member_id_t> voters;
    bool found_committed_config = false;
    raft_complex_config_t committed_config;

    for (auto const &state : states) {
        guarantee(state.commit_index >= state.log.prev_index);
        guarantee(state.log.get_latest_index() >= state.commit_index);

        if (state.commit_index == max_commit_index) {
            raft_complex_config_t config = config_at_index(state, max_commit_index);

            if (!found_committed_config) {
                committed_config = config;
                found_committed_config = true;
            } else {
                guarantee(committed_config == config);
            }
        }

        if (state.log.get_latest_index() <= max_commit_index) {
            continue;
        }

        for (raft_log_index_t i = max_commit_index + 1; i <= state.log.get_latest_index(); ++i) {
            const raft_log_entry_t<dummy_raft_state_t> &entry = state.log.get_entry_ref(i);
            if (entry.type == raft_log_entry_type_t::config) {
                add_voting_members(entry.config.get(), &voters);
            }
        }
    }

    guarantee(found_committed_config);
    add_voting_members(committed_config, &voters);
    guarantee(!voters.empty());

    RAFT_DEBUG("reviving %zu voters from persisted log (commit index %" PRIu64 ")\n",
               voters.size(), max_commit_index);

    print_config(committed_config);

    for (auto const &id : voters) {
        guarantee(all_members.count(id) == 1);
        RAFT_DEBUG("reviving member %s\n", show_member_id(id).c_str());
        cluster->set_live(id, dummy_raft_cluster_t::live_t::alive);
    }
}

class raft_fuzzer_t {
public:
    explicit raft_fuzzer_t(size_t initial_member_count) :
            cluster(initial_member_count, dummy_raft_state_t(), &member_ids)
    {
        raft_config_t config;
        for (auto &&id : member_ids) {
            config.voting_members.insert(id);
        }
        active_config.config = config;
    }

    void run(uint64_t timeout_ms) {
        signal_timer_t interruptor;
        interruptor.start(timeout_ms);

        for (RAFT_DEBUG_VAR auto const &id : member_ids) {
            RAFT_DEBUG("initial member: %s\n", show_member_id(id).c_str());
        }

        cond_t states_done, config_done, writes_done;
        coro_t::spawn_sometime(std::bind(&raft_fuzzer_t::fuzz_states,
                                         this, &states_done, &interruptor));
        coro_t::spawn_sometime(std::bind(&raft_fuzzer_t::fuzz_config,
                                         this, &config_done, &interruptor));
        coro_t::spawn_sometime(std::bind(&raft_fuzzer_t::fuzz_writes,
                                         this, &writes_done, &interruptor));
        states_done.wait();
        config_done.wait();
        writes_done.wait();
        RAFT_DEBUG("stopped fuzzer sub-coroutines\n");

        print_config(active_config);
        cluster.print_state();

        // `active_config` is only the fuzzer's in-run tracker. Choose voters from the
        // persisted log so an interrupted config change cannot leave the real quorum dead.
        revive_voters_from_log(&cluster);
        cluster.print_state();

        RAFT_DEBUG("running final test writes\n");
        do_writes_raft(&cluster, 100, 60000);

        if (::testing::Test::HasFailure()) {
            return;
        }

        RAFT_DEBUG("running final test config change\n");
        try {
            signal_timer_t final_interruptor;
            final_interruptor.start(30000); // Allow up to 30 seconds to finish the final verification
            raft_member_id_t leader = cluster.find_leader(&final_interruptor);
            raft_config_t final_config;
            final_config.voting_members.insert(leader);

            cluster.run_on_member(leader, [&](dummy_raft_member_t *m, signal_t *) {
                    m->get_readiness_for_config_change()->run_until_satisfied([&](bool v) {
                        return v;
                    }, &final_interruptor);
                });

            bool res = cluster.try_config_change(leader, final_config, &final_interruptor);
            if (!res) {
                ADD_FAILURE() << "final config change was not committed";
            }
        } catch (const interrupted_exc_t &) {
            ADD_FAILURE() << "final raft fuzzer verification timed out";
        }
    }

private:
    void fuzz_states(cond_t *done, signal_t *interruptor) {
        try {
            while (true) {
                signal_timer_t timeout;
                timeout.start(rng.randint(800));
                wait_interruptible(&timeout, interruptor);

                if (rng.randint(4) == 0) {
                    // We can only add a member if there is at least one alive member
                    for (auto &&id : member_ids) {
                        if (cluster.get_live(id) == dummy_raft_cluster_t::live_t::alive) {
                            raft_member_id_t id2 = cluster.join();
                            member_ids.push_back(id2);
                            break;
                        }
                    }
                } else {
                    raft_member_id_t id = member_ids[rng.randint(member_ids.size())];
                    dummy_raft_cluster_t::live_t new_state =
                        static_cast<dummy_raft_cluster_t::live_t>(rng.randint(3));
                    cluster.set_live(id, new_state);
                }
            }
        } catch (const interrupted_exc_t &) { /* pass */ }
        done->pulse();
    }

    void fuzz_config(cond_t *done, signal_t *interruptor) {
        try {
            while (true) {
                signal_timer_t timeout;
                timeout.start(rng.randint(100));
                wait_interruptible(&timeout, interruptor);

                raft_config_t config;
                for (auto &&id : member_ids) {
                    switch (rng.randint(3)) {
                    case 0:
                        config.voting_members.insert(id);
                        break;
                    case 1:
                        config.non_voting_members.insert(id);
                        break;
                    case 2:
                        /* Make this server not a member at all */
                        break;
                    default: unreachable();
                    }
                }

                if (rng.randint(3) == 0 && config.voting_members.size() > 0) {
                    raft_member_id_t leader = cluster.find_leader(interruptor);
                    // Check if we're in the middle of a joint consensus
                    cluster.run_on_member(leader, [&] (dummy_raft_member_t *m, signal_t *) {
                            scoped_ptr_t<dummy_raft_member_t::change_lock_t> lock;
                            lock.init(new dummy_raft_member_t::change_lock_t(m, interruptor));
                            bool is_joint = (m->get_committed_state()->get().config !=
                                             m->get_latest_state()->get().config) ||
                                            m->get_committed_state()->get().config.is_joint_consensus();
                            if (!is_joint) {
                                RAFT_DEBUG("Pre-config change, not in joint consensus\n");
                                raft_config_t committed = m->get_committed_state()->get().config.config;
                                if (active_config.is_joint_consensus()) {
                                    guarantee(committed == active_config.config ||
                                              committed == active_config.new_config.get());
                                } else {
                                    guarantee(committed == active_config.config);
                                }
                                active_config.config = m->get_committed_state()->get().config.config;
                                active_config.new_config.reset();
                            }

                            RAFT_DEBUG("Performing config change to:\n");
                            print_config(config);

                            scoped_ptr_t<dummy_raft_member_t::change_token_t> tok =
                                m->propose_config_change(lock.get(), config);
                            lock.reset();

                            bool res = false;
                            if (tok.has()) {
                                wait_interruptible(tok->get_ready_signal(), interruptor);
                                res = tok->assert_get_value();
                            }

                            if (is_joint) {
                                guarantee(!res);
                                RAFT_DEBUG("Config change failed - in joint consensus mode\n");
                            } else if (res) {
                                active_config.config = config;
                                active_config.new_config.reset();
                                RAFT_DEBUG("Config change succeeded\n");
                            } else {
                                active_config.new_config.set(config);
                                RAFT_DEBUG("Config change indeterminate\n");
                            }
                            print_config(active_config);
                        });

                }
            }
        } catch (const interrupted_exc_t &) { /* pass */ }
        done->pulse();
    }

    void fuzz_writes(cond_t *done, signal_t *interruptor) {
        try {
            while (true) {
                signal_timer_t timeout;
                timeout.start(rng.randint(100));
                wait_interruptible(&timeout, interruptor);

                raft_member_id_t leader = cluster.find_leader(interruptor);
                cluster.try_change(leader, generate_uuid(), interruptor);
            }
        } catch (const interrupted_exc_t &) { /* pass */ }
        done->pulse();
    }

    rng_t rng;
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster;
    raft_complex_config_t active_config;
};

TPTEST(ClusteringRaft, Fuzzer) {
#ifdef ENABLE_RAFT_DEBUG
    /* If we run multiple concurrent coroutines with raft debugging enabled, the logs
    will get interleaved and the result will be difficult to understand. */
    int num_concurrent = 1;
#else
    int num_concurrent = 10;
#endif
    pmap(num_concurrent, [](int) {
        raft_fuzzer_t fuzzer(randint(7) + 1);
        fuzzer.run(20000);
    });
}

}   /* namespace unittest */

