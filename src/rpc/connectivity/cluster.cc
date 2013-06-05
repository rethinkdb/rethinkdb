// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rpc/connectivity/cluster.hpp"

#include "errors.hpp"
#include <boost/optional.hpp>

#include "arch/io/network.hpp"
#include "arch/timing.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/semaphore.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/object_buffer.hpp"
#include "containers/uuid.hpp"
#include "logger.hpp"
#include "utils.hpp"

const std::string connectivity_cluster_t::cluster_proto_header("RethinkDB cluster\n");
const std::string connectivity_cluster_t::cluster_version(RETHINKDB_CODE_VERSION);

#if defined (__x86_64__)
const std::string connectivity_cluster_t::cluster_arch_bitsize("64bit");
#elif defined (__i386__)
const std::string connectivity_cluster_t::cluster_arch_bitsize("32bit");
#else
#error "Could not determine architecture"
#endif

#if defined (NDEBUG)
const std::string connectivity_cluster_t::cluster_build_mode("release");
#else
const std::string connectivity_cluster_t::cluster_build_mode("debug");
#endif

void debug_print(printf_buffer_t *buf, const peer_address_t &address) {
    buf->appendf("peer_address{ips=[");
    const std::set<ip_address_t> *ips = address.all_ips();
    for (std::set<ip_address_t>::const_iterator
             it = ips->begin(); it != ips->end(); ++it) {
        if (it != ips->begin()) buf->appendf(", ");
        debug_print(buf, *it);
    }
    buf->appendf("], port=%d}", address.port);
}


connectivity_cluster_t::run_t::run_t(connectivity_cluster_t *p,
                                     const std::set<ip_address_t> &local_addresses,
                                     int port,
                                     message_handler_t *mh,
                                     int client_port,
                                     heartbeat_manager_t *_heartbeat_manager) THROWS_ONLY(address_in_use_exc_t) :
    parent(p),
    message_handler(mh),
    heartbeat_manager(_heartbeat_manager),

    /* Create the socket to use when listening for connections from peers */
    cluster_listener_socket(new tcp_bound_socket_t(local_addresses, port)),
    cluster_listener_port(cluster_listener_socket->get_port()),

    /* The local port to use when connecting to the cluster port of peers */
    cluster_client_port(client_port),

    /* This sets `parent->current_run` to `this`. It's necessary to do it in the
    constructor of a subfield rather than in the body of the `run_t` constructor
    because `parent->current_run` needs to be set before `connection_to_ourself`
    is constructed. Otherwise, something could try to send a message to ourself
    in response to a connection notification from the constructor for
    `connection_to_ourself`, and that would be a problem. */
    register_us_with_parent(&parent->current_run, this),

    /* This constructor makes an entry for us in `routing_table`. The destructor
    will remove the entry. If the set of local addresses passed in is empty, it
    means that we bind to all local addresses.  That also means we need to get
    a new set of all local addresses from get_local_addresses() in that case. */
    routing_table_entry_for_ourself(&routing_table,
                                    parent->me,
                                    peer_address_t(local_addresses.empty() ? ip_address_t::get_local_addresses(std::set<ip_address_t>(), true) : local_addresses,
                                                   cluster_listener_socket->get_port())),

    /* The `connection_entry_t` constructor takes care of putting itself in the
    `connection_map` on each thread and notifying any listeners that we're now
    connected to ourself. The destructor will remove us from the
    `connection_map` and again notify any listeners. */
    connection_to_ourself(this, parent->me, NULL, routing_table[parent->me]),

    listener(new tcp_listener_t(cluster_listener_socket.get(),
                                boost::bind(&connectivity_cluster_t::run_t::on_new_connection,
                                            this, _1, auto_drainer_t::lock_t(&drainer))))
{
    parent->assert_thread();
}

connectivity_cluster_t::run_t::~run_t() { }

int connectivity_cluster_t::run_t::get_port() {
    return cluster_listener_port;
}

