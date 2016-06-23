// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rpc/connectivity/cluster.hpp"

#ifndef _WIN32
#include <netinet/in.h>
#endif

#include <algorithm>
#include <functional>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "arch/io/network.hpp"
#include "arch/timing.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/semaphore.hpp"
#include "config/args.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "containers/object_buffer.hpp"
#include "containers/uuid.hpp"
#include "logger.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"

// Number of messages after which the message handling loop yields
#define MESSAGE_HANDLER_MAX_BATCH_SIZE           16

// The cluster communication protocol version.
static_assert(cluster_version_t::CLUSTER == cluster_version_t::v2_3_is_latest,
              "We need to update CLUSTER_VERSION_STRING when we add a new cluster "
              "version.");

#define CLUSTER_VERSION_STRING "2.3.0"

const std::string connectivity_cluster_t::cluster_proto_header("RethinkDB cluster\n");
const std::string connectivity_cluster_t::cluster_version_string(CLUSTER_VERSION_STRING);

// Returns true and sets *out to the version number, if the version number in
// version_string is a recognized version and the same or earlier than our version.
static bool version_number_recognized_compatible(const std::string &version_string,
                                                 cluster_version_t *out) {
    // Right now, we only support one cluster version -- ours.
    if (version_string == CLUSTER_VERSION_STRING) {
        *out = cluster_version_t::CLUSTER;
        return true;
    }
    return false;
}

// Returns false if the string is not a valid version string (matching /\d+(\.\d+)*/)
static bool split_version_string(const std::string &version_string,
                                 std::vector<int64_t> *out) {
    const std::vector<std::string> parts = split_string(version_string, '.');
    std::vector<int64_t> ret(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        if (!strtoi64_strict(parts[i], 10, &ret[i])) {
            return false;
        }
    }
    *out = std::move(ret);
    return true;
}

// Returns true if the version string is recognized as a _greater_ version string
// (than our software's connectivity_cluster_t::cluster_version_string).  Returns
// false for unparseable version strings (see split_version_string) or lesser or
// equal version strings.
static bool version_number_unrecognized_greater(const std::string &version_string) {
    std::vector<int64_t> parts;
    if (!split_version_string(version_string, &parts)) {
        return false;
    }

    std::vector<int64_t> our_parts;
    const bool success = split_version_string(
            connectivity_cluster_t::cluster_version_string,
            &our_parts);
    guarantee(success);
    return std::lexicographical_compare(our_parts.begin(), our_parts.end(),
                                        parts.begin(), parts.end());
}

// Given a remote version string, we figure out whether we can try to talk to it, and
// if we can, what version we shall talk on.
static bool resolve_protocol_version(const std::string &remote_version_string,
                                     cluster_version_t *out) {
    if (version_number_recognized_compatible(remote_version_string, out)) {
        return true;
    }
    if (version_number_unrecognized_greater(remote_version_string)) {
        static_assert(cluster_version_t::CLUSTER == cluster_version_t::LATEST_OVERALL,
                      "If you've made CLUSTER != LATEST_OVERALL, presumably you know "
                      "how to change this code.");
        *out = cluster_version_t::CLUSTER;
        return true;
    }
    return false;
}

#if defined (__x86_64__) || defined (_WIN64)
const std::string connectivity_cluster_t::cluster_arch_bitsize("64bit");
#elif defined (__i386__) || defined(__arm__) || defined(_WIN32)
const std::string connectivity_cluster_t::cluster_arch_bitsize("32bit");
#else
#error "Could not determine architecture"
#endif

#if defined (NDEBUG)
const std::string connectivity_cluster_t::cluster_build_mode("release");
#else
const std::string connectivity_cluster_t::cluster_build_mode("debug");
#endif

void connectivity_cluster_t::connection_t::kill_connection() {
    /* `heartbeat_manager_t` assumes this doesn't block as long as it's called on the
    home thread. */
    guarantee(!is_loopback(), "Attempted to kill connection to myself.");
    on_thread_t thread_switcher(conn->home_thread());

    if (conn->is_read_open()) {
        conn->shutdown_read();
    }
    if (conn->is_write_open()) {
        conn->shutdown_write();
    }
}

connectivity_cluster_t::connection_t::connection_t(
        run_t *_parent,
        const peer_id_t &_peer_id,
        const server_id_t &_server_id,
        keepalive_tcp_conn_stream_t *_conn,
        const peer_address_t &_peer_address) THROWS_NOTHING :
    conn(_conn),
    peer_address(_peer_address),
    flusher([&](signal_t *) {
        guarantee(this->conn != nullptr);
        // We need to acquire the send_mutex because flushing the buffer
        // must not interleave with other writes (restriction of linux_tcp_conn_t).
        mutex_t::acq_t acq(&this->send_mutex);
        // We ignore the return value of flush_buffer(). Closed connections
        // must be handled elsewhere.
        this->conn->flush_buffer();
    }, 1),
    pm_collection(),
    pm_bytes_sent(secs_to_ticks(1), true),
    pm_collection_membership(
        &_parent->parent->connectivity_collection,
        &pm_collection,
        uuid_to_str(_peer_id.get_uuid())),
    pm_bytes_sent_membership(&pm_collection, &pm_bytes_sent, "bytes_sent"),
    parent(_parent),
    peer_id(_peer_id),
    server_id(_server_id),
    drainers()
{
    pmap(get_num_threads(), [this](int thread_id) {
        on_thread_t thread_switcher((threadnum_t(thread_id)));
        parent->parent->connections.get()->set_key_no_equals(
            peer_id,
            std::make_pair(this, auto_drainer_t::lock_t(drainers.get())));
    });
}

connectivity_cluster_t::connection_t::~connection_t() THROWS_NOTHING {
    // Drain out any users
    pmap(get_num_threads(), [this](int thread_id) {
        on_thread_t thread_switcher((threadnum_t(thread_id)));
        parent->parent->connections.get()->delete_key(peer_id);
        drainers.get()->drain();
    });

    /* The drainers have been destroyed, so nothing can be holding the `send_mutex`. */
    guarantee(!send_mutex.is_locked());
}

// Helper function for the `run_t` constructor's initialization list
static peer_address_t our_peer_address(std::set<ip_address_t> local_addresses,
                                       const peer_address_t &canonical_addresses,
                                       port_t cluster_port) {
    std::set<host_and_port_t> our_addrs;

    // If at least one canonical address was specified, we ignore all other addresses
    if (!canonical_addresses.hosts().empty()) {
        // We have to modify canonical addresses in case there is a port of 0
        //  use the real port from the socket
        for (auto it = canonical_addresses.hosts().begin();
             it != canonical_addresses.hosts().end(); ++it) {
            if (it->port().value() == 0) {
                our_addrs.insert(host_and_port_t(it->host(), cluster_port));
            } else {
                our_addrs.insert(*it);
            }
        }
    } else {
        // Otherwise we need to use the local addresses with the cluster port
        if (local_addresses.empty()) {
            local_addresses = get_local_ips(std::set<ip_address_t>(),
                                            local_ip_filter_t::ALL);
        }
        for (auto it = local_addresses.begin();
             it != local_addresses.end(); ++it) {
            our_addrs.insert(host_and_port_t(it->to_string(), cluster_port));
        }
    }
    return peer_address_t(our_addrs);
}

