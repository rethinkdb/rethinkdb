// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_CLUSTER_HPP_
#define RPC_CONNECTIVITY_CLUSTER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "arch/types.hpp"
#include "arch/io/openssl.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/watchable_map.hpp"
#include "containers/archive/tcp_conn_stream.hpp"
#include "containers/map_sentries.hpp"
#include "concurrency/pump_coro.hpp"
#include "perfmon/perfmon.hpp"
#include "random.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/connectivity/server_id.hpp"
#include "utils.hpp"

namespace boost {
template <class> class optional;
}

class auth_semilattice_metadata_t;
class cluster_message_handler_t;
class co_semaphore_t;
class heartbeat_semilattice_metadata_t;
template <class> class semilattice_read_view_t;

/* Uncomment this to enable message profiling. Message profiling will keep track of how
many messages of each type are sent over the network; it will dump the results to a file
named `msg_profiler_out_PID.txt` on shutdown. Each line of that file will be of the
following form:
    <# messages> <# bytes> <source>
where `<# messages>` is the total number of messages sent by that source, `<# bytes>` is
the combined size of all of those messages in bytes, and `<source>` is a string
describing the type of message. */
// #define ENABLE_MESSAGE_PROFILER

class cluster_send_message_write_callback_t {
public:
    virtual ~cluster_send_message_write_callback_t() { }
    // write() doesn't take a version argument because the version is always
    // cluster_version_t::CLUSTER for cluster messages.
    virtual void write(write_stream_t *stream) = 0;

#ifdef ENABLE_MESSAGE_PROFILER
    /* This should return a string that describes the type of message being sent for
    profiling purposes. The returned string must be statically allocated (i.e. valid
    indefinitely). */
    virtual const char *message_profiler_tag() const = 0;
#endif
};

/* `connectivity_cluster_t` is responsible for establishing connections with other
servers and communicating with them. It's the foundation of the entire clustering
system. However, it's very low-level; most code will instead use the directory or mailbox
mechanisms, which are built on top of `connectivity_cluster_t`.

Clustering is based around the concept of a "connection", as represented by
`connectivity_cluster_t::connection_t`. When the `cluster_t::run_t` is constructed, we
automatically create a `connection_t` to ourself, the "loopback connection". We also
accept TCP connections on some port. When we get a TCP connection, we perform a
handshake; if this succeeds, then we create a `connection_t` to represent the new
connection. Once a connection is established, messages can be sent across it in both
directions. Every message is guaranteed to eventually arrive unless the connection goes
down. Messages cannot be duplicated.

Can messages be reordered? I think the current implementation doesn't ever reorder
messages, but don't rely on this guarantee. However, some old code may rely on this
guarantee (I'm not sure) so don't break this property without checking first. */