void connectivity_cluster_t::run_t::join(peer_address_t address) THROWS_NOTHING {
    parent->assert_thread();
    coro_t::spawn_now_dangerously(boost::bind(
        &connectivity_cluster_t::run_t::join_blocking,
        this,
        address,
        /* We don't know what `peer_id_t` the peer has until we connect to it */
        boost::none,
        auto_drainer_t::lock_t(&drainer)));
}

connectivity_cluster_t::run_t::connection_entry_t::connection_entry_t(run_t *p, peer_id_t id, tcp_conn_stream_t *c, peer_address_t a) THROWS_NOTHING :
    conn(c), address(a), session_id(generate_uuid()),
    pm_collection(),
    pm_bytes_sent(secs_to_ticks(1), true),
    pm_collection_membership(&p->parent->connectivity_collection, &pm_collection, uuid_to_str(id.get_uuid())),
    pm_bytes_sent_membership(&pm_collection, &pm_bytes_sent, "bytes_sent"),
    parent(p), peer(id),
    entries(new one_per_thread_t<entry_installation_t>(this)) {
    if (peer != parent->parent->me && parent->heartbeat_manager != NULL) {
        parent->heartbeat_manager->begin_peer_heartbeat(peer);
    }
}

connectivity_cluster_t::run_t::connection_entry_t::~connection_entry_t() THROWS_NOTHING {
    if (peer != parent->parent->me && parent->heartbeat_manager != NULL) {
        parent->heartbeat_manager->end_peer_heartbeat(peer);
    }

    // Delete entries early just so we can make the assertion below.
    entries.reset();

    /* `~entry_installation_t` destroys the `auto_drainer_t`'s in entries,
    so nothing can be holding the `send_mutex`. */
    guarantee(!send_mutex.is_locked());
}

static void ping_connection_watcher(peer_id_t peer, peers_list_callback_t *connect_disconnect_cb) THROWS_NOTHING {
    rassert(connect_disconnect_cb != NULL);
    connect_disconnect_cb->on_connect(peer);
}

static void ping_disconnection_watcher(peer_id_t peer, peers_list_callback_t *connect_disconnect_cb) THROWS_NOTHING {
    rassert(connect_disconnect_cb != NULL);
    connect_disconnect_cb->on_disconnect(peer);
}

connectivity_cluster_t::run_t::connection_entry_t::entry_installation_t::entry_installation_t(connection_entry_t *that) : that_(that) {
    thread_info_t *ti = that_->parent->parent->thread_info.get();
    {
        ASSERT_FINITE_CORO_WAITING;
        rwi_lock_assertion_t::write_acq_t acq(&ti->lock);

        std::pair<std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator, bool>
            res = ti->connection_map.insert(std::make_pair(that_->peer,
                                                           std::make_pair(that_, auto_drainer_t::lock_t(&drainer_))));
        guarantee(res.second, "Map entry was not present.");

        ti->publisher.publish(boost::bind(&ping_connection_watcher, that_->peer, _1));
    }
}

connectivity_cluster_t::run_t::connection_entry_t::entry_installation_t::~entry_installation_t() {
    thread_info_t *ti = that_->parent->parent->thread_info.get();
    {
        ASSERT_FINITE_CORO_WAITING;
        rwi_lock_assertion_t::write_acq_t acq(&ti->lock);

        std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator entry
            = ti->connection_map.find(that_->peer);
        guarantee(entry != ti->connection_map.end() && entry->second.first == that_);
        ti->connection_map.erase(that_->peer);
        ti->publisher.publish(boost::bind(&ping_disconnection_watcher, that_->peer, _1));
    }
}

void connectivity_cluster_t::run_t::on_new_connection(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn, auto_drainer_t::lock_t lock) THROWS_NOTHING {
    parent->assert_thread();

    // conn gets owned by the tcp_conn_stream_t.
    tcp_conn_t *conn;
    nconn->make_overcomplicated(&conn);
    keepalive_tcp_conn_stream_t conn_stream(conn);

    handle(&conn_stream, boost::none, boost::none, lock, NULL);
}

