// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>

#include "arch/runtime/thread_pool.hpp"
#include "arch/timing.hpp"
#ifdef _WIN32
#include "windows.hpp"
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif
#include "containers/scoped.hpp"
#include "containers/archive/socket_stream.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

/* `recording_test_application_t` sends and receives integers over a
`message_service_t`. It keeps track of the integers it has received.
*/

class recording_test_application_t :
    public home_thread_mixin_t,
    public cluster_message_handler_t
{
public:
    explicit recording_test_application_t(connectivity_cluster_t *cm,
                                          connectivity_cluster_t::message_tag_t tag) :
        cluster_message_handler_t(cm, tag),
        sequence_number(0)
        { }
    void send(int message, peer_id_t peer) {
        auto_drainer_t::lock_t connection_keepalive;
        connectivity_cluster_t::connection_t *connection =
            get_connectivity_cluster()->get_connection(peer, &connection_keepalive);
        if (connection) {
            send(message, connection, connection_keepalive);
        }
    }
    void send(int message, connectivity_cluster_t::connection_t *connection,
            auto_drainer_t::lock_t connection_keepalive) {
        class writer_t : public cluster_send_message_write_callback_t {
        public:
            explicit writer_t(int _data) : data(_data) { }
            virtual ~writer_t() { }
            void write(write_stream_t *stream) {
                write_message_t wm;
                serialize<cluster_version_t::CLUSTER>(&wm, data);
                int res = send_write_message(stream, &wm);
                if (res) { throw fake_archive_exc_t(); }
            }
#ifdef ENABLE_MESSAGE_PROFILER
            const char *message_profiler_tag() const {
                return "unittest";
            }
#endif
            int32_t data;
        } writer(message);
        get_connectivity_cluster()->send_message(connection, connection_keepalive,
            get_message_tag(), &writer);
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
    void on_message(connectivity_cluster_t::connection_t *connection,
                    auto_drainer_t::lock_t,
                    read_stream_t *stream) {
        int i;
        archive_result_t res
            = deserialize<cluster_version_t::CLUSTER>(stream, &i);
        if (bad(res)) { throw fake_archive_exc_t(); }
        on_thread_t th(home_thread());
        inbox[i] = connection->get_peer_id();
        timing[i] = sequence_number++;
    }

    std::map<int, peer_id_t> inbox;
    std::map<int, int> timing;
    int sequence_number;
};

/* `StartStop` starts a cluster of three nodes, then shuts it down again. */

TPTEST_MULTITHREAD(RPCConnectivityTest, StartStop, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2, c3;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    cr2.join(get_cluster_local_address(&c1));
    cr3.join(get_cluster_local_address(&c1));
    let_stuff_happen();
}


/* `Message` sends some simple messages between the nodes of a cluster. */

TPTEST_MULTITHREAD(RPCConnectivityTest, Message, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2, c3;
    recording_test_application_t a1(&c1, 'T'), a2(&c2, 'T'), a3(&c3, 'T');
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    cr2.join(get_cluster_local_address(&c1));
    cr3.join(get_cluster_local_address(&c1));

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
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2;
    recording_test_application_t a1(&c1, 'T'), a2(&c2, 'T');
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    /* Note that we DON'T join them together. */

    let_stuff_happen();

    /* We shouldn't see the other peer, and checking this shouldn't crash anything */
    auto_drainer_t::lock_t connection_keepalive;
    connectivity_cluster_t::connection_t *conn =
        c1.get_connection(c2.get_me(), &connection_keepalive);
    EXPECT_TRUE(conn == NULL);

    a1.send(888, c2.get_me());

    let_stuff_happen();

    /* The message should not have been delivered. The system shouldn't have
    crashed, either. */
    a2.expect_undelivered(888);

    cr1.join(get_cluster_local_address(&c2));

    let_stuff_happen();

    a1.send(999, c2.get_me());

    let_stuff_happen();

    a2.expect_undelivered(888);
    a2.expect(999, c1.get_me());
}

/* `LostPeer` tests that nothing crashes when we lose a connection. */

TPTEST_MULTITHREAD(RPCConnectivityTest, LostPeer, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2;
    recording_test_application_t a1(&c1, 'T'), a2(&c2, 'T');
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    auto_drainer_t::lock_t connection_keepalive;
    connectivity_cluster_t::connection_t *connection;

    {
        connectivity_cluster_t::run_t cr2(
            &c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());
        cr2.join(get_cluster_local_address(&c1));

        let_stuff_happen();

        connection = c1.get_connection(c2.get_me(), &connection_keepalive);
        ASSERT_TRUE(connection != NULL);
        EXPECT_FALSE(connection_keepalive.get_drain_signal()->is_pulsed());
        EXPECT_TRUE(connection->get_peer_id() == c2.get_me());

        /* The `cr2` destructor happens here */
    }

    let_stuff_happen();

    /* `c1` should see that `c2` is disconnected */
    EXPECT_TRUE(connection_keepalive.get_drain_signal()->is_pulsed());
    /* `connection` should still be valid */
    EXPECT_TRUE(connection->get_peer_id() == c2.get_me());
    a1.send(246, connection, connection_keepalive);

    /* The `connection_keepalive` destructor must run before the `cr1` destructor */
}

/* `Ordering` tests that messages sent by the same route arrive in the same
order they were sent in.

TODO: Maybe we should drop this test. See the note about reordering in `cluster.hpp`. */

TPTEST_MULTITHREAD(RPCConnectivityTest, Ordering, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2;
    recording_test_application_t a1(&c1, 'T'), a2(&c2, 'T');
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    cr1.join(get_cluster_local_address(&c2));

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

/* `GetConnections` confirms that the behavior of `cluster_t::get_connections()` is
correct. */

TPTEST_MULTITHREAD(RPCConnectivityTest, GetConnections, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    /* Make sure `get_connections()` is initially sane */
    std::map<peer_id_t, connectivity_cluster_t::connection_pair_t> list_1 =
        c1.get_connections()->get_all();
    EXPECT_TRUE(list_1.find(c1.get_me()) != list_1.end());
    EXPECT_EQ(1u, list_1.size());

    {
        connectivity_cluster_t c2;
        connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(),
            peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());
        cr2.join(get_cluster_local_address(&c1));

        let_stuff_happen();

        /* Make sure `get_connections()` correctly notices that a peer connects */
        std::map<peer_id_t, connectivity_cluster_t::connection_pair_t> list_2 =
            c1.get_connections()->get_all();
        ASSERT_TRUE(list_2.find(c2.get_me()) != list_2.end());
        EXPECT_EQ(
            cr2.get_port(),
            list_2[c2.get_me()].first->get_peer_address().ips().begin()->port().value());

        /* `c2`'s destructor is called here */
    }

    let_stuff_happen();

    /* Make sure `get_peers_list()` notices that a peer has disconnected */
    std::map<peer_id_t, connectivity_cluster_t::connection_pair_t> list_3 =
        c1.get_connections()->get_all();
    EXPECT_EQ(1u, list_3.size());
}

/* `StopMidJoin` makes sure that nothing breaks if you shut down the cluster
while it is still coming up */

TPTEST_MULTITHREAD(RPCConnectivityTest, StopMidJoin, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    const int num_members = 5;

    /* Spin up `num_members` cluster-members */
    object_buffer_t<connectivity_cluster_t> nodes[num_members];
    object_buffer_t<connectivity_cluster_t::run_t> runs[num_members];
    for (int i = 0; i < num_members; i++) {
        nodes[i].create();
        runs[i].create(nodes[i].get(), get_unittest_addresses(), peer_address_t(),
            ANY_PORT, 0, heartbeat_manager.get_view());
    }
    for (int i = 1; i < num_members; i++) {
        runs[i]->join(get_cluster_local_address(nodes[0].get()));
    }

    coro_t::yield();

    EXPECT_NE(
        static_cast<size_t>(num_members),
        nodes[1]->get_connections()->get_all().size())
      << "This test is supposed to test what happens when a cluster is "
         "interrupted as it starts up, but the cluster finished starting up "
         "before we could interrupt it.";

    /* Shut down cluster nodes and hope nothing crashes. (The destructors do the
    work of shutting down.) */
}

/* `BlobJoin` tests whether two groups of cluster nodes can correctly merge
together. */

TPTEST_MULTITHREAD(RPCConnectivityTest, BlobJoin, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    /* Two blobs of `blob_size` nodes */
    const size_t blob_size = 4;

    /* Spin up cluster-members */
    object_buffer_t<connectivity_cluster_t> nodes[blob_size * 2];
    object_buffer_t<connectivity_cluster_t::run_t> runs[blob_size * 2];
    for (size_t i = 0; i < blob_size * 2; i++) {
        nodes[i].create();
        runs[i].create(nodes[i].get(), get_unittest_addresses(), peer_address_t(),
            ANY_PORT, 0, heartbeat_manager.get_view());
    }

    for (size_t i = 1; i < blob_size; i++) {
        runs[i]->join(get_cluster_local_address(nodes[0].get()));
    }
    for (size_t i = blob_size+1; i < blob_size*2; i++) {
        runs[i]->join(get_cluster_local_address(nodes[blob_size].get()));
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
            pass &= (blob_size == nodes[i]->get_connections()->get_all().size());
        }
    }

    // Link the two blobs
    runs[1]->join(get_cluster_local_address(nodes[blob_size+1].get()));

    pass = false;
    while (!pass) {
        let_stuff_happen();
        ++total_waits;
        ASSERT_LT(total_waits, 50u); // cluster blobs took to long to coalesce with each other

        pass = true;
        for (size_t i = 0; i < blob_size * 2; i++) {
            pass &= ((blob_size * 2) == nodes[i]->get_connections()->get_all().size());
        }
    }
}

