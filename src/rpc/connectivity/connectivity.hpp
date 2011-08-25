#ifndef __RPC_CONNECTIVITY_CONNECTIVITY_HPP__
#define __RPC_CONNECTIVITY_CONNECTIVITY_HPP__

#include <map>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/utility.hpp>
#include "arch/streamed_tcp.hpp"
#include "arch/address.hpp"
#include "utils.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/auto_drainer.hpp"

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
    boost::uuids::uuid uuid;
    peer_id_t(boost::uuids::uuid u) : uuid(u) { }

    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, UNUSED const unsigned int version) {
        ar & uuid;
    }
};

/* `event_watcher_t` is used to watch for any node joining or leaving the
cluster. `connect_watcher_t` and `disconnect_watcher_t` are used to watch for a
specific peer connecting or disconnecting. */

struct connectivity_cluster_t;

struct event_watcher_t : public intrusive_list_node_t<event_watcher_t> {
    event_watcher_t(connectivity_cluster_t *);
    virtual ~event_watcher_t();
    virtual void on_connect(peer_id_t) = 0;
    virtual void on_disconnect(peer_id_t) = 0;
private:
    connectivity_cluster_t *cluster;
};

struct connect_watcher_t : public signal_t, private event_watcher_t {
    connect_watcher_t(connectivity_cluster_t *, peer_id_t);
private:
    void on_connect(peer_id_t);
    void on_disconnect(peer_id_t);
    peer_id_t peer;
};

struct disconnect_watcher_t : public signal_t, private event_watcher_t {
    disconnect_watcher_t(connectivity_cluster_t *, peer_id_t);
private:
    void on_connect(peer_id_t);
    void on_disconnect(peer_id_t);
    peer_id_t peer;
};

struct connectivity_cluster_t :
    public home_thread_mixin_t
{
public:
    /* Attaches the cluster this node is part of to another existing cluster.
    May only be called on home thread. */
    void join(peer_address_t);

    /* `get_me()` returns the `peer_id_t` for this cluster node.
    `get_everybody()` returns all the currently-accessible peers in the
    cluster and their addresses, including us. These may only be called on the
    home thread. */
    peer_id_t get_me();
    std::map<peer_id_t, peer_address_t> get_everybody();

protected:
    /* Creating a new cluster node, and connecting one cluster to another
    cluster */
    connectivity_cluster_t(int port);
    virtual ~connectivity_cluster_t();

    /* TODO: We should have a better mechanism for sending messages to ourself.
    Right now, they get serialized and then deserialized. If we did it more
    efficiently, we wouldn't have to special-case messages to local mailboxes on
    the higher levels. */

    /* `send_message()` is used to send a message to a specific peer. The
    function will be called with a `std::ostream&` that leads to the peer in
    question. `send_message()` can be called on any thread. It may block. */
    void send_message(peer_id_t, boost::function<void(std::ostream&)>);

    /* Whenever we receive a message, we spawn a new coroutine running
    `on_message()`. Its arguments are the peer we received the message from, a
    `std::istream&` from that peer, and a function to call when we're done
    reading the message off the stream. `on_message()` should read the message,
    call the function, then perform whatever action the message requires. This
    way, the next message can be read off the socket as soon as possible.
    `connectivity_cluster_t` may run `on_message()` on any thread. */
    virtual void on_message(peer_id_t, std::istream&, boost::function<void()>&) = 0;

private:
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

    /* `me` is our `peer_id_t`. `routing_table` is all the peers we can
    currently access and their addresses. Peers that are in the process of
    connecting or disconnecting may be in `routing_table` but not in
    `connections`. */
    peer_id_t me;
    std::map<peer_id_t, peer_address_t> routing_table;

    /* `connections` holds open connections to other peers. It has an entry for
    every peer that we are fully and officially connected to, not including us.
    That means it's a subset of the nodes in `routing_table`. */
    struct connection_t {
        streamed_tcp_conn_t *conn;
        mutex_t send_mutex;
    };
    std::map<peer_id_t, connection_t*> connections;

    /* Writes to `routing_table` and `connections` are protected by this mutex
    so we never get redundant connections to the same peer. */
    mutex_t new_connection_mutex;

    /* List of everybody watching for connectivity events. `watchers_mutex` is
    so nobody updates `watchers` while we're iterating over it. */
    friend class event_watcher_t;
    intrusive_list_t<event_watcher_t> watchers;
    mutex_t watchers_mutex;

    /* This makes sure all the connections are dead before we shut down */
    auto_drainer_t drainer;

    DISABLE_COPYING(connectivity_cluster_t);
};

#endif /* __RPC_CONNECTIVITY_CONNECTIVITY_HPP__ */
