// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>

#include "arch/runtime/thread_pool.hpp"
#include "arch/timing.hpp"
#include "containers/scoped.hpp"
#include "containers/archive/socket_stream.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

/* `recording_test_application_t` sends and receives integers over a
`message_service_t`. It keeps track of the integers it has received.
*/

class recording_test_application_t : public home_thread_mixin_t, public message_handler_t {
public:
    explicit recording_test_application_t(message_service_t *s) :
        service(s),
        sequence_number(0)
        { }
    void send(int message, peer_id_t peer) {
        class writer_t : public send_message_write_callback_t {
        public:
            explicit writer_t(int _data) : data(_data) { }
            virtual ~writer_t() { }
            void write(write_stream_t *stream) {
                write_message_t wm;
                serialize(&wm, data);
                int res = send_write_message(stream, &wm);
                if (res) { throw fake_archive_exc_t(); }
            }
            int32_t data;
        } writer(message);
        service->send_message(peer, &writer);
    }
    void expect(int message, peer_id_t peer) {
        expect_delivered(message);
        assert_thread();
        EXPECT_TRUE(inbox[message] == peer);
    }
    void expect_delivered(int message) {
        assert_thread();
        EXPECT_TRUE(inbox.find(message) != inbox.end());
    }
    void expect_undelivered(int message) {
        assert_thread();
        EXPECT_TRUE(inbox.find(message) == inbox.end());
    }
    void expect_order(int first, int second) {
        expect_delivered(first);
        expect_delivered(second);
        assert_thread();
        EXPECT_LT(timing[first], timing[second]);
    }

private:
    void on_message(peer_id_t peer, read_stream_t *stream) {
        int i;
        archive_result_t res = deserialize(stream, &i);
        if (bad(res)) { throw fake_archive_exc_t(); }
        on_thread_t th(home_thread());
        inbox[i] = peer;
        timing[i] = sequence_number++;
    }

    message_service_t *service;
    std::map<int, peer_id_t> inbox;
    std::map<int, int> timing;
    int sequence_number;
};

class dummy_message_handler_t : public message_handler_t {
public:
    void on_message(peer_id_t, read_stream_t *stream) {
        char msg;
        if (force_read(stream, &msg, 1) != 1) {
            throw fake_archive_exc_t();
        }
    }
};

/* `StartStop` starts a cluster of three nodes, then shuts it down again. */

TPTEST_MULTITHREAD(RPCConnectivityTest, StartStop, 3) {
    dummy_message_handler_t mh;
    connectivity_cluster_t c1, c2, c3;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);
    cr2.join(c1.get_peer_address(c1.get_me()));
    cr3.join(c1.get_peer_address(c1.get_me()));
    let_stuff_happen();
}


/* `Message` sends some simple messages between the nodes of a cluster. */

TPTEST_MULTITHREAD(RPCConnectivityTest, Message, 3) {
    connectivity_cluster_t c1, c2, c3;
    recording_test_application_t a1(&c1), a2(&c2), a3(&c3);
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a2, 0, NULL);
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a3, 0, NULL);
    cr2.join(c1.get_peer_address(c1.get_me()));
    cr3.join(c1.get_peer_address(c1.get_me()));

    let_stuff_happen();

    a1.send(873, c2.get_me());
    a2.send(66663, c1.get_me());
    a3.send(6849, c1.get_me());
    a3.send(999, c3.get_me());

    let_stuff_happen();

    a2.expect(873, c1.get_me());
    a1.expect(66663, c2.get_me());
    a1.expect(6849, c3.get_me());
    a3.expect(999, c3.get_me());
}

/* `UnreachablePeer` tests that messages sent to unreachable peers silently
fail. */