void connectivity_cluster_t::run_t::connect_to_peer(const peer_address_t *addr, int index, boost::optional<peer_id_t> expected_id, auto_drainer_t::lock_t drainer_lock, bool *successful_join, semaphore_t *rate_control) THROWS_NOTHING {
    // Wait to start the connection attempt, max time is one second per address
    signal_timer_t timeout(index * 1000);
    try {
        wait_any_t interrupt(&timeout, drainer_lock.get_drain_signal());
        rate_control->co_lock_interruptible(&interrupt);
    } catch (const interrupted_exc_t &) {
        // Stop if interrupted externally, keep going if we timed out waiting
        if (drainer_lock.get_drain_signal()->is_pulsed()) {
            return;
        }
        rassert(timeout.is_pulsed());
    }

    // Indexing through a std::set is rather awkward
    const std::set<ip_address_t> *all_ips = addr->all_ips();
    std::set<ip_address_t>::const_iterator selected_addr;
    for (selected_addr = all_ips->begin(); selected_addr != all_ips->end() && index > 0; ++selected_addr, --index);
    guarantee(index == 0);

    // Don't bother if there's already a connection
    if (!*successful_join) {
        try {
            keepalive_tcp_conn_stream_t conn(*selected_addr, addr->port, drainer_lock.get_drain_signal(), cluster_client_port);
            if (!*successful_join) {
                handle(&conn, expected_id, boost::optional<peer_address_t>(*addr), drainer_lock, successful_join);
            }
        } catch (const tcp_conn_t::connect_failed_exc_t &) {
            /* Ignore */
        } catch (const interrupted_exc_t &) {
            /* Ignore */
        }
    }

    // Allow the next address attempt to run
    rate_control->unlock(1);
}

void connectivity_cluster_t::run_t::join_blocking(
        peer_address_t address,
        boost::optional<peer_id_t> expected_id,
        auto_drainer_t::lock_t drainer_lock) THROWS_NOTHING {
    drainer_lock.assert_is_holding(&parent->current_run->drainer);
    parent->assert_thread();
    {
        mutex_assertion_t::acq_t acq(&attempt_table_mutex);
        if (attempt_table.find(address) != attempt_table.end()) {
            return;
        }
        attempt_table.insert(address);
    }

    // Attempt to connect to all known ip addresses of the peer
    bool successful_join = false; // Variable so that handle() can check that only one connection succeeds
    semaphore_t rate_control(address.all_ips()->size()); // Mutex to control the rate that connection attempts are made
    rate_control.co_lock(address.all_ips()->size() - 1); // Start with only one coroutine able to run

    pmap(address.all_ips()->size(), boost::bind(&connectivity_cluster_t::run_t::connect_to_peer, this, &address, _1, expected_id, drainer_lock, &successful_join, &rate_control));

    // All attempts have completed
    {
        mutex_assertion_t::acq_t acq(&attempt_table_mutex);
        attempt_table.erase(address);
    }
}

class cluster_conn_closing_subscription_t : public signal_t::subscription_t {
public:
    explicit cluster_conn_closing_subscription_t(tcp_conn_stream_t *conn) : conn_(conn) { }

    virtual void run() {
        if (conn_->is_read_open()) {
            conn_->shutdown_read();
        }
        if (conn_->is_write_open()) {
            conn_->shutdown_write();
        }
    }
private:
    tcp_conn_stream_t *conn_;
    DISABLE_COPYING(cluster_conn_closing_subscription_t);
};

