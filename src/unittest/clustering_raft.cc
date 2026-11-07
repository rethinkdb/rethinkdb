// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.hpp"
#include "clustering/generic/raft_network.tcc"
#include "concurrency/wait_any.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/clustering_utils_raft.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

TPTEST(ClusteringRaft, Basic) {
    /* Spin up a Raft cluster and wait for it to elect a leader */
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), nullptr);
    /* Do some writes and check the result */
    do_writes_raft(&cluster, 100, 60000);
}

void failover_test(dummy_raft_cluster_t::live_t failure_type) {
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(5, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    do_writes_raft(&cluster, 100, 60000);
    cluster.set_live(member_ids[0], failure_type);
    cluster.set_live(member_ids[1], failure_type);
    do_writes_raft(&cluster, 100, 60000);
    cluster.set_live(member_ids[2], failure_type);
    cluster.set_live(member_ids[3], failure_type);
    cluster.set_live(member_ids[0], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[1], dummy_raft_cluster_t::live_t::alive);
    do_writes_raft(&cluster, 100, 60000);
    cluster.set_live(member_ids[4], failure_type);
    cluster.set_live(member_ids[2], dummy_raft_cluster_t::live_t::alive);
    cluster.set_live(member_ids[3], dummy_raft_cluster_t::live_t::alive);
    do_writes_raft(&cluster, 100, 60000);
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
        do_writes_raft(&cluster, 10, 60000);

        /* Kill one member and do some more test writes */
        cluster.set_live(member_ids[i], dummy_raft_cluster_t::live_t::dead);
        do_writes_raft(&cluster, 10, 60000);

        /* Add a replacement member and do some more test writes */
        member_ids.push_back(cluster.join());
        do_writes_raft(&cluster, 10, 60000);

        /* Update the configuration and do some more test writes */
        raft_config_t new_config;
        for (size_t n = i+1; n < i+1+cluster_size; ++n) {
            new_config.voting_members.insert(member_ids[n]);
        }
        signal_timer_t timeout;
        timeout.start(10000);
        raft_member_id_t leader = cluster.find_leader(&timeout);
        cluster.try_config_change(leader, new_config, &timeout);
        do_writes_raft(&cluster, 10, 60000);
    }
    ASSERT_LT(100, traffic_generator.get_num_changes());
    traffic_generator.check_changes_present();
}

TPTEST(ClusteringRaft, NonVoting) {
    dummy_raft_cluster_t cluster(1, dummy_raft_state_t(), nullptr);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    do_writes_raft(&cluster, 10, 60000);

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

    do_writes_raft(&cluster, 10, 60000);

    for (const raft_member_id_t &member : new_config.non_voting_members) {
        cluster.set_live(member, dummy_raft_cluster_t::live_t::dead);
    }

    do_writes_raft(&cluster, 10, 60000);

    traffic_generator.check_changes_present();
}

TPTEST(ClusteringRaft, Regression4234) {
    cond_t non_interruptor;
    std::vector<raft_member_id_t> member_ids;
    dummy_raft_cluster_t cluster(2, dummy_raft_state_t(), &member_ids);
    dummy_raft_traffic_generator_t traffic_generator(&cluster, 3);
    do_writes_raft(&cluster, 10, 60000);
    raft_member_id_t leader = cluster.find_leader(1000);

    raft_config_t new_config;
    for (const raft_member_id_t &member : member_ids) {
        if (member != leader) {
            new_config.voting_members.insert(member);
        }
    }

    cluster.run_on_member(leader,
    [&](dummy_raft_member_t *member, signal_t *) {
        guarantee(member != nullptr);
        raft_member_t<dummy_raft_state_t>::change_lock_t change_lock(
            member, &non_interruptor);
        auto tok = member->propose_config_change(&change_lock, new_config);
        guarantee(tok.has());
    });

    /* This test is probabilistic. When the bug was present, the test failed about
    20% of the time. */
    nap(randint(30), &non_interruptor);
    cluster.set_live(leader, dummy_raft_cluster_t::live_t::dead);
    cluster.set_live(leader, dummy_raft_cluster_t::live_t::alive);

    do_writes_raft(&cluster, 10, 60000);
    traffic_generator.check_changes_present();
}

class raft_network_test_access_t {
public:
    static bool send_rpc(
            raft_networked_member_t<dummy_raft_state_t> *member,
            const raft_member_id_t &dest,
            const raft_rpc_request_t<dummy_raft_state_t> &request,
            signal_t *interruptor,
            raft_rpc_reply_t *reply_out) {
        return member->send_rpc(dest, request, interruptor, reply_out);
    }
};

