// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "arch/timing.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/directory/map_read_manager.hpp"
#include "rpc/directory/map_write_manager.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "unittest/gtest.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* `OneNode` starts a single directory node, then shuts it down again. */
TPTEST(RPCDirectoryTest, OneNode) {
    connectivity_cluster_t c;
    directory_read_manager_t<int> read_manager(&c, 'D');
    watchable_variable_t<int> watchable(5);
    directory_write_manager_t<int> write_manager(&c, 'D', watchable.get_watchable());
    test_cluster_run_t cr(&c);
    let_stuff_happen();
}

/* `ThreeNodes` starts three directory nodes, lets them contact each other, and
then shuts them down again. */
TPTEST(RPCDirectoryTest, ThreeNodes) {
    connectivity_cluster_t c1, c2, c3;
    directory_read_manager_t<int> rm1(&c1, 'D'), rm2(&c2, 'D'), rm3(&c3, 'D');
    watchable_variable_t<int> w1(101), w2(202), w3(303);
    directory_write_manager_t<int> wm1(&c1, 'D', w1.get_watchable()),
                                   wm2(&c2, 'D', w2.get_watchable()),
                                   wm3(&c3, 'D', w3.get_watchable());
    test_cluster_run_t cr1(&c1);
    test_cluster_run_t cr2(&c2);
    test_cluster_run_t cr3(&c3);
    cr2.join(get_cluster_local_address(&c1), 0);
    cr3.join(get_cluster_local_address(&c1), 0);
    let_stuff_happen();
}

/* `Exchange` tests that directory nodes receive values from their peers. */
TPTEST(RPCDirectoryTest, Exchange) {
    connectivity_cluster_t c1, c2, c3;
    directory_read_manager_t<int> rm1(&c1, 'D'), rm2(&c2, 'D'), rm3(&c3, 'D');
    watchable_variable_t<int> w1(101), w2(202), w3(303);
    directory_write_manager_t<int> wm1(&c1, 'D', w1.get_watchable()),
                                   wm2(&c2, 'D', w2.get_watchable()),
                                   wm3(&c3, 'D', w3.get_watchable());
    test_cluster_run_t cr1(&c1);
    test_cluster_run_t cr2(&c2);
    test_cluster_run_t cr3(&c3);
    cr2.join(get_cluster_local_address(&c1), 0);
    cr3.join(get_cluster_local_address(&c1), 0);
    let_stuff_happen();
    EXPECT_EQ(1u, rm1.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(101, rm1.get_root_view()->get().get_inner().find(c1.get_me())->second);
    EXPECT_EQ(1u, rm1.get_root_view()->get().get_inner().count(c2.get_me()));
    EXPECT_EQ(202, rm1.get_root_view()->get().get_inner().find(c2.get_me())->second);
    EXPECT_EQ(1u, rm3.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(101, rm3.get_root_view()->get().get_inner().find(c1.get_me())->second);
}

/* `Update` tests that directory nodes see updates from their peers. */
TPTEST(RPCDirectoryTest, Update) {
    connectivity_cluster_t c1, c2, c3;
    directory_read_manager_t<int> rm1(&c1, 'D'), rm2(&c2, 'D'), rm3(&c3, 'D');
    watchable_variable_t<int> w1(101), w2(202), w3(303);
    directory_write_manager_t<int> wm1(&c1, 'D', w1.get_watchable()),
                                   wm2(&c2, 'D', w2.get_watchable()),
                                   wm3(&c3, 'D', w3.get_watchable());
    test_cluster_run_t cr1(&c1);
    test_cluster_run_t cr2(&c2);
    test_cluster_run_t cr3(&c3);
    cr2.join(get_cluster_local_address(&c1), 0);
    cr3.join(get_cluster_local_address(&c1), 0);
    let_stuff_happen();
    w1.set_value(151);
    let_stuff_happen();
    ASSERT_EQ(1u, rm1.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(151, rm1.get_root_view()->get().get_inner().find(c1.get_me())->second);
    ASSERT_EQ(1u, rm2.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(151, rm2.get_root_view()->get().get_inner().find(c1.get_me())->second);
    ASSERT_EQ(1u, rm3.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(151, rm3.get_root_view()->get().get_inner().find(c1.get_me())->second);
}

/* `MapUpdate` tests that directory nodes see updates from their peers when using
`directory_map_*_manager_t`. */
TPTEST(RPCDirectoryTest, MapUpdate) {
    connectivity_cluster_t c1, c2, c3;
    directory_map_read_manager_t<int, int> rm1(&c1, 'D'), rm2(&c2, 'D'), rm3(&c3, 'D');
    watchable_map_var_t<int, int> w1, w2, w3;
    w1.set_key(101, 1);
    directory_map_write_manager_t<int, int>
        wm1(&c1, 'D', &w1), wm2(&c2, 'D', &w2), wm3(&c3, 'D', &w3);
    test_cluster_run_t cr1(&c1);
    test_cluster_run_t cr2(&c2);
    test_cluster_run_t cr3(&c3);
    cr2.join(get_cluster_local_address(&c1), 0);
    cr3.join(get_cluster_local_address(&c1), 0);
    let_stuff_happen();
    ASSERT_TRUE(boost::optional<int>(1) ==
        rm2.get_root_view()->get_key(std::make_pair(c1.get_me(), 101)));
    ASSERT_TRUE(boost::optional<int>() ==
        rm2.get_root_view()->get_key(std::make_pair(c1.get_me(), 102)));
    w1.set_key(101, 2);
    w1.set_key(102, 1);
    let_stuff_happen();
    ASSERT_TRUE(boost::optional<int>(2) ==
        rm2.get_root_view()->get_key(std::make_pair(c1.get_me(), 101)));
    ASSERT_TRUE(boost::optional<int>(1) ==
        rm2.get_root_view()->get_key(std::make_pair(c1.get_me(), 102)));
    w1.delete_key(101);
    let_stuff_happen();
    ASSERT_TRUE(boost::optional<int>() ==
        rm2.get_root_view()->get_key(std::make_pair(c1.get_me(), 101)));
    ASSERT_TRUE(boost::optional<int>(1) ==
        rm2.get_root_view()->get_key(std::make_pair(c1.get_me(), 102)));
}

/* `DestructorRace` tests a nasty race condition that we had at some point. */
TPTEST(RPCDirectoryTest, DestructorRace) {
    connectivity_cluster_t c;
    directory_read_manager_t<int> rm(&c, 'D');
    watchable_variable_t<int> w(5);
    directory_write_manager_t<int> wm(&c, 'D', w.get_watchable());
    test_cluster_run_t cr(&c);

    w.set_value(6);
}

}   /* namespace unittest */
