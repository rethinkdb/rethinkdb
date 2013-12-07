// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/reactor/blueprint.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/test_cluster_group.hpp"

using mock::dummy_protocol_t;

namespace unittest {

void runOneShardOnePrimaryOneNodeStartupShutdowntest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OneShardOnePrimaryOneNodeStartupShutdown) {
    unittest::run_in_thread_pool(&runOneShardOnePrimaryOneNodeStartupShutdowntest);
}

void runOneShardOnePrimaryOneSecondaryStartupShutdowntest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(3);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OneShardOnePrimaryOneSecondaryStartupShutdowntest) {
    unittest::run_in_thread_pool(&runOneShardOnePrimaryOneSecondaryStartupShutdowntest);
}

void runTwoShardsTwoNodes() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("ps,sp"));

    cluster_group.wait_until_blueprint_is_satisfied("ps,sp");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, TwoShardsTwoNodes) {
    unittest::run_in_thread_pool(&runTwoShardsTwoNodes);
}

void runRoleSwitchingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("n,p"));
    cluster_group.wait_until_blueprint_is_satisfied("n,p");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, RoleSwitchingTest) {
    unittest::run_in_thread_pool(&runRoleSwitchingTest);
}

void runOtherRoleSwitchingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("s,p"));
    cluster_group.wait_until_blueprint_is_satisfied("s,p");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OtherRoleSwitchingTest) {
    unittest::run_in_thread_pool(&runOtherRoleSwitchingTest);
}

void runAddSecondaryTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(3);
    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("p,s,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,s");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, AddSecondaryTest) {
    unittest::run_in_thread_pool(&runAddSecondaryTest);
}

void runReshardingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pp,ns"));
    cluster_group.wait_until_blueprint_is_satisfied("pp,ns");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pn,np"));
    cluster_group.wait_until_blueprint_is_satisfied("pn,np");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, ReshardingTest) {
    unittest::run_in_thread_pool(&runReshardingTest);
}

void runLessGracefulReshardingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pn,np"));
    cluster_group.wait_until_blueprint_is_satisfied("pn,np");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, LessGracefulReshardingTest) {
    unittest::run_in_thread_pool(&runLessGracefulReshardingTest);
}

void runViceprimaryTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);
    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,v"));
    cluster_group.wait_until_blueprint_is_satisfied("p,v");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, ViceprimaryTest) {
    unittest::run_in_thread_pool(&runViceprimaryTest);
}

} // namespace unittest