TPTEST_MULTITHREAD(RPCConnectivityTest, UnreachablePeer, 3) {
    connectivity_cluster_t c1, c2;
    recording_test_application_t a1(&c1), a2(&c2);
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a2, 0, NULL);

    /* Note that we DON'T join them together. */

    let_stuff_happen();

    a1.send(888, c2.get_me());

    let_stuff_happen();

    /* The message should not have been delivered. The system shouldn't have
    crashed, either. */
    a2.expect_undelivered(888);

    cr1.join(c2.get_peer_address(c2.get_me()));

    let_stuff_happen();

    a1.send(999, c2.get_me());

    let_stuff_happen();

    a2.expect_undelivered(888);
    a2.expect(999, c1.get_me());
}

/* `Ordering` tests that messages sent by the same route arrive in the same
order they were sent in. */

TPTEST_MULTITHREAD(RPCConnectivityTest, Ordering, 3) {
    connectivity_cluster_t c1, c2;
    recording_test_application_t a1(&c1), a2(&c2);
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a2, 0, NULL);

    cr1.join(c2.get_peer_address(c2.get_me()));

    let_stuff_happen();

    for (int i = 0; i < 10; i++) {
        a1.send(i, c2.get_me());
        a1.send(i, c1.get_me());
    }

    let_stuff_happen();

    for (int i = 0; i < 9; i++) {
        a1.expect_order(i, i+1);
        a2.expect_order(i, i+1);
    }
}

/* `GetPeersList` confirms that the behavior of `cluster_t::get_peers_list()` is
correct. */

TPTEST_MULTITHREAD(RPCConnectivityTest, GetPeersList, 3) {
    dummy_message_handler_t mh;
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);

    /* Make sure `get_peers_list()` is initially sane */
    std::set<peer_id_t> list_1 = c1.get_peers_list();
    EXPECT_TRUE(list_1.find(c1.get_me()) != list_1.end());
    EXPECT_EQ(1u, list_1.size());

    {
        connectivity_cluster_t c2;
        connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);
        cr2.join(c1.get_peer_address(c1.get_me()));

        let_stuff_happen();

        /* Make sure `get_peers_list()` correctly notices that a peer connects */
        std::set<peer_id_t> list_2 = c1.get_peers_list();
        ASSERT_TRUE(list_2.find(c2.get_me()) != list_2.end());
        EXPECT_EQ(cr2.get_port(), c1.get_peer_address(c2.get_me()).ips().begin()->port().value());

        /* `c2`'s destructor is called here */
    }

    let_stuff_happen();

    /* Make sure `get_peers_list()` notices that a peer has disconnected */
    std::set<peer_id_t> list_3 = c1.get_peers_list();
    EXPECT_EQ(1u, list_3.size());
}

/* `EventWatchers` confirms that `disconnect_watcher_t` and
`connectivity_service_t::peers_list_subscription_t` work properly. */

TPTEST_MULTITHREAD(RPCConnectivityTest, EventWatchers, 3) {
    dummy_message_handler_t mh;
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);

    connectivity_cluster_t c2;
    scoped_ptr_t<connectivity_cluster_t::run_t> cr2(new connectivity_cluster_t::run_t(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL));

    /* Make sure `c1` notifies us when `c2` connects */
    struct : public cond_t, public peers_list_callback_t {
        void on_connect(UNUSED peer_id_t peer) {
            pulse();
        }
        void on_disconnect(UNUSED peer_id_t peer) { }
    } connection_established;
    connectivity_service_t::peers_list_subscription_t subs(&connection_established);

    {
        connectivity_service_t::peers_list_freeze_t freeze(&c1);
        if (c1.get_peers_list().count(c2.get_me()) == 0) {
            subs.reset(&c1, &freeze);
        } else {
            connection_established.pulse();
        }
    }

    EXPECT_FALSE(connection_established.is_pulsed());
    cr1.join(c2.get_peer_address(c2.get_me()));
    let_stuff_happen();
    EXPECT_TRUE(connection_established.is_pulsed());

    /* Make sure `c1` notifies us when `c2` disconnects */
    disconnect_watcher_t disconnect_watcher(&c1, c2.get_me());
    EXPECT_FALSE(disconnect_watcher.is_pulsed());
    cr2.reset();
    let_stuff_happen();
    EXPECT_TRUE(disconnect_watcher.is_pulsed());

    /* Make sure `disconnect_watcher_t` works for an already-unconnected peer */
    disconnect_watcher_t disconnect_watcher_2(&c1, c2.get_me());
    EXPECT_TRUE(disconnect_watcher_2.is_pulsed());
}