class heartbeat_keepalive_t : public keepalive_tcp_conn_stream_t::keepalive_callback_t,
                              public heartbeat_manager_t::heartbeat_keepalive_tracker_t {
public:
    heartbeat_keepalive_t(keepalive_tcp_conn_stream_t *_conn, heartbeat_manager_t *_heartbeat, peer_id_t _peer) :
        conn(_conn),
        heartbeat(_heartbeat),
        peer(_peer)
    {
        rassert(conn != NULL);
        rassert(heartbeat != NULL);
        conn->set_keepalive_callback(this);
        heartbeat->set_keepalive_tracker(peer, this);
    }

    ~heartbeat_keepalive_t() {
        conn->set_keepalive_callback(NULL);
        heartbeat->set_keepalive_tracker(peer, NULL);
    }

    void keepalive_read() {
        read_done = true;
    }

    void keepalive_write() {
        write_done = true;
    }

    bool check_and_reset_reads() {
        bool result = read_done;
        read_done = false;
        return result;
    }

    bool check_and_reset_writes() {
        bool result = write_done;
        write_done = false;
        return result;
    }

private:
    keepalive_tcp_conn_stream_t * const conn;
    heartbeat_manager_t * const heartbeat;
    const peer_id_t peer;
    bool read_done;
    bool write_done;

    DISABLE_COPYING(heartbeat_keepalive_t);
};

// Error-handling helper for connectivity_cluster_t::run_t::handle(). Returns true if handle()
// should return.
template<typename T>
static bool deserialize_and_check(tcp_conn_stream_t *c, T *p, const char *peer) {
    archive_result_t res = deserialize(c, p);
    switch (res) {
      case ARCHIVE_SUCCESS: return false; // no problem.

        // Network error. Report nothing.
      case ARCHIVE_SOCK_ERROR: case ARCHIVE_SOCK_EOF: return true;

      case ARCHIVE_RANGE_ERROR:
        logERR("could not deserialize data received from %s, closing connection", peer);
        return true;

      default: case ARCHIVE_GENERIC_ERROR:
        logERR("unknown error occurred on connection from %s, closing connection", peer);
        return true;
    }
}

bool is_similar_peer_address(const peer_address_t &left, const peer_address_t &right) {
    bool left_loopback_only = true;
    bool right_loopback_only = true;

    if (left.port != right.port) {
        return false;
    }

    // We ignore any loopback addresses because they don't give us any useful information
    // Return true if any non-loopback addresses match
    for (std::set<ip_address_t>::iterator left_it = left.all_ips()->begin();
         left_it != left.all_ips()->end(); ++left_it) {
        if (left_it->is_loopback()) {
            continue;
        } else {
            left_loopback_only = false;
        }

        for (std::set<ip_address_t>::iterator right_it = right.all_ips()->begin();
             right_it != right.all_ips()->end(); ++right_it) {
            if (right_it->is_loopback()) {
                continue;
            } else {
                right_loopback_only = false;
            }

            if (*right_it == *left_it) {
                return true;
            }
        }
    }

    // No non-loopback addresses matched, return true if either side was *only* loopback addresses
    //  because we can't easily prove if they are the same or different addresses
    return left_loopback_only || right_loopback_only;
}