/* `in_memory_raft_storage_t` applies writes immediately, without the random delays of
`dummy_raft_cluster_t`'s storage. */
class in_memory_raft_storage_t :
    public raft_storage_interface_t<dummy_raft_state_t> {
public:
    explicit in_memory_raft_storage_t(
            const raft_persistent_state_t<dummy_raft_state_t> &initial_state) :
        state(initial_state) { }

    const raft_persistent_state_t<dummy_raft_state_t> *get() {
        return &state;
    }

    void write_current_term_and_voted_for(
            raft_term_t current_term,
            raft_member_id_t voted_for) {
        state.current_term = current_term;
        state.voted_for = voted_for;
    }

    void write_commit_index(raft_log_index_t commit_index) {
        state.commit_index = commit_index;
    }

    void write_log_replace_tail(
            const raft_log_t<dummy_raft_state_t> &log,
            raft_log_index_t first_replaced) {
        guarantee(first_replaced > state.log.prev_index);
        guarantee(first_replaced <= state.log.get_latest_index() + 1);
        if (first_replaced != state.log.get_latest_index() + 1) {
            state.log.delete_entries_from(first_replaced);
        }
        for (raft_log_index_t i = first_replaced; i <= log.get_latest_index(); ++i) {
            state.log.append(log.get_entry_ref(i));
        }
    }

    void write_log_append_one(const raft_log_entry_t<dummy_raft_state_t> &entry) {
        state.log.append(entry);
    }

    void write_snapshot(
            const dummy_raft_state_t &snapshot_state,
            const raft_complex_config_t &snapshot_config,
            bool clear_log,
            raft_log_index_t log_prev_index,
            raft_term_t log_prev_term,
            raft_log_index_t commit_index) {
        state.snapshot_state = snapshot_state;
        state.snapshot_config = snapshot_config;
        if (clear_log) {
            state.log.entries.clear();
            state.log.prev_index = log_prev_index;
            state.log.prev_term = log_prev_term;
        } else {
            state.log.delete_entries_to(log_prev_index, log_prev_term);
        }
        state.commit_index = commit_index;
    }

private:
    raft_persistent_state_t<dummy_raft_state_t> state;

    DISABLE_COPYING(in_memory_raft_storage_t);
};

/* An in-flight RPC must fail when the destination's directory entry is replaced with a
different mailbox address, even though the key and the transport connection remain. */
TPTEST(ClusteringRaft, RpcAbortsOnMailboxReplacement) {
    typedef raft_business_card_t<dummy_raft_state_t> bcard_t;

    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager(&connectivity_cluster, 'M');
    test_cluster_run_t connectivity_cluster_run(&connectivity_cluster);

    raft_member_id_t sender_id(generate_uuid());
    raft_member_id_t dest_id(generate_uuid());

    /* `dest_id` isn't in the sender's config, so the sender's own Raft logic never sends
    it any RPCs. */
    raft_config_t config;
    config.voting_members.insert(sender_id);
    in_memory_raft_storage_t storage(
        raft_persistent_state_t<dummy_raft_state_t>::make_initial(
            dummy_raft_state_t(), config));
    watchable_map_var_t<raft_member_id_t, bcard_t> directory;

    cond_t rpc_received;
    bcard_t::rpc_mailbox_t unresponsive_mailbox(&mailbox_manager,
        [&](signal_t *,
                const raft_rpc_request_t<dummy_raft_state_t> &,
                const mailbox_addr_t<raft_rpc_reply_t> &) {
            rpc_received.pulse_if_not_already_pulsed();
        });
    bcard_t::rpc_mailbox_t replacement_mailbox(&mailbox_manager,
        [](signal_t *,
                const raft_rpc_request_t<dummy_raft_state_t> &,
                const mailbox_addr_t<raft_rpc_reply_t> &) { });
    directory.set_key(dest_id,
        bcard_t { unresponsive_mailbox.get_address(), optional<raft_term_t>() });

    raft_networked_member_t<dummy_raft_state_t> sender(
        sender_id, &mailbox_manager, &directory, &storage, "",
        raft_start_election_immediately_t::NO);

    signal_timer_t timeout;
    timeout.start(10000);
    bool rpc_result = false;
    bool rpc_interrupted = false;
    cond_t rpc_done;
    coro_t::spawn_sometime([&]() {
        raft_rpc_request_t<dummy_raft_state_t> request;
        raft_rpc_reply_t reply;
        try {
            rpc_result = raft_network_test_access_t::send_rpc(
                &sender, dest_id, request, &timeout, &reply);
        } catch (const interrupted_exc_t &) {
            rpc_interrupted = true;
        }
        rpc_done.pulse();
    });

    {
        wait_any_t received_or_done(&rpc_received, &rpc_done);
        received_or_done.wait_lazily_unordered();
    }
    EXPECT_TRUE(rpc_received.is_pulsed());
    EXPECT_FALSE(rpc_done.is_pulsed());

    EXPECT_TRUE(unresponsive_mailbox.get_address().get_peer() ==
        replacement_mailbox.get_address().get_peer());
    directory.set_key(dest_id,
        bcard_t { replacement_mailbox.get_address(), optional<raft_term_t>() });

    /* The coroutine refers to locals of this function, so it must finish before any
    early return. `timeout` guarantees that it does. */
    rpc_done.wait_lazily_unordered();
    EXPECT_FALSE(rpc_interrupted);
    EXPECT_FALSE(rpc_result);
    EXPECT_TRUE(directory.get_key(dest_id).has_value());
}

}   /* namespace unittest */

