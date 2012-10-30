// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "containers/archive/archive.hpp"
#include "mock/unittest_utils.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

namespace {



class sl_pair_t {
public:
    sl_pair_t(sl_int_t x_, sl_int_t y_) : x(x_), y(y_) { }
    sl_int_t x, y;

    // FIXME: serialization functions are never referenced and build with Intel fails
    // RDB_MAKE_ME_SERIALIZABLE_2(x, y);
};

void semilattice_join(sl_pair_t *a, sl_pair_t b) {
    semilattice_join(&a->x, b.x);
    semilattice_join(&a->y, b.y);
}

template<class T>
void assign(T *target, T value) {
    *target = value;
}

}   /* anonymous namespace */

/* `SingleMetadata` tests metadata's properties on a single node. */

void run_single_metadata_test() {
    int port = mock::randport();
    connectivity_cluster_t c;
    semilattice_manager_t<sl_int_t> slm(&c, sl_int_t(2));
    connectivity_cluster_t::run_t cr(&c, port, &slm);

    /* Make sure that metadata works properly when passed to the constructor */
    EXPECT_EQ(2, slm.get_root_view()->get().i);

    /* Make sure `join()` works properly */
    slm.get_root_view()->join(sl_int_t(1));

    EXPECT_EQ(3, slm.get_root_view()->get().i);
}
TEST(RPCSemilatticeTest, SingleMetadata) {
    mock::run_in_thread_pool(&run_single_metadata_test, 2);
}

/* `MetadataExchange` makes sure that metadata is correctly exchanged between
nodes. */

void run_metadata_exchange_test() {
    int port = mock::randport();
    connectivity_cluster_t cluster1, cluster2;
    semilattice_manager_t<sl_int_t> slm1(&cluster1, sl_int_t(1)), slm2(&cluster2, sl_int_t(2));
    connectivity_cluster_t::run_t run1(&cluster1, port, &slm1), run2(&cluster2, port+1, &slm2);

    EXPECT_EQ(1, slm1.get_root_view()->get().i);
    EXPECT_EQ(2, slm2.get_root_view()->get().i);

    run1.join(cluster2.get_peer_address(cluster2.get_me()));

    /* Block until the connection is established */
    {
        struct : public cond_t, public peers_list_callback_t {
            void on_connect(UNUSED peer_id_t peer) {
                pulse();
            }
            void on_disconnect(UNUSED peer_id_t peer) { }
        } connection_established;
        connectivity_service_t::peers_list_subscription_t subs(&connection_established);

        {
            ASSERT_FINITE_CORO_WAITING;
            connectivity_service_t::peers_list_freeze_t freeze(&cluster1);
            if (!cluster1.get_peer_connected(cluster2.get_me())) {
                subs.reset(&cluster1, &freeze);
            } else {
                connection_established.pulse();
            }
        }

        connection_established.wait_lazily_unordered();
    }

    cond_t interruptor1;
    slm1.get_root_view()->sync_to(cluster2.get_me(), &interruptor1);

    EXPECT_EQ(3, slm1.get_root_view()->get().i);
    EXPECT_EQ(3, slm2.get_root_view()->get().i);

    slm1.get_root_view()->join(sl_int_t(4));

    cond_t interruptor2;
    slm1.get_root_view()->sync_to(cluster2.get_me(), &interruptor2);

    EXPECT_EQ(7, slm1.get_root_view()->get().i);
    EXPECT_EQ(7, slm2.get_root_view()->get().i);
}
TEST(RPCSemilatticeTest, MetadataExchange) {
    mock::run_in_thread_pool(&run_metadata_exchange_test, 2);
}