class connectivity_cluster_t :
    public home_thread_mixin_debug_only_t
{
public:
    static const std::string cluster_proto_header;
    static const std::string cluster_version_string;
    static const std::string cluster_arch_bitsize;
    static const std::string cluster_build_mode;

    /* Every clustering message has a "tag", which determines what message handler on the
    destination server will deal with it. Tags are a low-level concept, and there are
    only a few of them; for example, all directory-related messages have one tag, and all
    mailbox-related messages have another. Higher-level code uses the mailbox system for
    routing messages. */
    typedef uint8_t message_tag_t;
    static const int max_message_tag = 256;

    /* This tag is reserved exclusively for heartbeat messages. */
    static const message_tag_t heartbeat_tag = 'H';

    class run_t;

    /* `connection_t` represents an open connection to another server. If we lose
    contact with another server and then regain it, then a new `connection_t` will be
    created. Generally, any code that handles a `connection_t *` will also carry around a
    `auto_drainer_t::lock_t` that ensures the connection object isn't destroyed while in
    use. This doubles as a mechanism for finding out when the connection has been lost;
    if the connection dies, the `auto_drainer_t::lock_t` will pulse its
    `get_drain_signal()`. There will never be two `connection_t` objects that refer to
    the same peer.

    `connection_t` is completely thread-safe. You can pass connections from thread to
    thread and call the methods on any thread. */
    class connection_t : public home_thread_mixin_debug_only_t {
    public:
        /* Returns the peer ID of the other server. Peer IDs change when a node
        restarts, but not when it loses and then regains contact. */
        peer_id_t get_peer_id() const {
            return peer_id;
        }

        /* Returns the server id of the other server. */
        server_id_t get_server_id() const {
            return server_id;
        }

        /* Returns the address of the other server. */
        peer_address_t get_peer_address() const {
            return peer_address;
        }

        /* Returns `true` if this is the loopback connection */
        bool is_loopback() const {
            return conn == nullptr;
        }

        /* Drops the connection. */
        void kill_connection();

    private:
        friend class connectivity_cluster_t;

        /* The constructor registers us in every thread's `connections` map, thereby
        notifying event subscribers. */
        connection_t(
            run_t *,
            const peer_id_t &peer_id,
            const server_id_t &server_id,
            keepalive_tcp_conn_stream_t *,
            const peer_address_t &peer_address) THROWS_NOTHING;
        ~connection_t() THROWS_NOTHING;

        /* NULL for the loopback connection (i.e. our "connection" to ourself) */
        keepalive_tcp_conn_stream_t *conn;

        /* `connection_t` contains the addresses so that we can call
        `get_peers_list()` on any thread. Otherwise, we would have to go
        cross-thread to access the routing table. */
        peer_address_t peer_address;

        /* Unused for our connection to ourself */
        mutex_t send_mutex;

        /* Calls `conn->flush_buffer()`. Can be used for making sure that a
        buffered write makes it to the TCP stack. */
        pump_coro_t flusher;

        perfmon_collection_t pm_collection;
        perfmon_sampler_t pm_bytes_sent;
        perfmon_membership_t pm_collection_membership, pm_bytes_sent_membership;

        /* We only hold this information so we can deregister ourself */
        run_t *parent;

        peer_id_t peer_id;
        server_id_t server_id;

        one_per_thread_t<auto_drainer_t> drainers;
    };

    /* Construct one `run_t` for each `connectivity_cluster_t` after setting up the
    message handlers. The `run_t`'s constructor is what actually starts listening for
    connections from other nodes, and the destructor is what stops listening. This way,
    we use RAII to ensure that we stop sending messages to the message handlers before we
    destroy the message handlers. */
    class run_t {
    public:
        run_t(connectivity_cluster_t *parent,
              const server_id_t &server_id,
              const std::set<ip_address_t> &local_addresses,
              const peer_address_t &canonical_addresses,
              const int join_delay_secs,
              int port,
              int client_port,
              boost::shared_ptr<semilattice_read_view_t<
                  heartbeat_semilattice_metadata_t> > heartbeat_sl_view,
              boost::shared_ptr<semilattice_read_view_t<
                  auth_semilattice_metadata_t> > auth_sl_view,
              tls_ctx_t *tls_ctx)
            THROWS_ONLY(address_in_use_exc_t, tcp_socket_exc_t);

        ~run_t();

        /* Attaches the cluster this node is part of to another existing
        cluster. May only be called on home thread. Returns immediately (it does
        its work in the background). */
        void join(const peer_address_t &address, const int join_delay_secs) THROWS_NOTHING;

        std::set<host_and_port_t> get_canonical_addresses();
        int get_port();

    private:
        friend class connectivity_cluster_t;

        /* Sets a variable to a value in its constructor; sets it to NULL in its
        destructor. This is kind of silly. The reason we need it is that we need
        the variable to be set before the constructors for some other fields of
        the `run_t` are constructed. */
        class variable_setter_t {
        public:
            variable_setter_t(run_t **var, run_t *val) : variable(var) , value(val) {
                guarantee(*variable == nullptr);
                *variable = value;
            }

            ~variable_setter_t() THROWS_NOTHING {
                guarantee(*variable == value);
                *variable = nullptr;
            }
        private:
            run_t **variable;
            run_t *value;
            DISABLE_COPYING(variable_setter_t);
        };

        void on_new_connection(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
                const int join_delay_secs,
                auto_drainer_t::lock_t lock) THROWS_NOTHING;

        /* `connect_to_peer` is spawned for each known ip address of a peer which we want
        to connect to, all but one should fail */
        void connect_to_peer(const peer_address_t *addr,
                             int index,
                             boost::optional<peer_id_t> expected_id,
                             auto_drainer_t::lock_t drainer_lock,
                             bool *successful_join_inout,
                             const int join_delay_secs,
                             co_semaphore_t *rate_control) THROWS_NOTHING;

        /* `join_blocking()` is spawned in a new coroutine by `join()`. It's also run by
        `handle()` when we hear about a new peer from a peer we are connected to. */
        void join_blocking(const peer_address_t hosts,
                           boost::optional<peer_id_t>,
                           const int join_delay_secs,
                           auto_drainer_t::lock_t) THROWS_NOTHING;

        // Normal routing table isn't serializable, so we send just the hosts/ports
        bool get_routing_table_to_send_and_add_peer(const peer_id_t &other_peer_id,
                                                    const peer_address_t &other_peer_addr,
                                                    object_buffer_t<map_insertion_sentry_t<peer_id_t, peer_address_t> > *routing_table_entry_sentry,
                                                    std::map<peer_id_t, std::set<host_and_port_t> > *result);

        /* `handle()` takes an `auto_drainer_t::lock_t` so that we never shut
        down while there are still running instances of `handle()`. It's
        responsible for the entire lifetime of an intra-cluster TCP connection.
        It handles the handshake, exchanging node maps, sending out the
        connect-notification, receiving messages from the peer until it
        disconnects or we are shut down, and sending out the
        disconnect-notification. */
        void handle(keepalive_tcp_conn_stream_t *c,
            boost::optional<peer_id_t> expected_id,
            boost::optional<peer_address_t> expected_address,
            auto_drainer_t::lock_t,
            bool *successful_join_inout,
            const int join_delay_secs) THROWS_NOTHING;

        connectivity_cluster_t *parent;

        /* The server's own id and the set of servers we are connected to, we only allow
        a single connection per server. */
        server_id_t server_id;
        std::set<server_id_t> servers;

        tls_ctx_t *tls_ctx;

        /* `attempt_table` is a table of all the host:port pairs we're currently
        trying to connect to or have connected to. If we are told to connect to
        an address already in this table, we'll just ignore it. That's important
        because when `client_port` is specified we will make all of our
        connections from the same source, and TCP might not be able to
        disambiguate between them. */
        peer_address_set_t attempt_table;
        mutex_assertion_t attempt_table_mutex;

        /* `routing_table` is all the peers we can currently access and their
        addresses. Peers that are in the process of connecting or disconnecting
        may be in `routing_table` but not in
        `parent->thread_info.get()->connection_map`. */
        std::map<peer_id_t, peer_address_t> routing_table;

        /* Writes to `routing_table` are protected by this mutex so we never get
        redundant connections to the same peer. */
        mutex_t new_connection_mutex;

        scoped_ptr_t<tcp_bound_socket_t> cluster_listener_socket;
        int cluster_listener_port;
        int cluster_client_port;

        variable_setter_t register_us_with_parent;

        map_insertion_sentry_t<peer_id_t, peer_address_t> routing_table_entry_for_ourself;
        connection_t connection_to_ourself;

        /* For picking random threads */
        rng_t rng;

        boost::shared_ptr<semilattice_read_view_t<heartbeat_semilattice_metadata_t> >
            heartbeat_sl_view;

        boost::shared_ptr<semilattice_read_view_t<auth_semilattice_metadata_t> >
            auth_sl_view;

        auto_drainer_t drainer;

        /* This must be destroyed before `drainer` is. */
        scoped_ptr_t<tcp_listener_t> listener;
    };

    connectivity_cluster_t() THROWS_NOTHING;
    ~connectivity_cluster_t() THROWS_NOTHING;

    peer_id_t get_me() THROWS_NOTHING;

    /* This returns a watchable table of every active connection. The returned
    `watchable_map_t` will be valid for the thread that `get_connections()` was called
    on. */
    typedef std::pair<connection_t *, auto_drainer_t::lock_t> connection_pair_t;
    watchable_map_t<peer_id_t, connection_pair_t> *get_connections() THROWS_NOTHING;

    /* Shortcut if you just want to access one connection, which is by far the most
    common case. Returns `NULL` if there is no active connection to the given peer. */
    connection_t *get_connection(peer_id_t peer,
            auto_drainer_t::lock_t *keepalive_out) THROWS_NOTHING;

    /* Sends a message to the other server. The message is associated with a "tag",
    which determines which message handler on the other server will receive the message.
    */
    void send_message(connection_t *connection,
                      auto_drainer_t::lock_t connection_keepalive,
                      message_tag_t tag,
                      cluster_send_message_write_callback_t *callback);

private:
    friend class cluster_message_handler_t;
    friend class run_t;

    class heartbeat_manager_t;

    /* `me` is our `peer_id_t`. */
    const peer_id_t me;

    /* Used to assign threads to individual cluster connections */
    thread_allocator_t thread_allocator;

    /* `connections` holds open connections to other peers. It's the same on every
    thread, except that the `auto_drainer_t::lock_t`s on each thread correspond to the
    thread-specific `auto_drainer_t`s in the `connection_t`. It has an entry for every
    peer we are fully and officially connected to, including us. That means it's a subset
    of the entries in `run_t::routing_table`. The only legal way to acquire a
    connection's `auto_drainer_t` is through this map; this way, the connection can make
    sure nobody acquires a lock on its `auto_drainer_t` after it removes itself from
    `connections`. */
    one_per_thread_t<watchable_map_var_t<peer_id_t, connection_pair_t> > connections;

    cluster_message_handler_t *message_handlers[max_message_tag];

#ifdef ENABLE_MESSAGE_PROFILER
    /* The key is the string passed to `send_message()`. The value is a pair of (number
    of individual messages, total number of bytes). */
    one_per_thread_t<std::map<std::string, std::pair<uint64_t, uint64_t> > >
        message_profiler_counts;
#endif

    run_t *current_run;

    perfmon_collection_t connectivity_collection;
    perfmon_membership_t stats_membership;

    DISABLE_COPYING(connectivity_cluster_t);
};