/* `EventWatcherOrdering` confirms that information delivered via event
notification is consistent with information delivered via `get_peers_list()`. */

struct watcher_t : private peers_list_callback_t {

    watcher_t(connectivity_cluster_t *c, recording_test_application_t *a) :
        cluster(c),
        application(a),
        event_watcher(this) {
        connectivity_service_t::peers_list_freeze_t freeze(cluster);
        event_watcher.reset(cluster, &freeze);
    }

    void on_connect(peer_id_t p) {
        /* When we get a connection event, make sure that the peer address
        is present in the routing table */
        std::set<peer_id_t> list = cluster->get_peers_list();
        EXPECT_TRUE(list.find(p) != list.end());

        /* Make sure messages sent from connection events are delivered
        properly. We must use `coro_t::spawn_now_dangerously()` because `send_message()`
        may block. */
        coro_t::spawn_now_dangerously(
                std::bind(&recording_test_application_t::send, application, 89765, p));
    }

    void on_disconnect(peer_id_t p) {
        /* When we get a disconnection event, make sure that the peer
        address is gone from the routing table */
        std::set<peer_id_t> list = cluster->get_peers_list();
        EXPECT_TRUE(list.find(p) == list.end());
    }

    connectivity_cluster_t *cluster;
    recording_test_application_t *application;
    connectivity_service_t::peers_list_subscription_t event_watcher;
};

TPTEST_MULTITHREAD(RPCConnectivityTest, EventWatcherOrdering, 3) {
    connectivity_cluster_t c1;
    recording_test_application_t a1(&c1);
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a1, 0, NULL);

    watcher_t watcher(&c1, &a1);

    /* Generate some connection/disconnection activity */
    {
        connectivity_cluster_t c2;
        recording_test_application_t a2(&c2);
        connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a2, 0, NULL);
        cr2.join(c1.get_peer_address(c1.get_me()));

        let_stuff_happen();

        /* Make sure that the message sent in `on_connect()` was delivered */
        a2.expect(89765, c1.get_me());
    }

    let_stuff_happen();
}

/* `StopMidJoin` makes sure that nothing breaks if you shut down the cluster
while it is still coming up */

TPTEST_MULTITHREAD(RPCConnectivityTest, StopMidJoin, 3) {
    const int num_members = 5;

    /* Spin up `num_members` cluster-members */
    dummy_message_handler_t mh;
    scoped_ptr_t<connectivity_cluster_t> nodes[num_members];
    scoped_ptr_t<connectivity_cluster_t::run_t> runs[num_members];
    for (int i = 0; i < num_members; i++) {
        nodes[i].init(new connectivity_cluster_t);
        runs[i].init(new connectivity_cluster_t::run_t(nodes[i].get(), get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL));
    }
    for (int i = 1; i < num_members; i++) {
        runs[i]->join(nodes[0]->get_peer_address(nodes[0]->get_me()));
    }

    coro_t::yield();

    EXPECT_NE(static_cast<size_t>(num_members), nodes[1]->get_peers_list().size()) << "This test is "
        "supposed to test what happens when a cluster is interrupted as it "
        "starts up, but the cluster finished starting up before we could "
        "interrupt it.";

    /* Shut down cluster nodes and hope nothing crashes. (The destructors do the
    work of shutting down.) */
}

/* `BlobJoin` tests whether two groups of cluster nodes can correctly merge
together. */

