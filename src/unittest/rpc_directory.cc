// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/timing.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* `OneNode` starts a single directory node, then shuts it down again. */

void run_one_node_test() {
    connectivity_cluster_t c;
    directory_read_manager_t<int> read_manager(&c);
    watchable_variable_t<int> watchable(5);
    directory_write_manager_t<int> write_manager(&c, watchable.get_watchable());
    connectivity_cluster_t::run_t cr(&c, get_unittest_addresses(), peer_address_t(), ANY_PORT, &read_manager, 0, NULL);
    let_stuff_happen();
}
TEST(RPCDirectoryTest, OneNode) {
    unittest::run_in_thread_pool(&run_one_node_test, 1);
}

/* `ThreeNodes` starts three directory nodes, lets them contact each other, and
then shuts them down again. */

void run_three_nodes_test() {
    connectivity_cluster_t c1, c2, c3;
    directory_read_manager_t<int> rm1(&c1), rm2(&c2), rm3(&c3);
    watchable_variable_t<int> w1(101), w2(202), w3(303);
    directory_write_manager_t<int> wm1(&c1, w1.get_watchable()), wm2(&c2, w2.get_watchable()), wm3(&c3, w3.get_watchable());
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm2, 0, NULL);
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm3, 0, NULL);
    cr2.join(c1.get_peer_address(c1.get_me()));
    cr3.join(c1.get_peer_address(c1.get_me()));
    let_stuff_happen();
}
TEST(RPCDirectoryTest, ThreeNodes) {
    unittest::run_in_thread_pool(&run_three_nodes_test, 1);
}

/* `Exchange` tests that directory nodes receive values from their peers. */

void run_exchange_test() {
    connectivity_cluster_t c1, c2, c3;
    directory_read_manager_t<int> rm1(&c1), rm2(&c2), rm3(&c3);
    watchable_variable_t<int> w1(101), w2(202), w3(303);
    directory_write_manager_t<int> wm1(&c1, w1.get_watchable()), wm2(&c2, w2.get_watchable()), wm3(&c3, w3.get_watchable());
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm2, 0, NULL);
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm3, 0, NULL);
    cr2.join(c1.get_peer_address(c1.get_me()));
    cr3.join(c1.get_peer_address(c1.get_me()));
    let_stuff_happen();
    EXPECT_EQ(1u, rm1.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(101, rm1.get_root_view()->get().get_inner().find(c1.get_me())->second);
    EXPECT_EQ(1u, rm1.get_root_view()->get().get_inner().count(c2.get_me()));
    EXPECT_EQ(202, rm1.get_root_view()->get().get_inner().find(c2.get_me())->second);
    EXPECT_EQ(1u, rm3.get_root_view()->get().get_inner().count(c1.get_me()));
    EXPECT_EQ(101, rm3.get_root_view()->get().get_inner().find(c1.get_me())->second);
}
TEST(RPCDirectoryTest, Exchange) {
    unittest::run_in_thread_pool(&run_exchange_test, 1);
}

/* `Update` tests that directory nodes see updates from their peers. */

void run_update_test() {
    connectivity_cluster_t c1, c2, c3;
    directory_read_manager_t<int> rm1(&c1), rm2(&c2), rm3(&c3);
    watchable_variable_t<int> w1(101), w2(202), w3(303);
    directory_write_manager_t<int> wm1(&c1, w1.get_watchable()), wm2(&c2, w2.get_watchable()), wm3(&c3, w3.get_watchable());
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm2, 0, NULL);
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm3, 0, NULL);
    cr2.join(c1.get_peer_address(c1.get_me()));
    cr3.join(c1.get_peer_address(c1.get_me()));
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
TEST(RPCDirectoryTest, Update) {
    unittest::run_in_thread_pool(&run_update_test, 1);
}

/* `DestructorRace` tests a nasty race condition that we had at some point. */

void run_destructor_race_test() {
    connectivity_cluster_t c;
    directory_read_manager_t<int> rm(&c);
    watchable_variable_t<int> w(5);
    directory_write_manager_t<int> wm(&c, w.get_watchable());
    connectivity_cluster_t::run_t cr(&c, get_unittest_addresses(), peer_address_t(), ANY_PORT, &rm, 0, NULL);

    w.set_value(6);
}

TEST(RPCDirectoryTest, DestructorRace) {
    unittest::run_in_thread_pool(&run_destructor_race_test, 1);
}

}   /* namespace unittest */