connectivity_cluster_t::run_t::run_t(
        connectivity_cluster_t *_parent,
        const server_id_t &_server_id,
        const std::set<ip_address_t> &local_addresses,
        const peer_address_t &canonical_addresses,
        const int join_delay_secs,
        int port,
        int client_port,
        boost::shared_ptr<semilattice_read_view_t<heartbeat_semilattice_metadata_t> >
            _heartbeat_sl_view,
        boost::shared_ptr<semilattice_read_view_t<auth_semilattice_metadata_t> >
            _auth_sl_view,
        tls_ctx_t *_tls_ctx)
        THROWS_ONLY(address_in_use_exc_t, tcp_socket_exc_t) :
    parent(_parent),
    server_id(_server_id),
    tls_ctx(_tls_ctx),

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
    a new set of all local addresses from get_local_ips() in that case. */
    routing_table_entry_for_ourself(
        &routing_table,
        parent->me,
        our_peer_address(local_addresses,
                         canonical_addresses,
                         port_t(cluster_listener_socket->get_port()))),

    /* The `connection_entry_t` constructor takes care of putting itself in the
    `connection_map` on each thread and notifying any listeners that we're now
    connected to ourself. The destructor will remove us from the
    `connection_map` and again notify any listeners. */
    connection_to_ourself(this, parent->me, _server_id, nullptr, routing_table[parent->me]),

    heartbeat_sl_view(_heartbeat_sl_view),
    auth_sl_view(_auth_sl_view),

    listener(new tcp_listener_t(
        cluster_listener_socket.get(),
        std::bind(&connectivity_cluster_t::run_t::on_new_connection,
                 this, ph::_1, join_delay_secs, auto_drainer_t::lock_t(&drainer))))
{
    parent->assert_thread();
}

connectivity_cluster_t::run_t::~run_t() {
    /* The member destructors take care of cutting off TCP connections, cleaning up, etc.
    */
}

std::set<host_and_port_t> connectivity_cluster_t::run_t::get_canonical_addresses() {
    parent->assert_thread();
    return routing_table.at(parent->me).hosts();
}

int connectivity_cluster_t::run_t::get_port() {
    return cluster_listener_port;
}

void connectivity_cluster_t::run_t::join(
        const peer_address_t &address,
        const int join_delay_secs) THROWS_NOTHING {
    parent->assert_thread();
    coro_t::spawn_now_dangerously(std::bind(
        &connectivity_cluster_t::run_t::join_blocking,
        this,
        address,
        /* We don't know what `peer_id_t` the peer has until we connect to it */
        boost::none,
        join_delay_secs,
        auto_drainer_t::lock_t(&drainer)));
}