TPTEST_MULTITHREAD(RPCConnectivityTest, BlobJoin, 3) {
    /* Two blobs of `blob_size` nodes */
    const size_t blob_size = 4;

    /* Spin up cluster-members */
    dummy_message_handler_t mh;
    scoped_ptr_t<connectivity_cluster_t> nodes[blob_size * 2];
    scoped_ptr_t<connectivity_cluster_t::run_t> runs[blob_size * 2];
    for (size_t i = 0; i < blob_size * 2; i++) {
        nodes[i].init(new connectivity_cluster_t);
        runs[i].init(new connectivity_cluster_t::run_t(nodes[i].get(), get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL));
    }

    for (size_t i = 1; i < blob_size; i++) {
        runs[i]->join(nodes[0]->get_peer_address(nodes[0]->get_me()));
    }
    for (size_t i = blob_size+1; i < blob_size*2; i++) {
        runs[i]->join(nodes[blob_size]->get_peer_address(nodes[blob_size]->get_me()));
    }

    // Allow some time for the two blobs to join with themselves
    uint32_t total_waits = 0;
    bool pass = false;
    while (!pass) {
        let_stuff_happen();
        ++total_waits;
        ASSERT_LT(total_waits, 50u);  // cluster blobs took to long to coalesce internally

        pass = true;
        for (size_t i = 0; i < blob_size * 2; i++) {
            pass &= (blob_size == nodes[i]->get_peers_list().size());
        }
    }

    // Link the two blobs
    runs[1]->join(nodes[blob_size+1]->get_peer_address(nodes[blob_size+1]->get_me()));

    pass = false;
    while (!pass) {
        let_stuff_happen();
        ++total_waits;
        ASSERT_LT(total_waits, 50u); // cluster blobs took to long to coalesce with each other

        pass = true;
        for (size_t i = 0; i < blob_size * 2; i++) {
            pass &= ((blob_size * 2) == nodes[i]->get_peers_list().size());
        }
    }
}

/* `Multiplexer` tests `message_multiplexer_t`. */

TPTEST(RPCConnectivityTest, Multiplexer) {
    connectivity_cluster_t c1, c2;
    message_multiplexer_t c1m(&c1), c2m(&c2);
    message_multiplexer_t::client_t c1mcA(&c1m, 'A'), c2mcA(&c2m, 'A');
    recording_test_application_t c1aA(&c1mcA), c2aA(&c2mcA);
    message_multiplexer_t::client_t::run_t c1mcAr(&c1mcA, &c1aA), c2mcAr(&c2mcA, &c2aA);
    message_multiplexer_t::client_t c1mcB(&c1m, 'B'), c2mcB(&c2m, 'B');
    recording_test_application_t c1aB(&c1mcB), c2aB(&c2mcB);
    message_multiplexer_t::client_t::run_t c1mcBr(&c1mcB, &c1aB), c2mcBr(&c2mcB, &c2aB);
    message_multiplexer_t::run_t c1mr(&c1m), c2mr(&c2m);
    connectivity_cluster_t::run_t c1r(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &c1mr, 0, NULL);
    connectivity_cluster_t::run_t c2r(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &c2mr, 0, NULL);

    c1r.join(c2.get_peer_address(c2.get_me()));
    let_stuff_happen();

    c1aA.send(10065, c2.get_me());
    c1aB.send(10066, c2.get_me());
    c1aA.send(65, c1.get_me());

    let_stuff_happen();

    c2aA.expect(10065, c1.get_me());
    c2aB.expect(10066, c1.get_me());
    c1aA.expect(65, c1.get_me());
    c2aA.expect_undelivered(10066);
    c2aB.expect_undelivered(10065);
}

/* `BinaryData` makes sure that any octet can be sent over the wire. */

