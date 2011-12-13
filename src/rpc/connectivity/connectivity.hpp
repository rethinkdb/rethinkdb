#ifndef __RPC_CONNECTIVITY_CONNECTIVITY_HPP__
#define __RPC_CONNECTIVITY_CONNECTIVITY_HPP__

#include <map>

#include "utils.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/utility.hpp>

#include "arch/streamed_tcp.hpp"
#include "arch/address.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/signal.hpp"

struct peer_address_t {
    peer_address_t(ip_address_t i, int p) : ip(i), port(p) { }
    peer_address_t() : ip(), port(0) { } // For deserialization
    ip_address_t ip;
    int port;

    bool operator==(const peer_address_t &a) const {
        return ip == a.ip && port == a.port;
    }
    bool operator!=(const peer_address_t &a) const {
        return ip != a.ip || port != a.port;
    }

private:
    friend class ::boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, UNUSED const unsigned int version) {
        ar & ip;
        ar & port;
    }
};

/* `peer_id_t` is a wrapper around a `boost::uuids::uuid`. Each newly
created cluster node picks a UUID to be its peer-ID. */
struct peer_id_t {
    bool operator==(const peer_id_t &p) const {
        return p.uuid == uuid;
    }
    bool operator!=(const peer_id_t &p) const {
        return p.uuid != uuid;
    }
    bool operator<(const peer_id_t &p) const {
        return p.uuid < uuid;
    }

    peer_id_t() : uuid(boost::uuids::nil_uuid()) { }

    bool is_nil() const {
        return uuid.is_nil();
    }

private:
    friend class connectivity_cluster_t;
    friend std::ostream &operator<<(std::ostream &, peer_id_t);

    boost::uuids::uuid uuid;
    explicit peer_id_t(boost::uuids::uuid u) : uuid(u) { }

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, UNUSED const unsigned int version) {
        ar & uuid;
    }
};

inline std::ostream &operator<<(std::ostream &stream, peer_id_t id) {
    return stream << id.uuid;
}