/* `Multiplexer` uses different `message_tag_t`s and checks that the wires don't get
crossed. */
TPTEST(RPCConnectivityTest, Multiplexer) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2;
    recording_test_application_t c1aA(&c1, 'A'), c2aA(&c2, 'A');
    recording_test_application_t c1aB(&c1, 'B'), c2aB(&c2, 'B');
    connectivity_cluster_t::run_t c1r(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t c2r(&c2, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    c1r.join(get_cluster_local_address(&c2));
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

class binary_test_application_t : public cluster_message_handler_t {
public:
    explicit binary_test_application_t(connectivity_cluster_t *cm) :
        cluster_message_handler_t(cm, 'B'),
        got_spectrum(false)
        { }
    void send_spectrum(peer_id_t peer) {
        class dump_spectrum_writer_t :
            public cluster_send_message_write_callback_t {
        public:
            virtual ~dump_spectrum_writer_t() { }
            void write(write_stream_t *stream) {
                char spectrum[CHAR_MAX - CHAR_MIN + 1];
                for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
                    spectrum[i - CHAR_MIN] = i;
                }
                int64_t res = stream->write(spectrum, CHAR_MAX - CHAR_MIN + 1);
                if (res != CHAR_MAX - CHAR_MIN + 1) { throw fake_archive_exc_t(); }
            }
#ifdef ENABLE_MESSAGE_PROFILER
            const char *message_profiler_tag() const {
                return "unittest";
            }
#endif
        } writer;
        auto_drainer_t::lock_t connection_keepalive;
        connectivity_cluster_t::connection_t *connection =
            get_connectivity_cluster()->get_connection(peer, &connection_keepalive);
        ASSERT_TRUE(connection != NULL);
        get_connectivity_cluster()->send_message(connection, connection_keepalive,
                                                 get_message_tag(), &writer);
    }
    void on_message(connectivity_cluster_t::connection_t *,
                    auto_drainer_t::lock_t,
                    read_stream_t *stream) {
        char spectrum[CHAR_MAX - CHAR_MIN + 1];
        int64_t res = force_read(stream, spectrum, CHAR_MAX - CHAR_MIN + 1);
        if (res != CHAR_MAX - CHAR_MIN + 1) { throw fake_archive_exc_t(); }

        for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
            EXPECT_EQ(spectrum[i - CHAR_MIN], i);
        }
        got_spectrum = true;
    }
    bool got_spectrum;
};