class binary_test_application_t : public message_handler_t {
public:
    explicit binary_test_application_t(message_service_t *s) :
        service(s),
        got_spectrum(false)
        { }
    void send_spectrum(peer_id_t peer) {
        class dump_spectrum_writer_t : public send_message_write_callback_t {
        public:
            virtual ~dump_spectrum_writer_t() { }
            void write(write_stream_t *stream) {
                char spectrum[CHAR_MAX - CHAR_MIN + 1];
                for (int i = CHAR_MIN; i <= CHAR_MAX; i++) spectrum[i - CHAR_MIN] = i;
                int64_t res = stream->write(spectrum, CHAR_MAX - CHAR_MIN + 1);
                if (res != CHAR_MAX - CHAR_MIN + 1) { throw fake_archive_exc_t(); }
            }
        } writer;
        service->send_message(peer, &writer);
    }
    void on_message(peer_id_t, read_stream_t *stream) {
        char spectrum[CHAR_MAX - CHAR_MIN + 1];
        int64_t res = force_read(stream, spectrum, CHAR_MAX - CHAR_MIN + 1);
        if (res != CHAR_MAX - CHAR_MIN + 1) { throw fake_archive_exc_t(); }

        for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
            EXPECT_EQ(spectrum[i - CHAR_MIN], i);
        }
        got_spectrum = true;
    }
    message_service_t *service;
    bool got_spectrum;
};

TPTEST_MULTITHREAD(RPCConnectivityTest, BinaryData, 3) {
    connectivity_cluster_t c1, c2;
    binary_test_application_t a1(&c1), a2(&c2);
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a1, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &a2, 0, NULL);
    cr1.join(c2.get_peer_address(c2.get_me()));

    let_stuff_happen();

    a1.send_spectrum(c2.get_me());

    let_stuff_happen();

    EXPECT_TRUE(a2.got_spectrum);
}

/* `PeerIDSemantics` makes sure that `peer_id_t::is_nil()` works as expected. */
TPTEST_MULTITHREAD(RPCConnectivityTest, PeerIDSemantics, 3) {
    peer_id_t nil_peer;
    ASSERT_TRUE(nil_peer.is_nil());

    connectivity_cluster_t cluster_node;
    ASSERT_FALSE(cluster_node.get_me().is_nil());
}

fd_t connect_to_node_ipv4(const ip_and_port_t &ip_port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ip_port.port().value());
    addr.sin_addr = ip_port.ip().get_ipv4_addr();

    fd_t sock(::socket(AF_INET, SOCK_STREAM, 0));
    guarantee_err(sock != INVALID_FD, "could not open socket to connect to cluster node");
    int res = ::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    guarantee_err(res == 0, "could not connect to cluster node");
    return sock;
}

fd_t connect_to_node_ipv6(const ip_and_port_t &ip_port) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(ip_port.port().value());
    addr.sin6_addr = ip_port.ip().get_ipv6_addr();

    fd_t sock(::socket(AF_INET6, SOCK_STREAM, 0));
    guarantee_err(sock != INVALID_FD, "could not open socket to connect to cluster node");
    int res = ::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    guarantee_err(res == 0, "could not connect to cluster node");
    return sock;
}

fd_t connect_to_node(const ip_and_port_t &ip_port) {
    fd_t result;

    if (ip_port.ip().is_ipv4()) {
        result = connect_to_node_ipv4(ip_port);
    } else if (ip_port.ip().is_ipv6()) {
        result = connect_to_node_ipv6(ip_port);
    } else {
        crash("unknown address type");
    }

    return result;
}

// Make sure each side of the connection is closed
void check_tcp_closed(socket_stream_t *stream) {
    // Allow 6 seconds before timing out
    signal_timer_t interruptor;
    interruptor.start(6000);

    stream->set_interruptor(&interruptor);

    try {
        char buffer[1024];
        int64_t res;
        do {
            res = stream->read(&buffer, 1024);
        } while (res > 0);

        do {
            let_stuff_happen();
            res = stream->write("a", 1);
        } while(res != -1);
    } catch (const interrupted_exc_t &ex) {
        FAIL() << "test took too long to detect connection was down";
    }

    ASSERT_FALSE(stream->is_write_open());
    ASSERT_FALSE(stream->is_read_open());
}

