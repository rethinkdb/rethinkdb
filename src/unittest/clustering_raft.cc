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

}   /* namespace unittest */