void connectivity_cluster_t::run_t::on_new_connection(
        const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
        const int join_delay_secs,
        auto_drainer_t::lock_t lock) THROWS_NOTHING {
    parent->assert_thread();

    // conn gets owned by the keepalive_tcp_conn_stream_t.
    tcp_conn_t *conn;

    try {
        nconn->make_server_connection(tls_ctx, &conn, lock.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        // TLS handshake was interrupted.
        return;
    } catch (const crypto::openssl_error_t &err) {
        // TLS handshake failed.
        logERR("Cluster server connection TLS handshake failed: %s", err.what());
        return;
    }

    keepalive_tcp_conn_stream_t conn_stream(conn);

    handle(&conn_stream, boost::none, boost::none, lock, nullptr, join_delay_secs);
}

void connectivity_cluster_t::run_t::connect_to_peer(
        const peer_address_t *address,
        int index,
        boost::optional<peer_id_t> expected_id,
        auto_drainer_t::lock_t drainer_lock,
        bool *successful_join_inout,
        const int join_delay_secs,
        co_semaphore_t *rate_control) THROWS_NOTHING {
    // Wait to start the connection attempt, max time is one second per address
    signal_timer_t timeout;
    timeout.start(index * 1000);
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
    const std::set<ip_and_port_t> &all_addrs = address->ips();
    std::set<ip_and_port_t>::const_iterator selected_addr;
    for (selected_addr = all_addrs.begin();
         selected_addr != all_addrs.end() && index > 0;
         ++selected_addr, --index) { }
    guarantee(index == 0);

    // Don't bother if there's already a connection
    if (!*successful_join_inout) {
        try {
            keepalive_tcp_conn_stream_t conn(
                tls_ctx, selected_addr->ip(), selected_addr->port().value(),
                drainer_lock.get_drain_signal(), cluster_client_port);
            if (!*successful_join_inout) {
                handle(
                    &conn, expected_id, boost::optional<peer_address_t>(*address),
                    drainer_lock, successful_join_inout, join_delay_secs);
            }
        } catch (const tcp_conn_t::connect_failed_exc_t &) {
            /* Ignore */
        } catch (const crypto::openssl_error_t &) {
            /* Ignore */
        } catch (const interrupted_exc_t &) {
            /* Ignore */
        }
    }

    // Allow the next address attempt to run
    rate_control->unlock(1);
}

void connectivity_cluster_t::run_t::join_blocking(
        const peer_address_t peer,
        boost::optional<peer_id_t> expected_id,
        const int join_delay_secs,
        auto_drainer_t::lock_t drainer_lock) THROWS_NOTHING {
    drainer_lock.assert_is_holding(&parent->current_run->drainer);
    parent->assert_thread();
    {
        mutex_assertion_t::acq_t acq(&attempt_table_mutex);
        if (attempt_table.find(peer) != attempt_table.end()) {
            return;
        }
        attempt_table.insert(peer);
    }

    // Make sure the peer address isn't bogus
    guarantee(peer.ips().size() > 0);

    // Attempt to connect to all known ip addresses of the peer
    bool successful_join = false; // Variable so that handle() can check that only one connection succeeds
    static_semaphore_t rate_control(peer.ips().size()); // Mutex to control the rate that connection attempts are made
    rate_control.co_lock(peer.ips().size() - 1); // Start with only one coroutine able to run

    pmap(peer.ips().size(),
         std::bind(&connectivity_cluster_t::run_t::connect_to_peer,
                   this,
                   &peer,
                   ph::_1,
                   expected_id,
                   drainer_lock,
                   &successful_join,
                   join_delay_secs,
                   &rate_control));

    // All attempts have completed
    {
        mutex_assertion_t::acq_t acq(&attempt_table_mutex);
        attempt_table.erase(peer);
    }
}

class cluster_conn_closing_subscription_t : public signal_t::subscription_t {
public:
    explicit cluster_conn_closing_subscription_t(keepalive_tcp_conn_stream_t *conn) :
        conn_(conn) { }

    virtual void run() {
        if (conn_->is_read_open()) {
            conn_->shutdown_read();
        }
        if (conn_->is_write_open()) {
            conn_->shutdown_write();
        }
    }
private:
    keepalive_tcp_conn_stream_t *conn_;
    DISABLE_COPYING(cluster_conn_closing_subscription_t);
};

/* `heartbeat_manager_t` is responsible for sending heartbeats over a single connection
and making sure that heartbeats have arrived on time.
`connectivity_cluster_t::run_t::handle()` constructs one after constructing the
`connection_t`. */
class connectivity_cluster_t::heartbeat_manager_t :
    public keepalive_tcp_conn_stream_t::keepalive_callback_t,
    private repeating_timer_callback_t,
    private cluster_send_message_write_callback_t
{
public:
    static const int HEARTBEAT_TIMEOUT_INTERVALS = 5;

    heartbeat_manager_t(
            connectivity_cluster_t::connection_t *connection_,
            auto_drainer_t::lock_t connection_keepalive_,
            const std::string &peer_str_,
            clone_ptr_t<watchable_t<heartbeat_semilattice_metadata_t> >
                heartbeat_sl_view_) :
        connection(connection_),
        connection_keepalive(connection_keepalive_),
        read_done(false),
        write_done(false),
        intervals_since_last_read_done(0),
        peer_str(peer_str_),
        timeout(0),
        heartbeat_sl_view(std::move(heartbeat_sl_view_)),
        heartbeat_sl_view_sub(std::bind(&heartbeat_manager_t::on_heartbeat_change, this))
    {
        connection->conn->set_keepalive_callback(this);

        // This will trigger the initialization of the timeout, and timer
        watchable_t<heartbeat_semilattice_metadata_t>::freeze_t
            freeze(heartbeat_sl_view);
        heartbeat_sl_view_sub.reset(heartbeat_sl_view, &freeze);
    }

    ~heartbeat_manager_t() {
        connection->conn->set_keepalive_callback(nullptr);
    }

    /* These are called by the `keepalive_tcp_conn_stream_t`. */
    void keepalive_read() {
        read_done = true;
    }

    void keepalive_write() {
        write_done = true;
    }

    void on_ring() {
        ASSERT_FINITE_CORO_WAITING;

        if (intervals_since_last_read_done > HEARTBEAT_TIMEOUT_INTERVALS) {
            logERR("Heartbeat timeout, killing connection to peer %s", peer_str.c_str());

            /* This won't block if we call it from the same thread. This is an
            implementation detail that outside code shouldn't rely on, but since
            `heartbeat_manager_t` is part of `connectivity_cluster_t` it's OK. */
            connection->kill_connection();
            return;
        }
        if (write_done) {
            write_done = false;
        } else {
            /* The purpose of `heartbeat_manager_keepalive` is to ensure that we don't
            shut down while the heartbeat sending coroutine is still active */
            auto_drainer_t::lock_t this_keepalive(&drainer);
            coro_t::spawn_later_ordered(
                [this, this_keepalive /* important to capture */] {
                    /* This might block, so we have to run it in a sub-coroutine. */
                    connection->parent->parent->send_message(
                        connection, connection_keepalive,
                        connectivity_cluster_t::heartbeat_tag, this);
                });
        }
        if (read_done) {
            /* `intervals_since_last_read_done` may be negative when transitioning
               between timeouts, we should't reset it to zero when it's doing so. */
            if (intervals_since_last_read_done >= 0) {
                intervals_since_last_read_done = 0;
            } else {
                intervals_since_last_read_done++;
            }
            read_done = false;
        } else {
            intervals_since_last_read_done++;
        }
    }

    void write(write_stream_t *) {
        /* Do nothing. The cluster will end up sending just the tag 'H' with no message
        attached, which will trigger `keepalive_read()` on the remote server. */
    }

#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        return "heartbeat";
    }
#endif

    void on_heartbeat_change() {
        int64_t timeout_new = heartbeat_sl_view->get().heartbeat_timeout.get_ref();
        if (timeout == timeout_new) {
            /* The timeout hasn't changed, this could be due to the semilattice
               `versioned` changing without the value changing. */
            return;
        }

        if (timeout > timeout_new) {
            /* It may take the peer some time to transition to the new timeout, by
               setting `intervals_since_last_read_done` to a negative value we allow it
               to transition. */
            intervals_since_last_read_done = -timeout / timeout_new;
        } else {
            intervals_since_last_read_done = 0;
        }
        timeout = timeout_new;
        timer = scoped_ptr_t<repeating_timer_t>(new repeating_timer_t(
            timeout / HEARTBEAT_TIMEOUT_INTERVALS, this));
    }

private:
    connectivity_cluster_t::connection_t *connection;
    auto_drainer_t::lock_t connection_keepalive;
    bool read_done, write_done;
    int64_t intervals_since_last_read_done;
    std::string peer_str;
    int64_t timeout;

    /* Order is important here. When destroying the `heartbeat_manager_t`, we must first
    destroy the timer so that new `on_ring()` calls don't get spawned; then destroy the
    `auto_drainer_t` to drain any ongoing coroutines; and only then is it safe to destroy
    the other fields. */
    auto_drainer_t drainer;
    scoped_ptr_t<repeating_timer_t> timer;

    clone_ptr_t<watchable_t<heartbeat_semilattice_metadata_t> > heartbeat_sl_view;
    watchable_subscription_t<heartbeat_semilattice_metadata_t> heartbeat_sl_view_sub;

    DISABLE_COPYING(heartbeat_manager_t);
};

// Error-handling helper for connectivity_cluster_t::run_t::handle(). Returns true if
// handle() should return.
template <class T>
bool deserialize_and_check(cluster_version_t cluster_version,
                           keepalive_tcp_conn_stream_t *c, T *p, const char *peer) {
    archive_result_t res = deserialize_for_version(cluster_version, c, p);
    switch (res) {
    case archive_result_t::SUCCESS:
        return false;

        // Network error. Report nothing.
    case archive_result_t::SOCK_ERROR:
    case archive_result_t::SOCK_EOF:
        return true;

    case archive_result_t::RANGE_ERROR:
        logERR("could not deserialize data received from %s, closing connection", peer);
        return true;

    default:
        logERR("unknown error occurred on connection from %s, closing connection", peer);
        return true;
    }
}