// `CheckHeaders` makes sure that we close the connection if we get a malformed header.
TPTEST(RPCConnectivityTest, CheckHeaders) {
    // Set up a cluster node.
    dummy_message_handler_t mh;
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);

    // Manually connect to the cluster.
    peer_address_t addr = c1.get_peer_address(c1.get_me());
    scoped_fd_t sock(connect_to_node(*addr.ips().begin()));
    socket_stream_t stream(sock.get());

    // Read & check its header.
    const int64_t len = connectivity_cluster_t::cluster_proto_header.length();
    {
        scoped_array_t<char> data(len + 1);
        int64_t read = force_read(&stream, data.data(), len);
        ASSERT_GE(read, 0);
        data[read] = 0;         // null-terminate
        ASSERT_STREQ(connectivity_cluster_t::cluster_proto_header.c_str(), data.data());
    }

    // Send it an initially okay-looking but ultimately malformed header.
    const int64_t initlen = 10;
    ASSERT_TRUE(initlen < len); // sanity check
    ASSERT_TRUE(initlen == stream.write(connectivity_cluster_t::cluster_proto_header.c_str(), initlen));
    let_stuff_happen();
    ASSERT_TRUE(stream.is_read_open() && stream.is_write_open());

    // Send malformed continuation.
    char badchar = connectivity_cluster_t::cluster_proto_header[initlen] ^ 0x7f;
    ASSERT_EQ(1, stream.write(&badchar, 1));
    let_stuff_happen();

    check_tcp_closed(&stream);
}

TPTEST(RPCConnectivityTest, DifferentVersion) {
    // Set up a cluster node.
    dummy_message_handler_t mh;
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);

    // Manually connect to the cluster.
    peer_address_t addr = c1.get_peer_address(c1.get_me());
    scoped_fd_t sock(connect_to_node(*addr.ips().begin()));
    socket_stream_t stream(sock.get());

    // Read & check its header.
    const int64_t len = connectivity_cluster_t::cluster_proto_header.length();
    {
        scoped_array_t<char> data(len + 1);
        int64_t read = force_read(&stream, data.data(), len);
        ASSERT_GE(read, 0);
        data[read] = 0;         // null-terminate
        ASSERT_STREQ(connectivity_cluster_t::cluster_proto_header.c_str(), data.data());
    }

    // Send the base header
    ASSERT_EQ(len,
              stream.write(connectivity_cluster_t::cluster_proto_header.c_str(),
                           connectivity_cluster_t::cluster_proto_header.length()));
    let_stuff_happen();
    ASSERT_TRUE(stream.is_read_open() && stream.is_write_open());

    // Send bad version
    std::string bad_version_str("0.1.1b");
    write_message_t bad_version_msg;
    serialize(&bad_version_msg, bad_version_str.length());
    bad_version_msg.append(bad_version_str.data(), bad_version_str.length());
    serialize(&bad_version_msg, connectivity_cluster_t::cluster_arch_bitsize.length());
    bad_version_msg.append(connectivity_cluster_t::cluster_arch_bitsize.data(),
                           connectivity_cluster_t::cluster_arch_bitsize.length());
    serialize(&bad_version_msg, connectivity_cluster_t::cluster_build_mode.length());
    bad_version_msg.append(connectivity_cluster_t::cluster_build_mode.data(),
                           connectivity_cluster_t::cluster_build_mode.length());
    ASSERT_FALSE(send_write_message(&stream, &bad_version_msg));
    let_stuff_happen();

    check_tcp_closed(&stream);
}

