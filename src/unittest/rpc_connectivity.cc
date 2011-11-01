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
    thread_pool_t thread_pool(1, false);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

/* `recording_connectivity_cluster_t` is a `connectivity_cluster_t` that keeps
track of messages it receives from other nodes in the cluster. */

struct recording_connectivity_cluster_t : public connectivity_cluster_t {
private:
    std::map<int, peer_id_t> inbox;
    std::map<int, int> timing;
    int sequence_number;
    void on_message(peer_id_t peer, std::istream &stream, boost::function<void()> &on_done) {
        int i;
        stream >> i;
        on_done();
        inbox[i] = peer;
        timing[i] = sequence_number++;
    }
    static void write(int i, std::ostream &stream) {
        stream << i;
    }
public:
    explicit recording_connectivity_cluster_t(int i) : connectivity_cluster_t(i), sequence_number(0) { }
    void send(int message, peer_id_t peer) {
        send_message(peer, boost::bind(&write, message, _1));
    }
    void expect(int message, peer_id_t peer) {
        expect_delivered(message);
        EXPECT_TRUE(inbox[message] == peer);
    }
    void expect_delivered(int message) {
        EXPECT_TRUE(inbox.find(message) != inbox.end());
    }
    void expect_undelivered(int message) {
        EXPECT_TRUE(inbox.find(message) == inbox.end());
    }
    void expect_order(int first, int second) {
        expect_delivered(first);
        expect_delivered(second);
        EXPECT_LT(timing[first], timing[second]);
    }
};

/* `StartStop` starts a cluster of three nodes, then shuts it down again. */

void run_start_stop_test() {
    int port = 10000 + rand() % 20000;
    recording_connectivity_cluster_t c1(port), c2(port+1), c3(port+2);
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
    recording_connectivity_cluster_t c1(port), c2(port+1), c3(port+2);
    c2.join(peer_address_t(ip_address_t::us(), port));
    c3.join(peer_address_t(ip_address_t::us(), port));

    let_stuff_happen();

    c1.send(873, c2.get_me());
    c2.send(66663, c1.get_me());
    c3.send(6849, c1.get_me());
    c3.send(999, c3.get_me());

    let_stuff_happen();

    c2.expect(873, c1.get_me());
    c1.expect(66663, c2.get_me());
    c1.expect(6849, c3.get_me());
    c3.expect(999, c3.get_me());
}
TEST(RPCConnectivityTest, Message) {
    run_in_thread_pool(&run_message_test);
}

/* `UnreachablePeer` tests that messages sent to unreachable peers silently
fail. */

void run_unreachable_peer_test() {
    int port = 10000 + rand() % 20000;
    recording_connectivity_cluster_t c1(port), c2(port+1);
    /* Note that we DON'T join them together. */

    let_stuff_happen();

    c1.send(888, c2.get_me());

    let_stuff_happen();

    /* The message should not have been delivered. The system shouldn't have
    crashed, either. */
    c2.expect_undelivered(888);

    c1.join(peer_address_t(ip_address_t::us(), port+1));

    let_stuff_happen();

    c1.send(999, c2.get_me());

    let_stuff_happen();

    c2.expect_undelivered(888);
    c2.expect(999, c1.get_me());
}
TEST(RPCConnectivityTest, UnreachablePeer) {
    run_in_thread_pool(&run_unreachable_peer_test);
}

/* `Ordering` tests that messages sent by the same route arrive in the same
order they were sent in. */

void run_ordering_test() {
    int port = 10000 + rand() % 20000;
    recording_connectivity_cluster_t c1(port), c2(port+1);
    c1.join(peer_address_t(ip_address_t::us(), port+1));

    let_stuff_happen();

    for (int i = 0; i < 10; i++) {
        c1.send(i, c2.get_me());
        c1.send(i, c1.get_me());
    }

    let_stuff_happen();

    for (int i = 0; i < 9; i++) {
        c1.expect_order(i, i+1);
        c2.expect_order(i, i+1);
    }
}
TEST(RPCConnectivityTest, Ordering) {
    run_in_thread_pool(&run_ordering_test);
}

/* `GetEverybody` confirms that the behavior of `cluster_t::get_everybody()` is
correct. */