// We log error conditions as follows:
// - silent: network error; conflict between parallel connections
// - warning: invalid header
// - error: id or address don't match expected id or address; deserialization range error; unknown error
// In all cases we close the connection and quit.
void connectivity_cluster_t::run_t::handle(
        /* `conn` should remain valid until `handle()` returns.
         * `handle()` does not take ownership of `conn`. */
        keepalive_tcp_conn_stream_t *conn,
        boost::optional<peer_id_t> expected_id,
        boost::optional<peer_address_t> expected_address,
        auto_drainer_t::lock_t drainer_lock,
        bool *successful_join) THROWS_NOTHING
{
    parent->assert_thread();

    // Get the name of our peer, for error reporting.
    ip_address_t peer_addr;
    std::string peerstr = "(unknown)";
    if (!conn->get_underlying_conn()->getpeername(&peer_addr))
        peerstr = peer_addr.as_dotted_decimal();
    const char *peername = peerstr.c_str();

    // Make sure that if we're ordered to shut down, any pending read
    // or write gets interrupted.
    cluster_conn_closing_subscription_t conn_closer_1(conn);
    conn_closer_1.reset(drainer_lock.get_drain_signal());

    // Each side sends a header followed by its own ID and address, then receives and checks the
    // other side's.
    {
        write_message_t msg;
        msg.append(cluster_proto_header.c_str(), cluster_proto_header.length());
        msg << cluster_version;
        msg << cluster_arch_bitsize;
        msg << cluster_build_mode;
        msg << parent->me;
        msg << routing_table[parent->me];
        if (send_write_message(conn, &msg))
            return; // network error.
    }

    // Receive & check header.
    {
        const int64_t buffer_size = 64;
        char buffer[buffer_size];
        int64_t r;
        for (uint64_t i = 0; i < cluster_proto_header.length(); i += r) {
            r = conn->read(buffer, std::min(buffer_size, int64_t(cluster_proto_header.length() - i)));
            if (-1 == r)
                return; // network error.
            rassert(r >= 0);
            // If EOF or remote_header does not match header, terminate connection.
            if (0 == r || memcmp(cluster_proto_header.c_str() + i, buffer, r) != 0) {
                logWRN("Received invalid clustering header from %s, closing connection -- something might be connecting to the wrong port.", peername);
                return;
            }
        }
    }

    {
        std::string remote_version;
        std::string remote_arch_bitsize;
        std::string remote_build_mode;

        if (deserialize_and_check(conn, &remote_version, peername) ||
            deserialize_and_check(conn, &remote_arch_bitsize, peername),
            deserialize_and_check(conn, &remote_build_mode, peername))
            return;

        if (remote_version != cluster_version) {
            logWRN("Connection attempt with a RethinkDB node of the wrong version,"
                   " local version: %s, remote version: %s, connection dropped\n",
                   cluster_version.c_str(), remote_version.c_str());
            return;
        }

        if (remote_arch_bitsize != cluster_arch_bitsize) {
            logWRN("Connection attempt with a RethinkDB node of the wrong architecture,"
                   " local: %s, remote: %s, connection dropped\n",
                   cluster_arch_bitsize.c_str(), remote_arch_bitsize.c_str());
            return;
        }

        if (remote_build_mode != cluster_build_mode) {
            logWRN("Connection attempt with a RethinkDB node of the wrong build mode,"
                   " local: %s, remote: %s, connection dropped\n",
                   cluster_build_mode.c_str(), remote_build_mode.c_str());
            return;
        }
    }

    // Receive id, address.
    peer_id_t other_id;
    peer_address_t other_address;
    if (deserialize_and_check(conn, &other_id, peername) ||
        deserialize_and_check(conn, &other_address, peername))
        return;

    /* Sanity checks */
    if (other_id == parent->me) {
        // TODO: report this on command-line in some cases. see issue 546 on github.
        return;
    }
    if (other_id.is_nil()) {
        logERR("received nil peer id from %s, closing connection", peername);
        return;
    }
    if (expected_id && other_id != *expected_id) {
        // This is only a problem if we're not using a loopback address
        if (!peer_addr.is_loopback()) {
            logERR("received inconsistent routing information (wrong ID) from %s, closing connection", peername);
        }
        return;
    }
    if (expected_address && !is_similar_peer_address(other_address, *expected_address)) {
        printf_buffer_t buf;
        buf.appendf("expected_address = ");
        debug_print(&buf, *expected_address);
        buf.appendf(", other_address = ");
        debug_print(&buf, other_address);

        logERR("received inconsistent routing information (wrong address) from %s (%s), closing connection", peername, buf.c_str());
        return;
    }

    // Just saying that we're still on the rpc listener thread.
    parent->assert_thread();

    /* The trickiest case is when there are two or more parallel connections
    that are trying to be established between the same two machines. We can get
    this when e.g. machine A and machine B try to connect to each other at the
    same time. It's important that exactly one of the connections actually gets
    established. When there are multiple connections trying to be established,
    this is referred to as a "conflict". */

    object_buffer_t<map_insertion_sentry_t<peer_id_t, peer_address_t> >
        routing_table_entry_sentry;

    /* We pick one side of the connection to be the "leader" and the other side
    to be the "follower". These roles are only relevant in the initial startup
    process. The leader registers the connection locally. If there's a conflict,
    it drops the connection. If not, it sends its routing table to the follower.
    Then the follower registers itself locally. There shouldn't be a conflict
    because any duplicate connection would have been detected by the leader.
    Then the follower sends its routing table to the leader. */
    bool we_are_leader = parent->me < other_id;

    // Just saying: Still on rpc listener thread, for
    // sending/receiving routing table
    parent->assert_thread();
    std::map<peer_id_t, peer_address_t> other_routing_table;

    if (we_are_leader) {

        std::map<peer_id_t, peer_address_t> routing_table_to_send;

        /* Critical section: we must check for conflicts and register ourself
        without the interference of any other connections. This ensures that
        any conflicts are resolved consistently. It also ensures that if we get
        two connections from different nodes, one will find out about the other.
        */
        {
            mutex_t::acq_t acq(&new_connection_mutex);

            if (routing_table.find(other_id) != routing_table.end()) {
                /* Conflict! Abort! Terminate the connection unceremoniously;
                the follower will find out. */
                return;
            }

            /* Make a copy of `routing_table` before exiting the critical
            section */
            routing_table_to_send = routing_table;

            /* Register ourselves while in the critical section, so that whoever
            comes next will see us */
            routing_table_entry_sentry.create(&routing_table, other_id, other_address);
        }

        /* We're good to go! Transmit the routing table to the follower, so it
        knows we're in. */
        {
            write_message_t msg;
            msg << routing_table_to_send;
            if (send_write_message(conn, &msg))
                return;         // network error
        }

        /* Receive the follower's routing table */
        if (deserialize_and_check(conn, &other_routing_table, peername))
            return;

    } else {

        /* Receive the leader's routing table. (If our connection has lost a
        conflict, then the leader will close the connection instead of sending
        the routing table. */
        if (deserialize_and_check(conn, &other_routing_table, peername))
            return;

        std::map<peer_id_t, peer_address_t> routing_table_to_send;

        /* Register ourselves locally. This is in a critical section so that if
        we get two connections from different nodes at the same time, one will
        find out about the other. */
        {
            mutex_t::acq_t acq(&new_connection_mutex);

            // Here's how this situation can happen:
            // 1. We are connected to another node.
            // 2. The connection is interrupted.
            // 3. The other node gives up on the original TCP connection, but we have not given up on it yet.
            // 4. The other node tries to reconnect, and the new TCP connection gets through and this node ends up here.
            // 5. We now have a duplicate connection to the other node.
            if (routing_table.find(other_id) != routing_table.end()) {
                // In this case, just exit this function, which will close the connection
                // This will happen until the old connection dies
                // TODO: ensure that the old connection shuts down?
                return;
            }

            /* Make a copy of `routing_table` before exiting the critical
            section */
            routing_table_to_send = routing_table;

            /* Register ourselves while in the critical section, so that whoever
            comes next will see us */
            routing_table_entry_sentry.create(&routing_table, other_id, other_address);
        }

        /* Send our routing table to the leader */
        {
            write_message_t msg;
            msg << routing_table_to_send;
            if (send_write_message(conn, &msg))
                return;         // network error
        }
    }

    // Just saying: We haven't left the RPC listener thread.
    parent->assert_thread();

    // This check is so that when trying multiple connections to a peer in parallel, we can
    //  make sure only one of them succeeds
    if (successful_join != NULL) {
        if (*successful_join) {
            logWRN("Somehow ended up with two successful joins to a peer, closing one");
            return;
        }
        *successful_join = true;
    }

    /* For each peer that our new friend told us about that we don't already
    know about, start a new connection. If the cluster is shutting down, skip
    this step. */
    if (!drainer_lock.get_drain_signal()->is_pulsed()) {
        for (std::map<peer_id_t, peer_address_t>::iterator it = other_routing_table.begin();
             it != other_routing_table.end(); it++) {
            if (routing_table.find(it->first) == routing_table.end()) {
                /* `it->first` is the ID of a peer that our peer is connected
                to, but we aren't connected to. */
                coro_t::spawn_now_dangerously(boost::bind(
                    &connectivity_cluster_t::run_t::join_blocking, this,
                    it->second,
                    boost::optional<peer_id_t>(it->first),
                    drainer_lock));
            }
        }
    }

    /* Now that we're about to switch threads, it's not safe to try to close
    the connection from this thread anymore. This is safe because we won't do
    anything that permanently blocks before setting up `conn_closer_2`. */
    conn_closer_1.reset();

    // We could pick a better way to pick a better thread, our choice
    // now is hopefully a performance non-problem.
    int chosen_thread = rng.randint(get_num_threads());

    cross_thread_signal_t connection_thread_drain_signal(drainer_lock.get_drain_signal(), chosen_thread);

    rethread_tcp_conn_stream_t unregister_conn(conn, INVALID_THREAD);
    on_thread_t conn_threader(chosen_thread);
    rethread_tcp_conn_stream_t reregister_conn(conn, get_thread_id());

    // Make sure that if we're ordered to shut down, any pending read
    // or write gets interrupted.
    cluster_conn_closing_subscription_t conn_closer_2(conn);
    conn_closer_2.reset(&connection_thread_drain_signal);

    {
        /* `connection_entry_t` is the public interface of this coroutine. Its
        constructor registers it in the `connectivity_cluster_t`'s connection
        map and notifies any connect listeners. */
        connection_entry_t conn_structure(this, other_id, conn, other_address);
        object_buffer_t<heartbeat_keepalive_t> keepalive;

        if (heartbeat_manager != NULL) {
            keepalive.create(conn, heartbeat_manager, other_id);
        }

        /* Main message-handling loop: read messages off the connection until
        it's closed, which may be due to network events, or the other end
        shutting down, or us shutting down. */
        try {
            while (true) {
                /* For now, we use `std::string` for messages on the wire: it's
                just a length and a byte vector. This is obviously slow and we
                should change it when we care about performance. */
                std::string message;
                if (deserialize_and_check(conn, &message, peername))
                    break;

                std::vector<char> vec(message.begin(), message.end());
                vector_read_stream_t stream(&vec);
                message_handler->on_message(other_id, &stream); // might raise fake_archive_exc_t
            }
        } catch (const fake_archive_exc_t &) {
            /* The exception broke us out of the loop, and that's what we
            wanted. This could either be because we lost contact with the peer
            or because the cluster is shutting down and `close_conn()` got
            called. */
        }

        guarantee(!conn->is_read_open(), "the connection is still open for "
            "read, which means we had a problem other than the TCP "
            "connection closing or dying");

        /* The `conn_structure` destructor removes us from the connection map
        and notifies any disconnect listeners. */
    }
}

