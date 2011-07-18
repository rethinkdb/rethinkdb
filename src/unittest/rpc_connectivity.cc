#include "unittest/gtest.hpp"

#include "rpc/connectivity/connectivity.hpp"

namespace unittest {

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
void run_in_thread_pool(boost::function<void()> fun) {
    thread_pool_t thread_pool(1);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(50);
}

/* `dummy_cluster_t` is a `connectivity_cluster_t` that keeps track of messages
it receives from other nodes in the cluster. */

struct dummy_cluster_t : public connectivity_cluster_t {
private:
    std::map<int, peer_id_t> inbox;
    void on_message(peer_id_t peer, std::istream &stream) {
        int i;
        stream >> i;
        inbox[i] = peer;
    }
    static void write(int i, std::ostream &stream) {
        stream << i;
    }
public:
    dummy_cluster_t(int i) : connectivity_cluster_t(i) { }
    void send(int message, peer_id_t peer) {
        send_message(peer, boost::bind(&write, message, _1));
    }
    void expect(int message, peer_id_t peer) {
        EXPECT_TRUE(inbox.find(message) != inbox.end());
        EXPECT_TRUE(inbox[message] == peer);
    }
};

/* `StartStop` starts a cluster of three nodes, then shuts it down again. */

void run_start_stop_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t c1(port), c2(port+1), c3(port+2);
    c2.join(peer_address_t(ip_address_t::us(), port));
    c3.join(peer_address_t(ip_address_t::us(), port));
    let_stuff_happen();
}
TEST(RPCConnectivityTest, StartStop) {
    run_in_thread_pool(&run_start_stop_test);
}

/* `Message` sends some simple messages between the nodes of a cluster. */

void run_message_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t c1(port), c2(port+1), c3(port+2);
    c2.join(peer_address_t(ip_address_t::us(), port));
    c3.join(peer_address_t(ip_address_t::us(), port));

    let_stuff_happen();

    c1.send(873, c2.get_me());
    c2.send(66663, c1.get_me());
    c3.send(6849, c1.get_me());

    let_stuff_happen();

    c2.expect(873, c1.get_me());
    c1.expect(66663, c2.get_me());
    c1.expect(6849, c3.get_me());
}
TEST(RPCConnectivityTest, Message) {
    run_in_thread_pool(&run_message_test);
}

/* `GetEverybody` confirms that the behavior of `cluster_t::get_everybody()` is
correct. */

void run_get_everybody_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t c1(port);

    /* Make sure `get_everybody()` is initially sane */
    std::map<peer_id_t, peer_address_t> routing_table_1;
    routing_table_1 = c1.get_everybody();
    EXPECT_TRUE(routing_table_1.find(c1.get_me()) != routing_table_1.end());
    EXPECT_EQ(routing_table_1.size(), 1);

    {
        dummy_cluster_t c2(port+1);
        c2.join(peer_address_t(ip_address_t::us(), port));

        let_stuff_happen();

        /* Make sure `get_everybody()` correctly notices that a peer connects */
        std::map<peer_id_t, peer_address_t> routing_table_2;
        routing_table_2 = c1.get_everybody();
        EXPECT_TRUE(routing_table_2.find(c2.get_me()) != routing_table_2.end());
        EXPECT_EQ(routing_table_2[c2.get_me()].port, port+1);

        /* `c2`'s destructor is called here */
    }

    let_stuff_happen();

    /* Make sure `get_everybody()` notices that a peer has disconnected */
    std::map<peer_id_t, peer_address_t> routing_table_3;
    routing_table_3 = c1.get_everybody();
    EXPECT_EQ(routing_table_3.size(), 1);
}
TEST(RPCConnectivityTest, GetEverybody) {
    run_in_thread_pool(&run_get_everybody_test);
}

/* `EventWatchers` confirms that `cluster_t::[dis]connect_watcher_t` works
properly. */

void run_event_watchers_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t c1(port);

    boost::scoped_ptr<dummy_cluster_t> c2(new dummy_cluster_t(port+1));

    /* Make sure `c1` notifies us when `c2` connects */
    connect_watcher_t connect_watcher(&c1, c2->get_me());
    EXPECT_FALSE(connect_watcher.is_pulsed());
    c1.join(peer_address_t(ip_address_t::us(), port+1));
    let_stuff_happen();
    EXPECT_TRUE(connect_watcher.is_pulsed());

    /* Make sure `c1` notifies us when `c2` disconnects */
    disconnect_watcher_t disconnect_watcher(&c1, c2->get_me());
    EXPECT_FALSE(disconnect_watcher.is_pulsed());
    c2.reset();
    let_stuff_happen();
    EXPECT_TRUE(disconnect_watcher.is_pulsed());
}
TEST(RPCConnectivityTest, EventWatchers) {
    run_in_thread_pool(&run_event_watchers_test);
}