void run_get_everybody_test() {
    int port = 10000 + rand() % 20000;
    recording_connectivity_cluster_t c1(port);

    /* Make sure `get_everybody()` is initially sane */
    std::map<peer_id_t, peer_address_t> routing_table_1;
    routing_table_1 = c1.get_everybody();
    EXPECT_TRUE(routing_table_1.find(c1.get_me()) != routing_table_1.end());
    EXPECT_EQ(routing_table_1.size(), 1);

    {
        recording_connectivity_cluster_t c2(port+1);
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
    recording_connectivity_cluster_t c1(port);

    boost::scoped_ptr<recording_connectivity_cluster_t> c2(new recording_connectivity_cluster_t(port+1));
    peer_id_t c2_id = c2->get_me();

    /* Make sure `c1` notifies us when `c2` connects */
    connect_watcher_t connect_watcher(&c1, c2_id);
    EXPECT_FALSE(connect_watcher.is_pulsed());
    c1.join(peer_address_t(ip_address_t::us(), port+1));
    let_stuff_happen();
    EXPECT_TRUE(connect_watcher.is_pulsed());

    /* Make sure `connect_watcher_t` works for an already-connected peer */
    connect_watcher_t connect_watcher_2(&c1, c2_id);
    EXPECT_TRUE(connect_watcher_2.is_pulsed());

    /* Make sure `c1` notifies us when `c2` disconnects */
    disconnect_watcher_t disconnect_watcher(&c1, c2_id);
    EXPECT_FALSE(disconnect_watcher.is_pulsed());
    c2.reset();
    let_stuff_happen();
    EXPECT_TRUE(disconnect_watcher.is_pulsed());

    /* Make sure `disconnect_watcher_t` works for an already-unconnected peer */
    disconnect_watcher_t disconnect_watcher_2(&c1, c2_id);
    EXPECT_TRUE(disconnect_watcher_2.is_pulsed());
}
TEST(RPCConnectivityTest, EventWatchers) {
    run_in_thread_pool(&run_event_watchers_test);
}

/* `EventWatcherOrdering` confirms that information delivered via event
notification is consistent with information delivered via `get_everybody()`. */

void run_event_watcher_ordering_test() {

    int port = 10000 + rand() % 20000;
    recording_connectivity_cluster_t c1(port);

    struct watcher_t : public event_watcher_t {

        explicit watcher_t(recording_connectivity_cluster_t *c) :
            event_watcher_t(c), cluster(c) { }
        recording_connectivity_cluster_t *cluster;

        void on_connect(peer_id_t p) {
            /* When we get a connection event, make sure that the peer address
            is present in the routing table */
            std::map<peer_id_t, peer_address_t> routing_table;
            routing_table = cluster->get_everybody();
            EXPECT_TRUE(routing_table.find(p) != routing_table.end());

            /* Make sure messages sent from connection events are delivered
            properly. We must use `coro_t::spawn_now()` because `send_message()`
            may block. */
            coro_t::spawn_now(boost::bind(&recording_connectivity_cluster_t::send, cluster, 89765, p));
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
        recording_connectivity_cluster_t c2(port+1);
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
    boost::scoped_ptr<recording_connectivity_cluster_t> nodes[num_members];
    for (int i = 0; i < num_members; i++) {
        nodes[i].reset(new recording_connectivity_cluster_t(port+i));
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
    boost::scoped_ptr<recording_connectivity_cluster_t> nodes[blob_size * 2];
    for (int i = 0; i < blob_size * 2; i++) {
        nodes[i].reset(new recording_connectivity_cluster_t(port+i));
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

/* `BinaryData` makes sure that any octet can be sent over the wire. */

void run_binary_data_test() {

    struct binary_cluster_t : public connectivity_cluster_t {
        explicit binary_cluster_t(int port) : connectivity_cluster_t(port), got_spectrum(false) { }
        bool got_spectrum;
        static void dump_spectrum(std::ostream &stream) {
            char spectrum[CHAR_MAX - CHAR_MIN + 1];
            for (int i = CHAR_MIN; i <= CHAR_MAX; i++) spectrum[i - CHAR_MIN] = i;
            stream.write(spectrum, CHAR_MAX - CHAR_MIN + 1);
        }
        void send_spectrum(peer_id_t peer) {
            send_message(peer, &dump_spectrum);
        }
        void on_message(peer_id_t, std::istream &stream, boost::function<void()> &on_done) {
            char spectrum[CHAR_MAX - CHAR_MIN + 1];
            stream.read(spectrum, CHAR_MAX - CHAR_MIN + 1);
            int eof = stream.peek();
            on_done();
            for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
                EXPECT_EQ(spectrum[i - CHAR_MIN], i);
            }
            EXPECT_EQ(eof, EOF);
            got_spectrum = true;
        }
    };

    int port = 10000 + rand() % 20000;
    binary_cluster_t cluster1(port), cluster2(port+1);
    cluster1.join(cluster2.get_everybody()[cluster2.get_me()]);

    let_stuff_happen();

    cluster1.send_spectrum(cluster2.get_me());

    let_stuff_happen();

    EXPECT_TRUE(cluster2.got_spectrum);
}
TEST(RPCConnectivityTest, BinaryData) {
    run_in_thread_pool(&run_binary_data_test);
}

/* `PeerIDSemantics` makes sure that `peer_id_t::is_nil()` works as expected. */

void run_peer_id_semantics_test() {

    peer_id_t nil_peer;
    ASSERT_TRUE(nil_peer.is_nil());

    int port = 10000 + rand() % 20000;
    recording_connectivity_cluster_t cluster_node(port);
    ASSERT_FALSE(cluster_node.get_me().is_nil());
}
TEST(RPCConnectivityTest, PeerIDSemantics) {
    run_in_thread_pool(&run_peer_id_semantics_test);
}

}   /* namespace unittest */