class connectivity_cluster_t :
    public home_thread_mixin_t
{
public:
    /* While a `peers_list_freeze_t` exists, no connect or disconnect events
    will be delivered. This is so that you can check the status of a peer or
    peers and construct a `connectivity_subscription_t` atomically, without
    worrying about whether there was a connection or disconnection in between.
    Don't block while holding a `peers_list_freeze_t`. */
    class peers_list_freeze_t {
    public:
        peers_list_freeze_t(connectivity_cluster_t *);
        void assert_is_holding(connectivity_cluster_t *);
    private:
        mutex_assertion_t::acq_t acq;
    };

    /* `peers_list_subscription_t` will call the given functions when a peer
    connects or disconnects. */
    class peers_list_subscription_t {
    public:
        peers_list_subscription_t(
                const boost::function<void(peer_id_t)> &on_connect,
                const boost::function<void(peer_id_t)> &on_disconnect);
        peers_list_subscription_t(
                const boost::function<void(peer_id_t)> &on_connect,
                const boost::function<void(peer_id_t)> &on_disconnect,
                connectivity_cluster_t *, peers_list_freeze_t *proof);
        void reset();
        void reset(connectivity_cluster_t *, peers_list_freeze_t *proof);
    private:
        publisher_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > >::subscription_t subs;
    };

    connectivity_cluster_t(
        const boost::function<void(peer_id_t, std::istream &, const boost::function<void()> &)> &on_message,
        int port);
    ~connectivity_cluster_t();

    /* Attaches the cluster this node is part of to another existing cluster.
    May only be called on home thread. */
    void join(peer_address_t);

    /* `get_me()` returns the `peer_id_t` for this cluster node.
    `get_everybody()` returns all the currently-accessible peers in the cluster
    and their addresses, including us. */
    peer_id_t get_me();
    std::map<peer_id_t, peer_address_t> get_everybody();

    /* TODO: We should have a better mechanism for sending messages to ourself.
    Right now, they get serialized and then deserialized. If we did it more
    efficiently, we wouldn't have to special-case messages to local mailboxes on
    the higher levels. */

    /* `send_message()` is used to send a message to a specific peer. The
    function will be called with a `std::ostream&` that leads to the peer in
    question. `send_message()` can be called on any thread. It may block. */
    void send_message(peer_id_t, const boost::function<void(std::ostream &)> &);

#ifndef NDEBUG
    void assert_connection_thread(peer_id_t peer);
#else
    void assert_connection_thread(UNUSED peer_id_t peer) { }
#endif  // ndef NDEBUG

private:
    class connection_t {
    public:
        streamed_tcp_conn_t *conn;
        /* `connection_t` contains a `peer_address_t` so that we can call
        `get_everybody()` on any thread. Otherwise, we would have to go
        cross-thread to access the routing table. */
        peer_address_t address;
        mutex_t send_mutex;
    };

    class thread_info_t {
    public:
        /* `thread_info->connection_map` holds open connections to other peers.
        It's the same on every thread. It has an entry for every peer that we
        are fully and officially connected to, not including us.  That means
        it's a subset of the nodes in `routing_table`.  */
        std::map<peer_id_t, connection_t *> connection_map;
        mutex_assertion_t lock;
        publisher_controller_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > > publisher;
    };

    /* We listen for new connections from other peers. (The reason `listener` is
    in a `boost::scoped_ptr` is so that we can stop listening at the beginning
    of our destructor.) */
    boost::scoped_ptr<streamed_tcp_listener_t> listener;
    void on_new_connection(boost::scoped_ptr<streamed_tcp_conn_t> &);

    void join_blocking(peer_address_t address, boost::optional<peer_id_t>, auto_drainer_t::lock_t);

    /* `handle()` takes an `auto_drainer_t::lock_t` so that we never shut down
    while there are still running instances of `handle()`. */
    void handle(streamed_tcp_conn_t *c,
        boost::optional<peer_id_t> expected_id,
        boost::optional<peer_address_t> expected_address,
        auto_drainer_t::lock_t);

    /* Whenever we receive a message, we spawn a new coroutine running
    `on_message()`. Its arguments are the peer we received the message from, a
    `std::istream&` from that peer, and a function to call when we're done
    reading the message off the stream. `on_message()` should read the message,
    call the function, then perform whatever action the message requires. This
    way, the next message can be read off the socket as soon as possible. 
    `connectivity_cluster_t` may run `on_message()` on any thread. `on_message`
    may call `on_done` without consuming the whole message, in which case it is
    `connectivity_cluster_t`'s responsibility to make sure the bytes get
    consumed. */
    boost::function<void(peer_id_t, std::istream &, const boost::function<void()> &)> on_message;

    /* `me` is our `peer_id_t`. `routing_table` is all the peers we can
    currently access and their addresses. Peers that are in the process of
    connecting or disconnecting may be in `routing_table` but not in
    `connections`. */
    const peer_id_t me;
    const peer_address_t me_address;
    std::map<peer_id_t, peer_address_t> routing_table;

    one_per_thread_t<thread_info_t> thread_info;

    static void ping_connection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb);
    void set_a_connection_entry_and_ping_connection_watchers(int target_thread, peer_id_t other_id, connection_t *connection);
    static void ping_disconnection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb);
    void erase_a_connection_entry_and_ping_disconnection_watchers(int target_thread, peer_id_t other_id);

    /* Writes to `routing_table` and `connections` are protected by this mutex
    so we never get redundant connections to the same peer. */
    mutex_t new_connection_mutex;

    rng_t rng;

    /* This makes sure all the connections are dead before we shut down. It must
    be the last thing so that none of our other member variables get destroyed
    before its destructor returns. */
    auto_drainer_t drainer;

    DISABLE_COPYING(connectivity_cluster_t);
};

struct disconnect_watcher_t : public signal_t {
    disconnect_watcher_t(connectivity_cluster_t *, peer_id_t);
private:
    void on_disconnect(peer_id_t);
    connectivity_cluster_t::peers_list_subscription_t subs;
    peer_id_t peer;
};

#endif /* __RPC_CONNECTIVITY_CONNECTIVITY_HPP__ */