/* Subclass `cluster_message_handler_t` to handle messages received over the network. The
`cluster_message_handler_t` constructor will automatically register it to handle
messages. You can only register and unregister message handlers when there is no `run_t`
in existence. */
class cluster_message_handler_t {
public:
    connectivity_cluster_t *get_connectivity_cluster() {
        return connectivity_cluster;
    }
    connectivity_cluster_t::message_tag_t get_message_tag() { return tag; }

    peer_id_t get_me() {
        return connectivity_cluster->get_me();
    }

protected:
    /* Registers the message handler with the cluster */
    cluster_message_handler_t(connectivity_cluster_t *connectivity_cluster,
                              connectivity_cluster_t::message_tag_t tag);
    virtual ~cluster_message_handler_t();

    /* This can be called on any thread. */
    virtual void on_message(connectivity_cluster_t::connection_t *conn,
                            auto_drainer_t::lock_t keepalive,
                            read_stream_t *) = 0;

    /* The default implementation constructs a stream reading from `data` and then
    calls `on_message()`. Override to optimize for the local case. */
    virtual void on_local_message(connectivity_cluster_t::connection_t *conn,
                                  auto_drainer_t::lock_t keepalive,
                                  std::vector<char> &&data);

private:
    friend class connectivity_cluster_t;
    connectivity_cluster_t *connectivity_cluster;
    const connectivity_cluster_t::message_tag_t tag;
};

#endif /* RPC_CONNECTIVITY_CLUSTER_HPP_ */