template <class T>
bool deserialize_universal_and_check(keepalive_tcp_conn_stream_t *c,
                                     T *p, const char *peer) {
    archive_result_t res = deserialize_universal(c, p);
    switch (res) {
    case archive_result_t::SUCCESS:
        return false;

    case archive_result_t::SOCK_ERROR:
    case archive_result_t::SOCK_EOF:
        // Network error. Report nothing.
        return true;

    case archive_result_t::RANGE_ERROR:
        logERR("could not deserialize data received from %s, closing connection", peer);
        return true;

    default:
        logERR("unknown error occurred on connection from %s, closing connection", peer);
        return true;
    }
}

// Reads a chunk of data off of the connection, buffer must have at least 'size' bytes
//  available to write into
bool read_header_chunk(keepalive_tcp_conn_stream_t *conn, char *buffer, int64_t size,
        const char *peer) {
    int64_t r = conn->read(buffer, size);
    if (-1 == r) {
        logWRN("Network error while receiving clustering header from %s, closing connection.", peer);
        return false; // network error.
    }
    rassert(r >= 0);
    if (0 == r) {
        logWRN("Received incomplete clustering header from %s, closing connection.", peer);
        return false;
    }
    return true;
}

// Reads a uint64_t for size, then the string data
bool deserialize_compatible_string(keepalive_tcp_conn_stream_t *conn,
                                   std::string *str_out,
                                   const char *peer) {
    uint64_t raw_size;
    archive_result_t res = deserialize_universal(conn, &raw_size);
    if (res != archive_result_t::SUCCESS) {
        logWRN("Network error while receiving clustering header from %s, closing connection", peer);
        return false;
    }

    if (raw_size > 4096) {
        logWRN("Received excessive string size in header from peer %s, closing connection", peer);
        return false;
    }

    size_t size = raw_size;
    scoped_array_t<char> buffer(size);
    if (!read_header_chunk(conn, buffer.data(), size, peer)) {
        return false;
    }

    str_out->assign(buffer.data(), size);
    return true;
}

// Critical section: we must check for conflicts and register ourself
//  without the interference of any other connections. This ensures that
//  any conflicts are resolved consistently. It also ensures that if we get
//  two connections from different nodes, one will find out about the other.
bool connectivity_cluster_t::run_t::get_routing_table_to_send_and_add_peer(
        const peer_id_t &other_peer_id,
        const peer_address_t &other_peer_addr,
        object_buffer_t<map_insertion_sentry_t<peer_id_t, peer_address_t> > *routing_table_entry_sentry,
        std::map<peer_id_t, std::set<host_and_port_t> > *result) {
    mutex_t::acq_t acq(&new_connection_mutex);

    // Here's how this situation can happen:
    // 1. We are connected to another node.
    // 2. The connection is interrupted.
    // 3. The other node gives up on the original TCP connection, but we have not given up on it yet.
    // 4. The other node tries to reconnect, and the new TCP connection gets through and this node ends up here.
    // 5. We now have a duplicate connection to the other node.
    if (routing_table.find(other_peer_id) != routing_table.end()) {
        // In this case, just exit this function, which will close the connection
        // This will happen until the old connection dies
        // TODO: ensure that the old connection shuts down?
        return false;
    }

    // Make a serializable copy of `routing_table` before exiting the critical section
    result->clear();
    for (auto it = routing_table.begin(); it != routing_table.end(); ++it) {
        result->insert(std::make_pair(it->first, it->second.hosts()));
    }

    // Register ourselves while in the critical section, so that whoever comes next will see us
    routing_table_entry_sentry->create(&routing_table, other_peer_id, other_peer_addr);

    return true;
}

// You must update deserialize_universal(read_stream_t *, handshake_result_t *)
// below when changing this enum.
enum class handshake_result_code_t {
    SUCCESS = 0,
    UNRECOGNIZED_VERSION = 1,
    INCOMPATIBLE_ARCH = 2,
    INCOMPATIBLE_BUILD = 3,
    PASSWORD_MISMATCH = 4,
    UNKNOWN_ERROR = 5
};

class handshake_result_t {
public:
    handshake_result_t() { }
    static handshake_result_t success() {
        return handshake_result_t(handshake_result_code_t::SUCCESS);
    }
    static handshake_result_t error(handshake_result_code_t error_code,
                                    const std::string &additional_info) {
        return handshake_result_t(error_code, additional_info);
    }

    handshake_result_code_t get_code() const {
        return code;
    }

    std::string get_error_reason() const {
        if (code == handshake_result_code_t::UNKNOWN_ERROR) {
            return error_code_string + " (" + additional_info + ")";
        } else {
            return get_code_as_string() + " (" + additional_info + ")";
        }
    }

private:
    std::string get_code_as_string() const {
        switch (code) {
            case handshake_result_code_t::SUCCESS:
                return "success";
            case handshake_result_code_t::UNRECOGNIZED_VERSION:
                return "unrecognized or incompatible version";
            case handshake_result_code_t::INCOMPATIBLE_ARCH:
                return "incompatible architecture";
            case handshake_result_code_t::INCOMPATIBLE_BUILD:
                return "incompatible build mode";
            case handshake_result_code_t::PASSWORD_MISMATCH:
                return "no admin password";
            case handshake_result_code_t::UNKNOWN_ERROR:
                unreachable();
            default:
                unreachable();
        }
    }

    handshake_result_t(handshake_result_code_t _error_code,
                       const std::string &_additional_info)
        : code(_error_code), additional_info(_additional_info) {
        guarantee(code != handshake_result_code_t::UNKNOWN_ERROR);
        guarantee(code != handshake_result_code_t::SUCCESS);
        error_code_string = get_code_as_string();
    }
    explicit handshake_result_t(handshake_result_code_t _success)
        : code(_success) {
        guarantee(code == handshake_result_code_t::SUCCESS);
    }

    friend void serialize_universal(write_message_t *, const handshake_result_t &);
    friend archive_result_t deserialize_universal(read_stream_t *, handshake_result_t *);

    handshake_result_code_t code;
    // In case code is UNKNOWN_ERROR, this error message
    // will contain a human-readable description of the error code.
    // The idea is that if we are talking to a newer node on the other side,
    // it might send us some error codes that we don't understand. However the
    // other node will know how to format that error into an error message.
    std::string error_code_string;
    std::string additional_info;
};

