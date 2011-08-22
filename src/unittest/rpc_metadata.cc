#include "unittest/gtest.hpp"

#include "rpc/metadata/metadata.hpp"
#include "rpc/metadata/view/field.hpp"
#include "rpc/metadata/view/controller.hpp"

/* These are defined out here because the metadata code expects to find them in
the global namespace. */

void semilattice_join(uint64_t *a, uint64_t b) {
    *a |= b;
}

class semilattice_test_t {
public:
    semilattice_test_t(uint64_t x_, uint64_t y_) : x(x_), y(y_) { }
    uint64_t x, y;
};

void semilattice_join(semilattice_test_t *a, semilattice_test_t b) {
    semilattice_join(&a->x, b.x);
    semilattice_join(&a->y, b.y);
}

namespace unittest {

namespace {

/* `run_in_thread_pool()` starts a RethinkDB IO layer thread pool and calls the
given function within a coroutine inside of it. */

class starter_t : public thread_message_t {
public:
    thread_pool_t *tp;
    boost::function<void()> fun;
    starter_t(thread_pool_t *tp, boost::function<void()> fun) : tp(tp), fun(fun) { }
    void run() {
        fun();
        tp->shutdown();
    }
    void on_thread_switch() {
        coro_t::spawn_now(boost::bind(&starter_t::run, this));
    }
};
void run_in_thread_pool(boost::function<void()> fun, int nthreads = 1) {
    thread_pool_t thread_pool(nthreads);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

template<class T>
void assign(T *target, T value) {
    *target = value;
}

}   /* anonymous namespace */

/* `SingleMetadata` tests metadata's properties on a single node. */

void run_single_metadata_test() {
    int port = 10000 + rand() % 20000;
    metadata_cluster_t<uint64_t> cluster(port, 2);

    /* Make sure that metadata works properly when passed to the constructor */
    EXPECT_EQ(cluster.get_root_view()->get(), 2);

    /* Make sure `join()` works properly */
    cluster.get_root_view()->join(1);

    EXPECT_EQ(cluster.get_root_view()->get(), 3);
}
TEST(RPCMetadataTest, SingleMetadata) {
    run_in_thread_pool(&run_single_metadata_test, 2);
}

/* `MetadataExchange` makes sure that metadata is correctly exchanged between
nodes. */

void run_metadata_exchange_test() {
    int port = 10000 + rand() % 20000;
    metadata_cluster_t<uint64_t> cluster1(port, 1), cluster2(port+1, 2);

    EXPECT_EQ(cluster1.get_root_view()->get(), 1);
    EXPECT_EQ(cluster2.get_root_view()->get(), 2);

    cluster1.join(cluster2.get_everybody()[cluster2.get_me()]);
    let_stuff_happen();

    EXPECT_EQ(cluster1.get_root_view()->get(), 3);
    EXPECT_EQ(cluster2.get_root_view()->get(), 3);

    cluster1.get_root_view()->join(4);
    let_stuff_happen();

    EXPECT_EQ(cluster1.get_root_view()->get(), 7);
    EXPECT_EQ(cluster2.get_root_view()->get(), 7);
}
TEST(RPCMetadataTest, MetadataExchange) {
    run_in_thread_pool(&run_metadata_exchange_test, 2);
}

/* `Watcher` makes sure that metadata watchers get notified when metadata
changes. */

void run_watcher_test() {
    int port = 10000 + rand() % 20000;
    metadata_cluster_t<uint64_t> cluster(port, 2);

    bool have_been_notified = false;
    metadata_read_view_t<uint64_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        cluster.get_root_view());

    cluster.get_root_view()->join(1);

    EXPECT_TRUE(have_been_notified);
}
TEST(RPCMetadataTest, Watcher) {
    run_in_thread_pool(&run_watcher_test, 2);
}

/* `ViewController` tests `metadata_view_controller_t`. */

void run_view_controller_test() {

    metadata_view_controller_t<uint64_t> controller(16);
    EXPECT_EQ(controller.get_view()->get(), 16);

    controller.get_view()->join(2);
    EXPECT_EQ(controller.get_view()->get(), 18);

    bool have_been_notified = false;
    metadata_read_view_t<uint64_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        controller.get_view());

    EXPECT_FALSE(have_been_notified);

    controller.get_view()->join(1);
    EXPECT_EQ(controller.get_view()->get(), 19);

    EXPECT_TRUE(have_been_notified);
}
TEST(RPCMetadataTest, ViewController) {
    run_in_thread_pool(&run_view_controller_test);
}

/* `FieldView` tests `metadata_field_read_view_t`. */

void run_field_view_test() {

    metadata_view_controller_t<semilattice_test_t> controller(
        semilattice_test_t(8, 4));
    metadata_field_read_view_t<semilattice_test_t, uint64_t> x_view(
        &semilattice_test_t::x, controller.get_view());

    EXPECT_EQ(x_view.get(), 8);

    bool have_been_notified = false;
    metadata_read_view_t<uint64_t>::subscription_t watcher(
        boost::bind(&assign<bool>, &have_been_notified, true),
        &x_view);

    EXPECT_FALSE(have_been_notified);
    controller.get_view()->join(semilattice_test_t(1, 0));
    EXPECT_TRUE(have_been_notified);

    EXPECT_EQ(x_view.get(), 9);
}
TEST(RPCMetadataTest, FieldView) {
    run_in_thread_pool(&run_field_view_test);
}

}   /* namespace unittest */