connectivity_cluster_t::connectivity_cluster_t() THROWS_NOTHING :
    me(peer_id_t(generate_uuid())),
    current_run(NULL),
    connectivity_collection(),
    stats_membership(&get_global_perfmon_collection(), &connectivity_collection, "connectivity")
    { }

connectivity_cluster_t::~connectivity_cluster_t() THROWS_NOTHING {
    guarantee(!current_run);
}

peer_id_t connectivity_cluster_t::get_me() THROWS_NOTHING {
    return me;
}

std::set<peer_id_t> connectivity_cluster_t::get_peers_list() THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::set<peer_id_t> peers;
    for (std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::const_iterator it = connection_map->begin();
            it != connection_map->end(); it++) {
        peers.insert(it->first);
    }
    return peers;
}

uuid_u connectivity_cluster_t::get_connection_session_id(peer_id_t peer) THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator it =
        connection_map->find(peer);
    guarantee(it != connection_map->end(), "You're trying to access the session "
        "ID for an unconnected peer. Note that we are not considered to be "
        "connected to ourself until after a connectivity_cluster_t::run_t "
        "has been created.");
    return it->second.first->session_id;
}

connectivity_service_t *connectivity_cluster_t::get_connectivity_service() THROWS_NOTHING {
    /* This is kind of silly. We need to implement it because
    `message_service_t` has a `get_connectivity_service()` method, and we are
    also the `connectivity_service_t` for our own `message_service_t`. */
    return this;
}