TPTEST_MULTITHREAD(RPCConnectivityTest, BinaryData, 3) {
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    connectivity_cluster_t c1, c2;
    binary_test_application_t a1(&c1), a2(&c2);
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    cr1.join(get_cluster_local_address(&c2));

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
    // Initialize a dummy semilattice for the heartbeat metadata
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    // Set up a cluster node.
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    // Manually connect to the cluster.
    peer_address_t addr = get_cluster_local_address(&c1);
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
    // Initialize a dummy semilattice for the heartbeat metadata
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    // Set up a cluster node.
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    // Manually connect to the cluster.
    peer_address_t addr = get_cluster_local_address(&c1);
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
    serialize<cluster_version_t::CLUSTER>(&bad_version_msg,
                                         bad_version_str.length());
    bad_version_msg.append(bad_version_str.data(), bad_version_str.length());
    serialize<cluster_version_t::CLUSTER>(
            &bad_version_msg,
            connectivity_cluster_t::cluster_arch_bitsize.length());
    bad_version_msg.append(connectivity_cluster_t::cluster_arch_bitsize.data(),
                           connectivity_cluster_t::cluster_arch_bitsize.length());
    serialize<cluster_version_t::CLUSTER>(
            &bad_version_msg,
            connectivity_cluster_t::cluster_build_mode.length());
    bad_version_msg.append(connectivity_cluster_t::cluster_build_mode.data(),
                           connectivity_cluster_t::cluster_build_mode.length());
    ASSERT_FALSE(send_write_message(&stream, &bad_version_msg));
    let_stuff_happen();

    check_tcp_closed(&stream);
}