// It is ok to add new result codes to handshake_result_code_t.
// However the existing code and the structure of handshake_result_t must be
// kept compatible.
void serialize_universal(write_message_t *wm, const handshake_result_t &r) {
    guarantee(r.code != handshake_result_code_t::UNKNOWN_ERROR,
              "Cannot serialize an unknown handshake result code");
    serialize_universal(wm, static_cast<uint8_t>(r.code));
    serialize_universal(wm, r.error_code_string);
    serialize_universal(wm, r.additional_info);
}
archive_result_t deserialize_universal(read_stream_t *s, handshake_result_t *out) {
    archive_result_t res;
    uint8_t code_int;
    res = deserialize_universal(s, &code_int);
    if (res != archive_result_t::SUCCESS) {
        return res;
    }
    if (code_int >= static_cast<uint8_t>(handshake_result_code_t::UNKNOWN_ERROR)) {
        // Unrecognized error code. Fall back to UNKNOWN_ERROR.
        out->code = handshake_result_code_t::UNKNOWN_ERROR;
    } else {
        out->code = static_cast<handshake_result_code_t>(code_int);
    }
    res = deserialize_universal(s, &out->error_code_string);
    if (res != archive_result_t::SUCCESS) {
        return res;
    }
    res = deserialize_universal(s, &out->additional_info);
    return res;
}

