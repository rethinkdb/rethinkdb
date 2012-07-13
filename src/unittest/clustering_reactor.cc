#include "unittest/gtest.hpp"

#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "mock/test_cluster_group.hpp"
#include "mock/unittest_utils.hpp"

using mock::dummy_protocol_t;

namespace unittest {

void runOneShardOnePrimaryOneNodeStartupShutdowntest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OneShardOnePrimaryOneNodeStartupShutdown) {
    mock::run_in_thread_pool(&runOneShardOnePrimaryOneNodeStartupShutdowntest);
}

void runOneShardOnePrimaryOneSecondaryStartupShutdowntest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(3);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OneShardOnePrimaryOneSecondaryStartupShutdowntest) {
    mock::run_in_thread_pool(&runOneShardOnePrimaryOneSecondaryStartupShutdowntest);
}

void runTwoShardsTwoNodes() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("ps,sp"));

    cluster_group.wait_until_blueprint_is_satisfied("ps,sp");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, TwoShardsTwoNodes) {
    mock::run_in_thread_pool(&runTwoShardsTwoNodes);
}

void runRoleSwitchingTest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("n,p"));
    cluster_group.wait_until_blueprint_is_satisfied("n,p");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, RoleSwitchingTest) {
    mock::run_in_thread_pool(&runRoleSwitchingTest);
}

void runOtherRoleSwitchingTest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("s,p"));
    cluster_group.wait_until_blueprint_is_satisfied("s,p");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OtherRoleSwitchingTest) {
    mock::run_in_thread_pool(&runOtherRoleSwitchingTest);
}

void runAddSecondaryTest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(3);
    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("p,s,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,s");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, AddSecondaryTest) {
    mock::run_in_thread_pool(&runAddSecondaryTest);
}

void runReshardingTest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

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
    mock::run_in_thread_pool(&runReshardingTest);
}

void runLessGracefulReshardingTest() {
    mock::test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pn,np"));
    cluster_group.wait_until_blueprint_is_satisfied("pn,np");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, LessGracefulReshardingTest) {
    mock::run_in_thread_pool(&runLessGracefulReshardingTest);
}

} // namespace unittest