void connectivity_cluster_t::send_message(peer_id_t dest, send_message_write_callback_t *callback) THROWS_NOTHING {
    // We could be on _any_ thread.

    guarantee(!dest.is_nil());

    /* We currently write the message to a vector_stream_t, then
       serialize that as a string. It's horribly inefficient, of course. */
    // TODO: If we don't do it this way, we (or the caller) will need
    // to worry about having the writer run on the connection thread.
    vector_stream_t buffer;
    {
        ASSERT_FINITE_CORO_WAITING;
        callback->write(&buffer);
    }

#ifdef CLUSTER_MESSAGE_DEBUGGING
    {
        printf_buffer_t buf;
        buf.appendf("from ");
        debug_print(&buf, me);
        buf.appendf(" to ");
        debug_print(&buf, dest);
        buf.appendf("\n");
        fprintf(stderr, "%s", buf.c_str());
        print_hd(buffer.vector().data(), 0, buffer.vector().size());
    }
#endif

#ifndef NDEBUG
    /* We're allowed to block indefinitely, but it's tempting to write code on
    the assumption that we won't. This might catch some programming errors. */
    if (debug_rng.randint(10) == 0) {
        nap(10);
    }
#endif

    /* Find the connection entry */
    run_t::connection_entry_t *conn_structure;
    auto_drainer_t::lock_t conn_structure_lock;
    {
        std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
            &thread_info.get()->connection_map;
        std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::const_iterator it =
            connection_map->find(dest);
        if (it == connection_map->end()) {
            /* We don't currently have access to this peer. Our policy is to not
            notify the sender when a message cannot be transmitted (since this
            is not always possible). So just return. */
            return;
        }
        conn_structure = it->second.first;
        conn_structure_lock = it->second.second;
    }

    if (conn_structure->conn == NULL) {
        // We're sending a message to ourself
        guarantee(dest == me);
        // We could be on any thread here! Oh no!
        vector_read_stream_t buffer2(&buffer.vector());
        current_run->message_handler->on_message(me, &buffer2);
        conn_structure->pm_bytes_sent.record(buffer.vector().size());

    } else {
        guarantee(dest != me);
        on_thread_t threader(conn_structure->conn->home_thread());

        /* Acquire the send-mutex so we don't collide with other things trying
        to send on the same connection. */
        mutex_t::acq_t acq(&conn_structure->send_mutex);

        {
            write_message_t msg;
            std::string buffer_str(buffer.vector().begin(), buffer.vector().end());
            msg << buffer_str;
            int res = send_write_message(conn_structure->conn, &msg);
            conn_structure->pm_bytes_sent.record(buffer.vector().size());
            if (res) {
                /* Close the other half of the connection to make sure that
                   `connectivity_cluster_t::run_t::handle()` notices that something is
                   up */
                if (conn_structure->conn->is_read_open()) {
                    conn_structure->conn->shutdown_read();
                }
            }
        }
    }
}