void fail_handshake(keepalive_tcp_conn_stream_t *conn,
                    const char *peername,
                    const handshake_result_t &reason,
                    bool send_error_to_peer = true) {
    logWRN("Connection attempt from %s failed, reason: %s ",
           peername, sanitize_for_logger(reason.get_error_reason()).c_str());

    if (send_error_to_peer) {
        // Send the reason for the failed handshake to the other side, so it can
        // print a nice message or do something else with it.
        write_message_t wm;
        serialize_universal(&wm, reason);
        if (send_write_message(conn, &wm)) {
            // network error. Ignore
        }
    }
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
        bool *successful_join_inout,
        const int join_delay_secs) THROWS_NOTHING
{
    parent->assert_thread();

    bool has_admin_password = false;
    {
        auth_semilattice_metadata_t auth = auth_sl_view->get();
        auto admin_user_it = auth.m_users.find(auth::username_t("admin"));
        if (admin_user_it != auth.m_users.end()
            && admin_user_it->second.get_ref()
            && !admin_user_it->second.get_ref()->get_password().is_empty()) {
            has_admin_password = true;
        }
    }

    /* TODO: If the other peer mysteriously stops talking to us, but doesn't close the
    connection, during the initialization process but before we construct the
    `heartbeat_manager_t`, then we might get stuck. Maybe we should add a timeout? It
    could just be a `signal_timer_t` that is wired into `conn_closer_1` but not
    `conn_closer_2`. */

    // Get the name of our peer, for error reporting.
    ip_and_port_t peer_addr;
    std::string peerstr = "(unknown)";
    if (conn->get_underlying_conn()->getpeername(&peer_addr))
        peerstr = peer_addr.to_string();
    const char *peername = peerstr.c_str();

    // Make sure that if we're ordered to shut down, any pending read
    // or write gets interrupted.
    cluster_conn_closing_subscription_t conn_closer_1(conn);
    conn_closer_1.reset(drainer_lock.get_drain_signal());

    // Each side sends a header followed by its own ID and address, then receives and checks the
    // other side's.
    {
        write_message_t wm;
        wm.append(cluster_proto_header.c_str(), cluster_proto_header.length());
        // TODO: Make some serialize_compatible_string function (matching the name of
        // deserialize_compatible_string).
        serialize_universal(&wm, static_cast<uint64_t>(cluster_version_string.length()));
        wm.append(cluster_version_string.data(), cluster_version_string.length());

        // Everything after we send the version string COULD be moved _below_ the
        // point where we resolve the version string.  That would mean adding another
        // back and forth to the handshake?
        serialize_universal(&wm, server_id);
        serialize_universal(&wm, static_cast<uint64_t>(cluster_arch_bitsize.length()));
        wm.append(cluster_arch_bitsize.data(), cluster_arch_bitsize.length());
        serialize_universal(&wm, static_cast<uint64_t>(cluster_build_mode.length()));
        wm.append(cluster_build_mode.data(), cluster_build_mode.length());
        serialize_universal(&wm, has_admin_password);
        serialize_universal(&wm, parent->me);
        serialize_universal(&wm, routing_table[parent->me].hosts());
        if (send_write_message(conn, &wm)) {
            return; // network error.
        }
    }

    // Receive & check header.
    {
        for (size_t i = 0; i < cluster_proto_header.length(); ++i) {
            // We read one byte at a time so we can bail out early if the header doesn't
            // match. This is ok performance-wise because `read` is buffered.
            char buffer;
            int64_t r = conn->read(&buffer, 1);
            if (-1 == r) {
                return; // network error.
            }
            rassert(r >= 0);
            rassert(r <= 1);
            // If EOF or remote_header does not match header, terminate connection.
            if (0 == r || cluster_proto_header[i] != buffer) {
                logWRN("Received invalid clustering header from %s, closing connection -- something might be connecting to the wrong port.", peername);
                return;
            }
        }
    }

    // Check version number (e.g. 1.9.0-466-gadea67)
    cluster_version_t resolved_version;
    {
        std::string remote_version_string;

        if (!deserialize_compatible_string(conn, &remote_version_string, peername)) {
            return;
        }

        if (!resolve_protocol_version(remote_version_string, &resolved_version)) {
            auto reason = handshake_result_t::error(
                handshake_result_code_t::UNRECOGNIZED_VERSION,
                strprintf("local: %s, remote: %s",
                          cluster_version_string.c_str(), remote_version_string.c_str()));
            // Peers before 1.14 don't support receiving a handshake error message.
            // So we must not send one.
            bool handshake_error_supported = false;
            std::vector<int64_t> parts;
            if (split_version_string(remote_version_string, &parts)) {
                if ((parts.size() >= 1 && parts[0] > 1)
                    || (parts.size() >= 2 && parts[0] == 1 && parts[1] >= 14)) {
                    handshake_error_supported = true;
                }
            }
            fail_handshake(conn, peername, reason, handshake_error_supported);
            return;
        }

        // In the future we'll need to support multiple cluster versions.
        guarantee(resolved_version == cluster_version_t::CLUSTER);
    }

    server_id_t remote_server_id;
    {
        if (deserialize_universal_and_check(conn, &remote_server_id, peername)) {
            return;
        }

        if (servers.count(remote_server_id) != 0) {
            // There currently is another connection open to the server
            logINF("Rejected a connection from server %s since one is open already.",
                   remote_server_id.print().c_str());
            return;
        }
    }
    set_insertion_sentry_t<server_id_t> remote_server_id_sentry(
        &servers, remote_server_id);

    // Check bitsize (e.g. 32bit or 64bit)
    {
        std::string remote_arch_bitsize;

        if (!deserialize_compatible_string(conn, &remote_arch_bitsize, peername)) {
            return;
        }

        if (remote_arch_bitsize != cluster_arch_bitsize) {
            auto reason = handshake_result_t::error(
                handshake_result_code_t::INCOMPATIBLE_ARCH,
                strprintf("local: %s, remote: %s",
                          cluster_arch_bitsize.c_str(), remote_arch_bitsize.c_str()));
            fail_handshake(conn, peername, reason);
            return;
        }

    }

    // Check build mode (e.g. debug or release)
    {
        std::string remote_build_mode;

        if (!deserialize_compatible_string(conn, &remote_build_mode, peername)) {
            return;
        }

        if (remote_build_mode != cluster_build_mode) {
            logWRN("Connecting nodes with different build modes, local: %s, remote: %s",
                   cluster_build_mode.c_str(),
                   remote_build_mode.c_str());
        }
    }

    // Check whether the other server has an admin password
    {
        bool remote_has_admin_password;

        if (deserialize_universal_and_check(conn, &remote_has_admin_password, peername)) {
            return;
        }

        // It's enough to do this check on one side. The handshake failure we send
        // will cause a corresponding message to be printed on the other end.
        if (!has_admin_password && remote_has_admin_password) {
            auto reason = handshake_result_t::error(
                handshake_result_code_t::PASSWORD_MISMATCH,
                "The remote peer has an admin password configured, but we don't. "
                "Connecting to it could make the cluster insecure. You can run this "
                "server with the `--initial-password auto` option to allow joining "
                "the password-protected cluster.");
            fail_handshake(conn, peername, reason);
            return;
        }
    }

    // Receive id, host/ports.
    peer_id_t other_id;
    std::set<host_and_port_t> other_peer_addr_hosts;
    if (deserialize_universal_and_check(conn, &other_id, peername) ||
        deserialize_universal_and_check(conn, &other_peer_addr_hosts, peername)) {
        return;
    }

    {
        // Tell the other node that we are happy to connect with it
        write_message_t wm;
        serialize_universal(&wm, handshake_result_t::success());
        if (send_write_message(conn, &wm)) {
            return; // network error.
        }

        // Check if there was an issue with the connection initiation
        handshake_result_t handshake_result;
        if (deserialize_universal_and_check(conn, &handshake_result, peername)) {
            return;
        }
        if (handshake_result.get_code() != handshake_result_code_t::SUCCESS) {
            logWRN("Remote node refused to connect with us, peer: %s, reason: \"%s\"",
                   peername,
                   sanitize_for_logger(handshake_result.get_error_reason()).c_str());
            return;
        }
    }

    // Look up the ip addresses for the other host
    object_buffer_t<peer_address_t> other_peer_addr;

    try {
        other_peer_addr.create(other_peer_addr_hosts);
    } catch (const host_lookup_exc_t &) {
        printf_buffer_t hostnames;
        for (auto const &host_and_port : other_peer_addr_hosts) {
            hostnames.appendf("%s%s", hostnames.size() > 0 ? ", " : "",
                              host_and_port.host().c_str());
        }
        logERR("Connected to peer with unresolvable hostname%s: %s, closing "
               "connection.  Consider using the '--canonical-address' launch option.",
               other_peer_addr_hosts.size() > 1 ? "s" : "", hostnames.c_str());
        return;
    }

    /* Sanity checks */
    if (other_id == parent->me) {
        // TODO: report this on command-line in some cases. see issue 546 on github.
        return;
    }
    if (other_id.is_nil()) {
        logERR("Received nil peer id from %s, closing connection.", peername);
        return;
    }
    if (expected_id && other_id != *expected_id) {
        // This is only a problem if we're not using a loopback address
        if (!peer_addr.ip().is_loopback()) {
            logERR("Received inconsistent routing information (wrong ID) from %s, "
                   "closing connection.", peername);
        }
        return;
    }
    if (expected_address && !is_similar_peer_address(*other_peer_addr.get(),
                                                     *expected_address)) {
        printf_buffer_t buf;
        buf.appendf("expected_address = ");
        debug_print(&buf, *expected_address);
        buf.appendf(", other_peer_addr = ");
        debug_print(&buf, *other_peer_addr.get());

        logERR("Received inconsistent routing information (wrong address) from %s (%s), "
               "closing connection.  Consider using the '--canonical-address' launch "
               "option.", peername, buf.c_str());
        return;
    }

    // Just saying that we're still on the rpc listener thread.
    parent->assert_thread();

    /* The trickiest case is when there are two or more parallel connections
    that are trying to be established between the same two servers. We can get
    this when e.g. server A and server B try to connect to each other at the
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
    std::map<peer_id_t, std::set<host_and_port_t> > other_routing_table;

    if (we_are_leader) {
        std::map<peer_id_t, std::set<host_and_port_t> > routing_table_to_send;
        if (!get_routing_table_to_send_and_add_peer(other_id,
                                                    *other_peer_addr.get(),
                                                    &routing_table_entry_sentry,
                                                    &routing_table_to_send)) {
            return;
        }

        /* We're good to go! Transmit the routing table to the follower, so it
        knows we're in. */
        {
            write_message_t wm;
            serialize_for_version(resolved_version, &wm, routing_table_to_send);
            if (send_write_message(conn, &wm)) {
                return;         // network error
            }
        }

        /* Receive the follower's routing table */
        if (deserialize_and_check(resolved_version, conn,
                                  &other_routing_table, peername)) {
            return;
        }

    } else {
        /* Receive the leader's routing table. (If our connection has lost a
        conflict, then the leader will close the connection instead of sending
        the routing table. */
        if (deserialize_and_check(resolved_version, conn,
                                  &other_routing_table, peername)) {
            return;
        }

        std::map<peer_id_t, std::set<host_and_port_t> > routing_table_to_send;
        if (!get_routing_table_to_send_and_add_peer(other_id,
                                                    *other_peer_addr.get(),
                                                    &routing_table_entry_sentry,
                                                    &routing_table_to_send)) {
            return;
        }

        /* Send our routing table to the leader */
        {
            write_message_t wm;
            serialize_for_version(resolved_version, &wm, routing_table_to_send);
            if (send_write_message(conn, &wm)) {
                return;         // network error
            }
        }
    }

    // Just saying: We haven't left the RPC listener thread.
    parent->assert_thread();

    // This check is so that when trying multiple connections to a peer in parallel, we can
    //  make sure only one of them succeeds
    if (successful_join_inout != nullptr) {
        if (*successful_join_inout) {
            logWRN("Somehow ended up with two successful joins to a peer, closing one");
            return;
        }
        *successful_join_inout = true;
    }

    /* For each peer that our new friend told us about that we don't already
    know about, start a new connection. If the cluster is shutting down, skip
    this step. */
    if (!drainer_lock.get_drain_signal()->is_pulsed()) {
        for (auto it = other_routing_table.begin(); it != other_routing_table.end(); ++it) {
            if (routing_table.find(it->first) == routing_table.end()) {
                try {
                    // This is where we resolve the peer's ip addresses
                    peer_address_t next_peer_addr(it->second);

                    // `it->first` is the ID of a peer that our peer is connected
                    //  to, but we aren't connected to.
                    coro_t::spawn_now_dangerously(std::bind(
                        &connectivity_cluster_t::run_t::join_blocking, this,
                        next_peer_addr,
                        boost::optional<peer_id_t>(it->first),
                        join_delay_secs,
                        drainer_lock));
                } catch (const host_lookup_exc_t &ex) {
                    printf_buffer_t hostnames;
                    for (auto const &host_and_port : it->second) {
                        hostnames.appendf("%s%s", hostnames.size() > 0 ? ", " : "",
                                          host_and_port.host().c_str());
                    }
                    logERR("Informed of peer with unresolvable hostname%s: %s. We will not be "
                           "able to connect to this peer, which could result in diminished "
                           "availability.  Consider using the '--canonical-address' launch option.",
                           it->second.size() > 1 ? "s" : "", hostnames.c_str());
                }
            }
        }
    }

    /* Now that we're about to switch threads, it's not safe to try to close
    the connection from this thread anymore. This is safe because we won't do
    anything that permanently blocks before setting up `conn_closer_2`. */
    conn_closer_1.reset();

    thread_allocation_t chosen_thread(&parent->thread_allocator);

    cross_thread_signal_t connection_thread_drain_signal(
        drainer_lock.get_drain_signal(),
        chosen_thread.get_thread());
    cross_thread_watchable_variable_t<heartbeat_semilattice_metadata_t>
        cross_thread_heartbeat_sl_view(
            clone_ptr_t<semilattice_watchable_t<heartbeat_semilattice_metadata_t> >(
                new semilattice_watchable_t<heartbeat_semilattice_metadata_t>(
                    heartbeat_sl_view)), chosen_thread.get_thread());

    rethread_tcp_conn_stream_t unregister_conn(conn, INVALID_THREAD);
    on_thread_t conn_threader(chosen_thread.get_thread());
    rethread_tcp_conn_stream_t reregister_conn(conn, get_thread_id());

    // Make sure that if we're ordered to shut down, any pending read
    // or write gets interrupted.
    cluster_conn_closing_subscription_t conn_closer_2(conn);
    conn_closer_2.reset(&connection_thread_drain_signal);

    // Wait a certain amount to make sure that the connection is table. Only then
    // add it to the connectivity cluster and start processing messages.
    if (join_delay_secs > 0) {
        logINF("Delaying the join with server %s for %d seconds.",
               remote_server_id.print().c_str(),
               join_delay_secs);
        try {
            nap(static_cast<int64_t>(join_delay_secs) * 1000,
                &connection_thread_drain_signal);
        } catch (const interrupted_exc_t &) {
            // Ignore this here. We will bail out below.
        }
    }

    {
        /* `connection_t` is the public interface of this coroutine. Its
        constructor registers it in the `connectivity_cluster_t`'s connection
        map. */
        connection_t conn_structure(
            this, other_id, remote_server_id, conn, *other_peer_addr.get());

        /* `heartbeat_manager` will periodically send a heartbeat message to
        other servers, and it will also close the connection if we don't
        receive anything for a while. */
        heartbeat_manager_t heartbeat_manager(
            &conn_structure,
            auto_drainer_t::lock_t(conn_structure.drainers.get()),
            peerstr,
            cross_thread_heartbeat_sl_view.get_watchable());

        /* Main message-handling loop: read messages off the connection until
        it's closed, which may be due to network events, or the other end
        shutting down, or us shutting down. */
        try {
            int messages_handled_since_yield = 0;
            while (true) {
                message_tag_t tag;
                archive_result_t res = deserialize_universal(conn, &tag);
                if (bad(res)) { throw fake_archive_exc_t(); }

                /* Ignore messages tagged with the heartbeat tag. The
                `keepalive_tcp_conn_stream_t` will have already notified the
                `heartbeat_manager_t` as soon as the heartbeat arrived. */
                if (tag != heartbeat_tag) {
                    cluster_message_handler_t *handler = parent->message_handlers[tag];
                    guarantee(handler != nullptr, "Got a message for an unfamiliar tag. "
                        "Apparently we aren't compatible with the cluster on the other "
                        "end.");

                    /* If you really want to support old cluster versions, the
                    resolved_version should be passed into the on_message() handler. */
                    guarantee(resolved_version == cluster_version_t::CLUSTER);
                    handler->on_message(
                        &conn_structure,
                        auto_drainer_t::lock_t(conn_structure.drainers.get()),
                        conn); // might raise fake_archive_exc_t
                }

                ++messages_handled_since_yield;
                if (messages_handled_since_yield >= MESSAGE_HANDLER_MAX_BATCH_SIZE) {
                    coro_t::yield();
                    messages_handled_since_yield = 0;
                }
            }
        } catch (const fake_archive_exc_t &) {
            /* The exception broke us out of the loop, and that's what we
            wanted. This could either be because we lost contact with the peer
            or because the cluster is shutting down and `close_conn()` got
            called. */
        }

        if (conn->is_read_open()) {
            logWRN("Received invalid data on a cluster connection. Disconnecting.");
            conn->shutdown_read();
        }
        if (conn->is_write_open()) {
            /* Shutdown the write direction as well, to make sure that any active
            `send_message` calls get interrupted and don't stop us from destructing
            the `conn_structure`. */
            conn->shutdown_write();
        }

        /* The `conn_structure` destructor removes us from the connection map. It also
        blocks until all references to `conn_structure` have been released (using its
        `auto_drainer_t`s). */
    }

    /* Before we destruct the `rethread_tcp_conn_stream_t`s, we must make sure that
    any pending network writes have either been transmitted or aborted.
    `shutdown_write()` which we call above initiates aborting pending writes, but it
    doesn't wait until the process is done. */
    conn->flush_buffer();
}