void run_sync_from_test() {
    int port = mock::randport();
    connectivity_cluster_t cluster1, cluster2;
    semilattice_manager_t<sl_int_t> slm1(&cluster1, sl_int_t(1)), slm2(&cluster2, sl_int_t(2));
    connectivity_cluster_t::run_t run1(&cluster1, port, &slm1), run2(&cluster2, port+1, &slm2);

    EXPECT_EQ(1, slm1.get_root_view()->get().i);
    EXPECT_EQ(2, slm2.get_root_view()->get().i);

    cond_t non_interruptor;

    EXPECT_THROW(slm1.get_root_view()->sync_from(cluster2.get_me(), &non_interruptor), sync_failed_exc_t);
    EXPECT_THROW(slm1.get_root_view()->sync_to(cluster2.get_me(), &non_interruptor), sync_failed_exc_t);

    run1.join(cluster2.get_peer_address(cluster2.get_me()));

    /* Block until the connection is established */
    {
        struct : public cond_t, public peers_list_callback_t {
            void on_connect(UNUSED peer_id_t peer) {
                pulse();
            }
            void on_disconnect(UNUSED peer_id_t peer) { }
        } connection_established;

        connectivity_service_t::peers_list_subscription_t subs(&connection_established);

        {
            ASSERT_FINITE_CORO_WAITING;
            connectivity_service_t::peers_list_freeze_t freeze(&cluster1);
            if (!cluster1.get_peer_connected(cluster2.get_me())) {
                subs.reset(&cluster1, &freeze);
            } else {
                connection_established.pulse();
            }
        }

        connection_established.wait_lazily_unordered();
    }

    slm1.get_root_view()->sync_from(cluster2.get_me(), &non_interruptor);
    slm2.get_root_view()->sync_from(cluster1.get_me(), &non_interruptor);

    EXPECT_EQ(3, slm1.get_root_view()->get().i);
    EXPECT_EQ(3, slm2.get_root_view()->get().i);
}
TEST(RPCSemilatticeTest, SyncFrom) {
    mock::run_in_thread_pool(&run_sync_from_test, 2);
}

/* `Watcher` makes sure that metadata watchers get notified when metadata
changes. */

void run_watcher_test() {
    int port = mock::randport();
    connectivity_cluster_t cluster;
    semilattice_manager_t<sl_int_t> slm(&cluster, sl_int_t(2));
    connectivity_cluster_t::run_t run(&cluster, port, &slm);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        slm.get_root_view());

    slm.get_root_view()->join(sl_int_t(1));

    EXPECT_TRUE(have_been_notified);
}
TEST(RPCSemilatticeTest, Watcher) {
    mock::run_in_thread_pool(&run_watcher_test, 2);
}

/* `ViewController` tests `dummy_semilattice_controller_t`. */

void run_view_controller_test() {

    dummy_semilattice_controller_t<sl_int_t> controller(sl_int_t(16));
    EXPECT_EQ(16, controller.get_view()->get().i);

    controller.get_view()->join(sl_int_t(2));
    EXPECT_EQ(18, controller.get_view()->get().i);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        controller.get_view());

    EXPECT_FALSE(have_been_notified);

    controller.get_view()->join(sl_int_t(1));
    EXPECT_EQ(19, controller.get_view()->get().i);

    EXPECT_TRUE(have_been_notified);
}
TEST(RPCSemilatticeTest, ViewController) {
    mock::run_in_thread_pool(&run_view_controller_test);
}
TEST(RPCSemilatticeTest, ViewControllerMultiThread) {
    mock::run_in_thread_pool(&run_view_controller_test, 3);
}

/* `FieldView` tests `metadata_field()`. */

void run_field_view_test() {

    dummy_semilattice_controller_t<sl_pair_t> controller(
        sl_pair_t(sl_int_t(8), sl_int_t(4)));

    boost::shared_ptr<semilattice_read_view_t<sl_int_t> > x_view =
        metadata_field(&sl_pair_t::x, controller.get_view());

    EXPECT_EQ(8, x_view->get().i);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        x_view);

    EXPECT_FALSE(have_been_notified);
    controller.get_view()->join(sl_pair_t(sl_int_t(1), sl_int_t(0)));
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(9, x_view->get().i);
}
TEST(RPCSemilatticeTest, FieldView) {
    mock::run_in_thread_pool(&run_field_view_test);
}
TEST(RPCSemilatticeTest, FieldViewMultiThread) {
    mock::run_in_thread_pool(&run_field_view_test, 3);
}

/* `MemberView` tests `metadata_member()`. */

void run_member_view_test() {

    std::map<std::string, sl_int_t> initial_value;
    initial_value["foo"] = sl_int_t(8);
    dummy_semilattice_controller_t<std::map<std::string, sl_int_t> > controller(
        initial_value);

    boost::shared_ptr<semilattice_read_view_t<sl_int_t> > foo_view =
        metadata_member(std::string("foo"), controller.get_view());

    EXPECT_EQ(8, foo_view->get().i);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        foo_view);

    EXPECT_FALSE(have_been_notified);
    std::map<std::string, sl_int_t> new_value;
    new_value["foo"] = sl_int_t(1);
    controller.get_view()->join(new_value);
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(9, foo_view->get().i);
}
TEST(RPCSemilatticeTest, MemberView) {
    mock::run_in_thread_pool(&run_member_view_test);
}
TEST(RPCSemilatticeTest, MemberViewMultiThread) {
    mock::run_in_thread_pool(&run_member_view_test, 3);
}

}   /* namespace unittest */