TPTEST(RPCConnectivityTest, DifferentArch) {
    // Set up a cluster node.
    dummy_message_handler_t mh;
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);

    // Manually connect to the cluster.
    peer_address_t addr = c1.get_peer_address(c1.get_me());
    scoped_fd_t sock(connect_to_node(*addr.ips().begin()));
    socket_stream_t stream(sock.get());

    // Read & check its header.
    const int64_t len = connectivity_cluster_t::cluster_proto_header.length();
    {
        scoped_array_t<char> data(len + 1);
        int64_t read = force_read(&stream, data.data(), len);
        ASSERT_GE(read, 0);
        data[read] = 0;         // null-terminate
        ASSERT_STREQ(connectivity_cluster_t::cluster_proto_header.c_str(), data.data());
    }

    // Send the base header
    ASSERT_EQ(len,
              stream.write(connectivity_cluster_t::cluster_proto_header.c_str(),
                           connectivity_cluster_t::cluster_proto_header.length()));
    let_stuff_happen();
    ASSERT_TRUE(stream.is_read_open() && stream.is_write_open());

    // Send the expected version but bad arch bitsize
    std::string bad_arch_str("96bit");
    write_message_t bad_arch_msg;
    serialize(&bad_arch_msg, connectivity_cluster_t::cluster_version.length());
    bad_arch_msg.append(connectivity_cluster_t::cluster_version.data(),
                        connectivity_cluster_t::cluster_version.length());
    serialize(&bad_arch_msg, bad_arch_str.length());
    bad_arch_msg.append(bad_arch_str.data(), bad_arch_str.length());
    serialize(&bad_arch_msg, connectivity_cluster_t::cluster_build_mode.length());
    bad_arch_msg.append(connectivity_cluster_t::cluster_build_mode.data(),
                        connectivity_cluster_t::cluster_build_mode.length());
    ASSERT_FALSE(send_write_message(&stream, &bad_arch_msg));
    let_stuff_happen();

    check_tcp_closed(&stream);
}

TPTEST(RPCConnectivityTest, DifferentBuildMode) {
    // Set up a cluster node.
    dummy_message_handler_t mh;
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);

    // Manually connect to the cluster.
    peer_address_t addr = c1.get_peer_address(c1.get_me());
    scoped_fd_t sock(connect_to_node(*addr.ips().begin()));
    socket_stream_t stream(sock.get());

    // Read & check its header.
    const int64_t len = connectivity_cluster_t::cluster_proto_header.length();
    {
        scoped_array_t<char> data(len + 1);
        int64_t read = force_read(&stream, data.data(), len);
        ASSERT_GE(read, 0);
        data[read] = 0;         // null-terminate
        ASSERT_STREQ(connectivity_cluster_t::cluster_proto_header.c_str(), data.data());
    }

    // Send the base header
    ASSERT_EQ(len,
              stream.write(connectivity_cluster_t::cluster_proto_header.c_str(),
                           connectivity_cluster_t::cluster_proto_header.length()));
    let_stuff_happen();
    ASSERT_TRUE(stream.is_read_open() && stream.is_write_open());

    // Send the expected version but bad arch bitsize
    std::string bad_build_mode_str("build mode activated");
    write_message_t bad_build_mode_msg;
    serialize(&bad_build_mode_msg, connectivity_cluster_t::cluster_version.length());
    bad_build_mode_msg.append(connectivity_cluster_t::cluster_version.data(),
                              connectivity_cluster_t::cluster_version.length());
    serialize(&bad_build_mode_msg, connectivity_cluster_t::cluster_arch_bitsize.length());
    bad_build_mode_msg.append(connectivity_cluster_t::cluster_arch_bitsize.data(),
                              connectivity_cluster_t::cluster_arch_bitsize.length());
    serialize(&bad_build_mode_msg, bad_build_mode_str.length());
    bad_build_mode_msg.append(bad_build_mode_str.data(), bad_build_mode_str.length());
    ASSERT_FALSE(send_write_message(&stream, &bad_build_mode_msg));
    let_stuff_happen();

    check_tcp_closed(&stream);
}

std::set<host_and_port_t> convert_from_any_port(const std::set<host_and_port_t> &addresses,
                                                port_t actual_port) {
    std::set<host_and_port_t> result;
    for (auto it = addresses.begin(); it != addresses.end(); ++it) {
        if (it->port().value() == 0) {
            result.insert(host_and_port_t(it->host(), actual_port));
        } else {
            result.insert(*it);
        }
    }
    return result;
}