connectivity_cluster_t::connectivity_cluster_t() THROWS_NOTHING :
    me(peer_id_t(generate_uuid())),
    /* We assign threads from the highest thread number downwards. This is to reduce the
    potential for conflicting with btree threads, which assign threads from the lowest
    thread number upwards. */
    thread_allocator([](threadnum_t a, threadnum_t b) {
        return a.threadnum > b.threadnum;
    }),
    current_run(nullptr),
    connectivity_collection(),
    stats_membership(&get_global_perfmon_collection(), &connectivity_collection, "connectivity")
{
    for (int i = 0; i < max_message_tag; i++) {
        message_handlers[i] = nullptr;
    }
}

connectivity_cluster_t::~connectivity_cluster_t() THROWS_NOTHING {
    guarantee(!current_run);

#ifdef ENABLE_MESSAGE_PROFILER
    std::map<std::string, std::pair<uint64_t, uint64_t> > total_counts;
    pmap(get_num_threads(), [&](int num) {
        std::map<std::string, std::pair<uint64_t, uint64_t> > copy;
        {
            on_thread_t thread_switcher((threadnum_t(num)));
            copy = *message_profiler_counts.get();
        }
        for (const auto &pair : copy) {
            total_counts[pair.first].first += pair.second.first;
            total_counts[pair.first].second += pair.second.second;
        }
    });

    std::string output_filename =
        strprintf("message_profiler_out_%d.txt", static_cast<int>(getpid()));
    FILE *file = fopen(output_filename.c_str(), "w");
    guarantee(file != nullptr, "Cannot open %s for writing", output_filename.c_str());
    for (const auto &pair : total_counts) {
        fprintf(file, "%" PRIu64 " %" PRIu64 " %s\n",
            pair.second.first, pair.second.second, pair.first.c_str());
    }
    fclose(file);
#endif
}

