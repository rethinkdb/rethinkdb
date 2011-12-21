#ifndef __RPC_CONNECTIVITY_CLUSTER_HPP__
#define __RPC_CONNECTIVITY_CLUSTER_HPP__

#include "errors.hpp"
#include <boost/optional.hpp>

#include "arch/streamed_tcp.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"

class connectivity_cluster_t :
    public connectivity_service_t,
    public message_service_t,
    public home_thread_mixin_t
{
public:
    connectivity_cluster_t(int port);
    ~connectivity_cluster_t();

    /* `connectivity_service_t` public methods: */
    peer_id_t get_me();
    std::set<peer_id_t> get_peers_list();

    /* `message_service_t` public methods: */
    connectivity_service_t *get_connectivity();
    void send_message(peer_id_t, const boost::function<void(std::ostream &)> &);
    void set_message_callback(
            const boost::function<void(
                peer_id_t source_peer,
                std::istream &stream_from_peer,
                const boost::function<void()> &call_when_done
                )> &callback
            );

    /* Other public methods: */

    /* Returns the address of the given peer. Fatal error if we are not
    connected to the peer. */
    peer_address_t get_peer_address(peer_id_t);

    /* Attaches the cluster this node is part of to another existing cluster.
    May only be called on home thread. */
    void join(peer_address_t);

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
        `get_peers_list()` on any thread. Otherwise, we would have to go
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

    static void ping_connection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb);
    void set_a_connection_entry_and_ping_connection_watchers(int target_thread, peer_id_t other_id, connection_t *connection);
    static void ping_disconnection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb);
    void erase_a_connection_entry_and_ping_disconnection_watchers(int target_thread, peer_id_t other_id);

    /* `connectivity_service_t` methods */
    mutex_assertion_t *get_peers_list_lock();
    publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peers_list_publisher();

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

#endif /* __RPC_CONNECTIVITY_CLUSTER_HPP__ */
