#include "unittest/gtest.hpp"

#include "rpc/metadata/metadata.hpp"

/* For these tests, we use `uint64_t` as our semilattice. Semilattice-join is
defined as the bitwise `|` of the operands. It's defined out here because
`metadata_cluster_t` will look for it in the global namespace. */

void semilattice_join(uint64_t *a, uint64_t b) {
    *a |= b;
}

namespace unittest {

namespace {

/* `run_in_thread_pool()` starts a RethinkDB IO layer thread pool and calls the
given function within a coroutine inside of it. */

struct starter_t : public thread_message_t {
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

}   /* anonymous namespace */

/* `SingleMetadata` tests metadata's properties on a single node. */

void run_single_metadata_test() {
    int port = 10000 + rand() % 20000;
    metadata_cluster_t<uint64_t> cluster(port, 2);

    /* Make sure that metadata works properly when passed to the constructor */
    EXPECT_EQ(cluster.get_metadata(), 2);

    /* Make sure `join_metadata()` works properly */
    cluster.join_metadata(1);

    EXPECT_EQ(cluster.get_metadata(), 3);

    /* Make sure other threads work properly */
    on_thread_t thread_switcher(1);
    EXPECT_EQ(cluster.get_metadata(), 3);
}
TEST(RPCMetadataTest, SingleMetadata) {
    run_in_thread_pool(&run_single_metadata_test, 2);
}

/* `MetadataExchange` makes sure that metadata is correctly exchanged between
nodes. */

void run_metadata_exchange_test() {
    int port = 10000 + rand() % 20000;
    metadata_cluster_t<uint64_t> cluster1(port, 1), cluster2(port+1, 2);

    EXPECT_EQ(cluster1.get_metadata(), 1);
    EXPECT_EQ(cluster2.get_metadata(), 2);

    cluster1.join(cluster2.get_everybody()[cluster2.get_me()]);
    let_stuff_happen();

    EXPECT_EQ(cluster1.get_metadata(), 3);
    EXPECT_EQ(cluster2.get_metadata(), 3);

    cluster1.join_metadata(4);
    let_stuff_happen();

    EXPECT_EQ(cluster1.get_metadata(), 7);
    EXPECT_EQ(cluster2.get_metadata(), 7);
}
TEST(RPCMetadataTest, MetadataExchange) {
    run_in_thread_pool(&run_metadata_exchange_test, 2);
}

}   /* namespace unittest */