peer_id_t connectivity_cluster_t::get_me() THROWS_NOTHING {
    return me;
}

watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t> *
connectivity_cluster_t::get_connections() THROWS_NOTHING {
    return connections.get();
}

connectivity_cluster_t::connection_t *connectivity_cluster_t::get_connection(
        peer_id_t peer_id, auto_drainer_t::lock_t *keepalive_out) THROWS_NOTHING {
    connectivity_cluster_t::connection_t *conn;
    connections.get()->read_key(peer_id,
        [&](const connection_pair_t *value) {
            if (value == nullptr) {
                conn = nullptr;
            } else {
                conn = value->first;
                *keepalive_out = value->second;
            }
        });
    return conn;
}

void connectivity_cluster_t::send_message(connection_t *connection,
                                     auto_drainer_t::lock_t connection_keepalive,
                                     message_tag_t tag,
                                     cluster_send_message_write_callback_t *callback) {
    // We could be on _any_ thread.

    /* If the connection is being closed, just drop the message now. It's not going
    to actually get sent anyway. That way we avoid getting in line for the send_mutex. */
    if (connection_keepalive.get_drain_signal()->is_pulsed()) {
        return;
    }

    /* We currently write the message to a vector_stream_t, then
       serialize that as a string. It's horribly inefficient, of course. */
    // TODO: If we don't do it this way, we (or the caller) will need
    // to worry about having the writer run on the connection thread.
    vector_stream_t buffer;
    // Reserve some space to reduce overhead (especially for small messages)
    buffer.reserve(1024);
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
        print_hd(buffer.vector().data(), 0, buffer.vector().size());
    }
#endif

#ifndef NDEBUG
    connection_keepalive.assert_is_holding(connection->drainers.get());

    /* We're allowed to block indefinitely, but it's tempting to write code on
    the assumption that we won't. This might catch some programming errors. */
    if (randint(10) == 0) {
        nap(10);
    }
#endif

    size_t bytes_sent = buffer.vector().size();

#ifdef ENABLE_MESSAGE_PROFILER
    std::pair<uint64_t, uint64_t> *stats =
        &(*message_profiler_counts.get())[callback->message_profiler_tag()];
    stats->first += 1;
    stats->second += bytes_sent;
#endif

    if (connection->is_loopback()) {
        // We could be on any thread here! Oh no!
        std::vector<char> buffer_data;
        buffer.swap(&buffer_data);
        rassert(message_handlers[tag], "No message handler for tag %" PRIu8, tag);
        message_handlers[tag]->on_local_message(connection, connection_keepalive,
            std::move(buffer_data));
    } else {
        on_thread_t threader(connection->conn->home_thread());

        /* Acquire the send-mutex so we don't collide with other things trying
        to send on the same connection. */
        {
            /* The `true` is for eager waiting, which is a significant performance
            optimization in this case. */
            mutex_t::acq_t acq(&connection->send_mutex, true);

            /* Write the tag to the network */
            {
                // All cluster versions use a uint8_t tag here.
                write_message_t wm;
                static_assert(std::is_same<message_tag_t, uint8_t>::value,
                              "We expect to be serializing a uint8_t -- if this has "
                              "changed, the cluster communication format has changed and "
                              "you need to ask yourself whether live cluster upgrades work."
                              );
                serialize_universal(&wm, tag);
                make_buffered_tcp_conn_stream_wrapper_t buffered_conn(connection->conn);
                int res = send_write_message(&buffered_conn, &wm);
                if (res == -1) {
                    /* Close the other half of the connection to make sure that
                       `connectivity_cluster_t::run_t::handle()` notices that something is
                       up */
                    if (connection->conn->is_read_open()) {
                        connection->conn->shutdown_read();
                    }
                    return;
                }
            }

            /* Write the message itself to the network */
            {
                int64_t res = connection->conn->write_buffered(buffer.vector().data(),
                                                               buffer.vector().size());
                if (res == -1) {
                    if (connection->conn->is_read_open()) {
                        connection->conn->shutdown_read();
                    }
                    return;
                } else {
                    guarantee(res == static_cast<int64_t>(buffer.vector().size()));
                }
            }
        } /* Releases the send_mutex */

        connection->flusher.notify();
        cond_t dummy_interruptor;
        connection->flusher.flush(&dummy_interruptor);
        if (!connection->conn->is_write_open()) {
            if (connection->conn->is_read_open()) {
                connection->conn->shutdown_read();
            }
            return;
        }
    }

    connection->pm_bytes_sent.record(bytes_sent);
}

cluster_message_handler_t::cluster_message_handler_t(
        connectivity_cluster_t *cm,
        connectivity_cluster_t::message_tag_t t) :
    connectivity_cluster(cm), tag(t)
{
    guarantee(!connectivity_cluster->current_run);
    rassert(tag != connectivity_cluster_t::heartbeat_tag,
        "Tag %" PRIu8 " is reserved for heartbeat messages.",
        connectivity_cluster_t::heartbeat_tag);
    rassert(connectivity_cluster->message_handlers[tag] == nullptr);
    connectivity_cluster->message_handlers[tag] = this;
}

cluster_message_handler_t::~cluster_message_handler_t() {
    guarantee(!connectivity_cluster->current_run);
    rassert(connectivity_cluster->message_handlers[tag] == this);
    connectivity_cluster->message_handlers[tag] = nullptr;
}

void cluster_message_handler_t::on_local_message(
        connectivity_cluster_t::connection_t *conn,
        auto_drainer_t::lock_t keepalive,
        std::vector<char> &&data) {
    vector_read_stream_t read_stream(std::move(data));
    on_message(conn, keepalive, &read_stream);
}