void connectivity_cluster_t::kill_connection(peer_id_t peer) THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator it =
        connection_map->find(peer);

    if (it != connection_map->end()) {
        tcp_conn_stream_t *conn = it->second.first->conn;
        guarantee(get_thread_id() == conn->home_thread());

        if (conn->is_read_open()) {
            conn->shutdown_read();
        }
        if (conn->is_write_open()) {
            conn->shutdown_write();
        }
    }
}

peer_address_t connectivity_cluster_t::get_peer_address(peer_id_t p) THROWS_NOTHING {
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > *connection_map =
        &thread_info.get()->connection_map;
    std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> >::iterator it =
        connection_map->find(p);
    guarantee(it != connection_map->end(), "You can only call get_peer_address() "
        "on a peer that we're currently connected to. Note that we're not "
        "considered to be connected to ourself until after the "
        "connectivity_cluster_t::run_t has been constructed.");
    return it->second.first->address;
}

rwi_lock_assertion_t *connectivity_cluster_t::get_peers_list_lock() THROWS_NOTHING {
    return &thread_info.get()->lock;
}

publisher_t<peers_list_callback_t *> *connectivity_cluster_t::get_peers_list_publisher() THROWS_NOTHING {
    return thread_info.get()->publisher.get_publisher();
}