/* `EventWatcherOrdering` confirms that information delivered via event
notification is consistent with information delivered via `get_everybody()`. */

void run_event_watcher_ordering_test() {

    int port = 10000 + rand() % 20000;
    dummy_cluster_t c1(port);

    struct watcher_t : public event_watcher_t {

        watcher_t(dummy_cluster_t *c) :
            event_watcher_t(c), cluster(c) { }
        dummy_cluster_t *cluster;

        void on_connect(peer_id_t p) {
            /* When we get a connection event, make sure that the peer address
            is present in the routing table */
            std::map<peer_id_t, peer_address_t> routing_table;
            routing_table = cluster->get_everybody();
            EXPECT_TRUE(routing_table.find(p) != routing_table.end());

            /* Make sure messages sent from connection events are delivered
            properly. We must use `coro_t::spawn_now()` because `send_message()`
            may block. */
            coro_t::spawn_now(boost::bind(&dummy_cluster_t::send, cluster, 89765, p));
        }

        void on_disconnect(peer_id_t p) {
            /* When we get a disconnection event, make sure that the peer
            address is gone from the routing table */
            std::map<peer_id_t, peer_address_t> routing_table;
            routing_table = cluster->get_everybody();
            EXPECT_TRUE(routing_table.find(p) == routing_table.end());
        }
    } watcher(&c1);

    /* Generate some connection/disconnection activity */
    {
        dummy_cluster_t c2(port+1);
        c2.join(peer_address_t(ip_address_t::us(), port));

        let_stuff_happen();

        /* Make sure that the message sent in `on_connect()` was delivered */
        c2.expect(89765, c1.get_me());
    }

    let_stuff_happen();
}
TEST(RPCConnectivityTest, EventWatcherOrdering) {
    run_in_thread_pool(&run_event_watcher_ordering_test);
}

/* `StopMidJoin` makes sure that nothing breaks if you shut down the cluster
while it is still coming up */

void run_stop_mid_join_test() {

    int port = 10000 + rand() % 20000;

    const int num_members = 5;

    /* Spin up 20 cluster-members */
    boost::scoped_ptr<dummy_cluster_t> nodes[num_members];
    for (int i = 0; i < num_members; i++) {
        nodes[i].reset(new dummy_cluster_t(port+i));
    }
    for (int i = 1; i < num_members; i++) {
        nodes[i]->join(peer_address_t(ip_address_t::us(), port));
    }

    coro_t::yield();

    EXPECT_NE(nodes[1]->get_everybody().size(), num_members) << "This test is "
        "supposed to test what happens when a cluster is interrupted as it "
        "starts up, but the cluster finished starting up before we could "
        "interrupt it.";

    /* Shut down cluster nodes and hope nothing crashes. (The destructors do the
    work of shutting down.) */
}
TEST(RPCConnectivityTest, StopMidJoin) {
    run_in_thread_pool(&run_stop_mid_join_test);
}

/* `BlobJoin` tests whether two groups of cluster nodes can correctly merge
together. */

void run_blob_join_test() {

    int port = 10000 + rand() % 20000;

    /* Two blobs of `blob_size` nodes */
    const int blob_size = 4;

    /* Spin up cluster-members */
    boost::scoped_ptr<dummy_cluster_t> nodes[blob_size * 2];
    for (int i = 0; i < blob_size * 2; i++) {
        nodes[i].reset(new dummy_cluster_t(port+i));
    }

    for (int i = 1; i < blob_size; i++) {
        nodes[i]->join(peer_address_t(ip_address_t::us(), port));
    }
    for (int i = blob_size+1; i < blob_size*2; i++) {
        nodes[i]->join(peer_address_t(ip_address_t::us(), port+blob_size));
    }

    let_stuff_happen();

    nodes[1]->join(peer_address_t(ip_address_t::us(), port+blob_size+1));

    let_stuff_happen();

    /* Make sure every node sees every other */
    for (int i = 0; i < blob_size*2; i++) {
        ASSERT_EQ(nodes[i]->get_everybody().size(), blob_size*2);
    }
}
TEST(RPCConnectivityTest, BlobJoin) {
    run_in_thread_pool(&run_blob_join_test);
}

}   /* namespace unittest */