// This could possibly cause some weird behavior on someone's network
TPTEST(RPCConnectivityTest, CanonicalAddress) {
    // cr1 should use default addresses, cr2 and cr3 should use canonical addresses
    std::set<host_and_port_t> c2_addresses;
    c2_addresses.insert(host_and_port_t("10.9.9.254", port_t(0)));
    c2_addresses.insert(host_and_port_t("192.168.255.55", port_t(0)));
    c2_addresses.insert(host_and_port_t("192.168.255.55", port_t(0)));
    c2_addresses.insert(host_and_port_t("3aa2:8fe3::afa3:4843", port_t(0)));

    std::set<host_and_port_t> c3_addresses;
    c3_addresses.insert(host_and_port_t("10.9.9.254", port_t(6811)));
    c3_addresses.insert(host_and_port_t("10.255.255.255", port_t(0)));
    c3_addresses.insert(host_and_port_t("192.168.255.55", port_t(1034)));
    c3_addresses.insert(host_and_port_t("::c3a4:7159:27:aa78", port_t(1034)));

    // Note: this won't have full connectivity in the unit test because we aren't actually using
    //  a proxy or anything
    dummy_message_handler_t mh;
    connectivity_cluster_t c1, c2, c3;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(), ANY_PORT, &mh, 0, NULL);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(c2_addresses), ANY_PORT, &mh, 0, NULL);
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(c3_addresses), ANY_PORT, &mh, 0, NULL);

    int c2_port = 0;
    peer_address_t c2_self_address = c2.get_peer_address(c2.get_me());
    ip_address_t chosen_c2_addr("10.9.9.254");
    for (auto it = c2_self_address.ips().begin();
         it != c2_self_address.ips().end(); ++it) {
        if (it->ip() == chosen_c2_addr) {
            c2_port = it->port().value();
        }
    }
    ASSERT_NE(0, c2_port);

    int c3_port = 0;
    peer_address_t c3_self_address = c3.get_peer_address(c3.get_me());
    ip_address_t chosen_c3_addr("10.255.255.255");
    for (auto it = c3_self_address.ips().begin();
         it != c3_self_address.ips().end(); ++it) {
        if (it->ip() == chosen_c3_addr) {
            c3_port = it->port().value();
        }
    }
    ASSERT_NE(0, c3_port);

    // Convert the addresses to what we expect
    c2_addresses = convert_from_any_port(c2_addresses, port_t(c2_port));
    peer_address_t c2_peer_address(c2_addresses);

    c3_addresses = convert_from_any_port(c3_addresses, port_t(c3_port));
    peer_address_t c3_peer_address(c3_addresses);

    // Join the cluster together
    cr2.join(c1.get_peer_address(c1.get_me()));
    cr3.join(c1.get_peer_address(c1.get_me()));

    let_stuff_happen();

    // Check that the right addresses are available from the other peer
    // Note that peers 2 and 3 can't actually connect to each other, since they
    //  have fake canonical addresses (unless you have a very fucked up network)
    peer_address_t c2_addr_from_c1 = c1.get_peer_address(c2.get_me());
    peer_address_t c3_addr_from_c1 = c1.get_peer_address(c3.get_me());

    if (c2_addr_from_c1 != c2_peer_address) {
        printf_buffer_t buffer1;
        debug_print(&buffer1, c2_addr_from_c1);
        printf_buffer_t buffer2;
        debug_print(&buffer2, c2_peer_address);

        FAIL() << "peer address mismatch:"
            << "\n  addr from c1: " << buffer1.c_str()
            << "\n  expected: " << buffer2.c_str();
    }
    // cool

    if (c3_addr_from_c1 != c3_peer_address) {
        printf_buffer_t buffer1;
        debug_print(&buffer1, c3_addr_from_c1);
        printf_buffer_t buffer2;
        debug_print(&buffer2, c3_peer_address);

        FAIL() << "peer address mismatch:"
            << "\n  addr from c1: " << buffer1.c_str()
            << "\n  expected: " << buffer2.c_str();
    }
    // cool cool cool
}

}   /* namespace unittest */