TPTEST(RPCConnectivityTest, DifferentArch) {
    // Initialize a dummy semilattice for the heartbeat metadata
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    // Set up a cluster node.
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    // Manually connect to the cluster.
    peer_address_t addr = get_cluster_local_address(&c1);
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
    serialize<cluster_version_t::CLUSTER>(
            &bad_arch_msg,
            connectivity_cluster_t::cluster_version_string.length());
    bad_arch_msg.append(connectivity_cluster_t::cluster_version_string.data(),
                        connectivity_cluster_t::cluster_version_string.length());
    serialize<cluster_version_t::CLUSTER>(&bad_arch_msg, bad_arch_str.length());
    bad_arch_msg.append(bad_arch_str.data(), bad_arch_str.length());
    serialize<cluster_version_t::CLUSTER>(
            &bad_arch_msg,
            connectivity_cluster_t::cluster_build_mode.length());
    bad_arch_msg.append(connectivity_cluster_t::cluster_build_mode.data(),
                        connectivity_cluster_t::cluster_build_mode.length());
    ASSERT_FALSE(send_write_message(&stream, &bad_arch_msg));
    let_stuff_happen();

    check_tcp_closed(&stream);
}

TPTEST(RPCConnectivityTest, DifferentBuildMode) {
    // Initialize a dummy semilattice for the heartbeat metadata
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

    // Set up a cluster node.
    connectivity_cluster_t c1;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());

    // Manually connect to the cluster.
    peer_address_t addr = get_cluster_local_address(&c1);
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
    serialize<cluster_version_t::CLUSTER>(
            &bad_build_mode_msg,
            connectivity_cluster_t::cluster_version_string.length());
    bad_build_mode_msg.append(connectivity_cluster_t::cluster_version_string.data(),
                              connectivity_cluster_t::cluster_version_string.length());
    serialize<cluster_version_t::CLUSTER>(
            &bad_build_mode_msg,
            connectivity_cluster_t::cluster_arch_bitsize.length());
    bad_build_mode_msg.append(connectivity_cluster_t::cluster_arch_bitsize.data(),
                              connectivity_cluster_t::cluster_arch_bitsize.length());
    serialize<cluster_version_t::CLUSTER>(&bad_build_mode_msg,
                                         bad_build_mode_str.length());
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
    // Initialize a dummy semilattice for the heartbeat metadata
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);

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
    connectivity_cluster_t c1, c2, c3;
    connectivity_cluster_t::run_t cr1(&c1, get_unittest_addresses(), peer_address_t(),
        ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(),
        peer_address_t(c2_addresses), ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t cr3(&c3, get_unittest_addresses(),
        peer_address_t(c3_addresses), ANY_PORT, 0, heartbeat_manager.get_view());

    int c2_port = 0;
    peer_address_t c2_self_address = get_cluster_local_address(&c2);
    ip_address_t chosen_c2_addr("10.9.9.254");
    for (auto it = c2_self_address.ips().begin();
         it != c2_self_address.ips().end(); ++it) {
        if (it->ip() == chosen_c2_addr) {
            c2_port = it->port().value();
        }
    }
    ASSERT_NE(0, c2_port);

    int c3_port = 0;
    peer_address_t c3_self_address = get_cluster_local_address(&c3);
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
    cr2.join(get_cluster_local_address(&c1));
    cr3.join(get_cluster_local_address(&c1));

    let_stuff_happen();

    // Check that the right addresses are available from the other peer
    // Note that peers 2 and 3 can't actually connect to each other, since they
    //  have fake canonical addresses (unless you have a very fucked up network)
    peer_address_t c2_addr_from_c1;
    {
        auto_drainer_t::lock_t connection_keepalive;
        connectivity_cluster_t::connection_t *connection =
            c1.get_connection(c2.get_me(), &connection_keepalive);
        ASSERT_TRUE(connection != NULL);
        c2_addr_from_c1 = connection->get_peer_address();
    }
    peer_address_t c3_addr_from_c1;
    {
        auto_drainer_t::lock_t connection_keepalive;
        connectivity_cluster_t::connection_t *connection =
            c1.get_connection(c3.get_me(), &connection_keepalive);
        ASSERT_TRUE(connection != NULL);
        c3_addr_from_c1 = connection->get_peer_address();
    }

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
