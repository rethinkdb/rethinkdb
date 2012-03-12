#include "unittest/gtest.hpp"

#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

class sl_int_t {
public:
    sl_int_t() { }
    explicit sl_int_t(uint64_t initial) : i(initial) { }
    uint64_t i;
};

void semilattice_join(sl_int_t *a, sl_int_t b) {
    a->i |= b.i;
}

template<class archive_t>
void serialize(archive_t &a, sl_int_t &i, UNUSED unsigned int version) {
    a & i.i;
}

class sl_pair_t {
public:
    sl_pair_t(sl_int_t x_, sl_int_t y_) : x(x_), y(y_) { }
    sl_int_t x, y;
};

void semilattice_join(sl_pair_t *a, sl_pair_t b) {
    semilattice_join(&a->x, b.x);
    semilattice_join(&a->y, b.y);
}

template<class archive_t>
void serialize(archive_t &a, sl_pair_t &p, UNUSED unsigned int version) {
    a & p.x;
    a & p.y;
}

template<class T>
void assign(T *target, T value) {
    *target = value;
}

}   /* anonymous namespace */

/* `SingleMetadata` tests metadata's properties on a single node. */

void run_single_metadata_test() {
    int port = 10000 + randint(20000);
    connectivity_cluster_t c;
    semilattice_manager_t<sl_int_t> slm(&c, sl_int_t(2));
    connectivity_cluster_t::run_t cr(&c, port, &slm);

    /* Make sure that metadata works properly when passed to the constructor */
    EXPECT_EQ(slm.get_root_view()->get().i, 2);

    /* Make sure `join()` works properly */
    slm.get_root_view()->join(sl_int_t(1));

    EXPECT_EQ(slm.get_root_view()->get().i, 3);
}
TEST(RPCSemilatticeTest, SingleMetadata) {
    run_in_thread_pool(&run_single_metadata_test, 2);
}

/* `MetadataExchange` makes sure that metadata is correctly exchanged between
nodes. */

void run_metadata_exchange_test() {
    int port = 10000 + randint(20000);
    connectivity_cluster_t cluster1, cluster2;
    semilattice_manager_t<sl_int_t> slm1(&cluster1, sl_int_t(1)), slm2(&cluster2, sl_int_t(2));
    connectivity_cluster_t::run_t run1(&cluster1, port, &slm1), run2(&cluster2, port+1, &slm2);

    EXPECT_EQ(slm1.get_root_view()->get().i, 1);
    EXPECT_EQ(slm2.get_root_view()->get().i, 2);

    run1.join(cluster2.get_peer_address(cluster2.get_me()));

    /* Block until the connection is established */
    {
        cond_t connection_established;
        connectivity_service_t::peers_list_subscription_t subs(
            boost::bind(&cond_t::pulse, &connection_established),
            NULL);
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

    EXPECT_EQ(slm1.get_root_view()->get().i, 3);
    EXPECT_EQ(slm2.get_root_view()->get().i, 3);

    slm1.get_root_view()->join(sl_int_t(4));

    cond_t interruptor2;
    slm1.get_root_view()->sync_to(cluster2.get_me(), &interruptor2);

    EXPECT_EQ(slm1.get_root_view()->get().i, 7);
    EXPECT_EQ(slm2.get_root_view()->get().i, 7);
}
TEST(RPCSemilatticeTest, MetadataExchange) {
    run_in_thread_pool(&run_metadata_exchange_test, 2);
}

/* `Watcher` makes sure that metadata watchers get notified when metadata
changes. */

void run_watcher_test() {
    int port = 10000 + randint(20000);
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
    run_in_thread_pool(&run_watcher_test, 2);
}

/* `ViewController` tests `dummy_semilattice_controller_t`. */

void run_view_controller_test() {

    dummy_semilattice_controller_t<sl_int_t> controller(sl_int_t(16));
    EXPECT_EQ(controller.get_view()->get().i, 16);

    controller.get_view()->join(sl_int_t(2));
    EXPECT_EQ(controller.get_view()->get().i, 18);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        controller.get_view());

    EXPECT_FALSE(have_been_notified);

    controller.get_view()->join(sl_int_t(1));
    EXPECT_EQ(controller.get_view()->get().i, 19);

    EXPECT_TRUE(have_been_notified);
}
TEST(RPCSemilatticeTest, ViewController) {
    run_in_thread_pool(&run_view_controller_test);
}
TEST(RPCSemilatticeTest, ViewControllerMultiThread) {
    run_in_thread_pool(&run_view_controller_test, 3);
}

/* `FieldView` tests `metadata_field()`. */

void run_field_view_test() {

    dummy_semilattice_controller_t<sl_pair_t> controller(
        sl_pair_t(sl_int_t(8), sl_int_t(4)));

    boost::shared_ptr<semilattice_read_view_t<sl_int_t> > x_view =
        metadata_field(&sl_pair_t::x, controller.get_view());

    EXPECT_EQ(x_view->get().i, 8);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        x_view);

    EXPECT_FALSE(have_been_notified);
    controller.get_view()->join(sl_pair_t(sl_int_t(1), sl_int_t(0)));
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(x_view->get().i, 9);
}
TEST(RPCSemilatticeTest, FieldView) {
    run_in_thread_pool(&run_field_view_test);
}
TEST(RPCSemilatticeTest, FieldViewMultiThread) {
    run_in_thread_pool(&run_field_view_test, 3);
}

/* `MemberView` tests `metadata_member()`. */

void run_member_view_test() {

    std::map<std::string, sl_int_t> initial_value;
    initial_value["foo"] = sl_int_t(8);
    dummy_semilattice_controller_t<std::map<std::string, sl_int_t> > controller(
        initial_value);

    boost::shared_ptr<semilattice_read_view_t<sl_int_t> > foo_view =
        metadata_member(std::string("foo"), controller.get_view());

    EXPECT_EQ(foo_view->get().i, 8);

    bool have_been_notified = false;
    semilattice_read_view_t<sl_int_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        foo_view);

    EXPECT_FALSE(have_been_notified);
    std::map<std::string, sl_int_t> new_value;
    new_value["foo"] = sl_int_t(1);
    controller.get_view()->join(new_value);
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(foo_view->get().i, 9);
}
TEST(RPCSemilatticeTest, MemberView) {
    run_in_thread_pool(&run_member_view_test);
}
TEST(RPCSemilatticeTest, MemberViewMultiThread) {
    run_in_thread_pool(&run_member_view_test, 3);
}

}   /* namespace unittest */
