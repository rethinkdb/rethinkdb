// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/reactor/blueprint.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/test_cluster_group.hpp"

namespace unittest {

TPTEST(ClusteringReactor, OneShardOnePrimaryOneNodeStartupShutdown) {
    test_cluster_group_t cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();
}

TPTEST(ClusteringReactor, OneShardOnePrimaryOneSecondaryStartupShutdowntest) {
    test_cluster_group_t cluster_group(3);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");

    cluster_group.run_queries();
}

TPTEST(ClusteringReactor, TwoShardsTwoNodes) {
    test_cluster_group_t cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("ps,sp"));

    cluster_group.wait_until_blueprint_is_satisfied("ps,sp");

    cluster_group.run_queries();
}

TPTEST(ClusteringReactor, RoleSwitchingTest) {
    test_cluster_group_t cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("n,p"));
    cluster_group.wait_until_blueprint_is_satisfied("n,p");

    cluster_group.run_queries();
}

TPTEST(ClusteringReactor, OtherRoleSwitchingTest) {
    test_cluster_group_t cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("s,p"));
    cluster_group.wait_until_blueprint_is_satisfied("s,p");

    cluster_group.run_queries();
}

TPTEST(ClusteringReactor, AddSecondaryTest) {
    test_cluster_group_t cluster_group(3);
    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("p,s,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,s");
    cluster_group.run_queries();
}

TPTEST(ClusteringReactor, ReshardingTest) {
    test_cluster_group_t cluster_group(2);

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

TPTEST(ClusteringReactor, LessGracefulReshardingTest) {
    test_cluster_group_t cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pn,np"));
    cluster_group.wait_until_blueprint_is_satisfied("pn,np");
    cluster_group.run_queries();
}

} // namespace unittest
