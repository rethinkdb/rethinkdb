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
    class run_t {
    public:
        run_t(connectivity_cluster_t *parent,
            int port,
            message_handler_t *message_handler) THROWS_NOTHING;

        /* Attaches the cluster this node is part of to another existing
        cluster. May only be called on home thread. Returns immediately (it does
        its work in the background). */
        void join(peer_address_t) THROWS_NOTHING;

    private:
        friend class connectivity_cluster_t;

        class connection_entry_t : public home_thread_mixin_t {
        public:
            /* The constructor registers us in every thread's `connection_map`;
            the destructor deregisters us. Both also notify all subscribers. */
            connection_entry_t(run_t *, peer_id_t, streamed_tcp_conn_t *, peer_address_t) THROWS_NOTHING;
            ~connection_entry_t() THROWS_NOTHING;

            /* NULL for our "connection" to ourself */
            streamed_tcp_conn_t *conn;

            /* `connection_t` contains a `peer_address_t` so that we can call
            `get_peers_list()` on any thread. Otherwise, we would have to go
            cross-thread to access the routing table. */
            peer_address_t address;

            /* Unused for our connection to ourself */
            mutex_t send_mutex;

            boost::uuids::uuid session_id;

        private:
            void install_this(int target_thread) THROWS_NOTHING;
            void uninstall_this(int target_thread) THROWS_NOTHING;

            /* We only hold this information so we can deregister ourself */
            run_t *parent;
            peer_id_t peer;

            /* These are one-per-thread; we destroy them as we deregister
            ourselves on each thread. Things that want to send a message via
            this `connection_entry_t` acquire the drainer for their thread when
            they look us up in `thread_info_t::connection_map`. */
            boost::scoped_array<boost::scoped_ptr<auto_drainer_t> > drainers;
        };

        /* Sets a variable to a value in its constructor; sets it to NULL in its
        destructor. This is kind of silly. The reason we need it is that we need
        the variable to be set before the constructors for some other fields of
        the `run_t` are constructed. */
        class variable_setter_t {
        public:
            variable_setter_t(run_t **var, run_t *val) THROWS_NOTHING : variable(var), value(val) {
                rassert(*variable == NULL);
                *variable = value;
            }
            ~variable_setter_t() THROWS_NOTHING {
                rassert(*variable == value);
                *variable = NULL;
            }
        private:
            run_t **variable;
            run_t *value;
        };

        void on_new_connection(boost::scoped_ptr<streamed_tcp_conn_t> &, auto_drainer_t::lock_t) THROWS_NOTHING;

        /* `connectivity_cluster_t::join_blocking()` is spawned in a new
        coroutine by `connectivity_cluster_t::join()`. It's also run by
        `connectivity_cluster_t::handle()` when we hear about a new peer from a
        peer we are connected to. */
        void join_blocking(peer_address_t address, boost::optional<peer_id_t>, auto_drainer_t::lock_t) THROWS_NOTHING;

        /* `handle()` takes an `auto_drainer_t::lock_t` so that we never shut
        down while there are still running instances of `handle()`. It's
        responsible for the entire lifetime of an intra-cluster TCP connection.
        It handles the handshake, exchanging node maps, sending out the
        connect-notification, receiving messages from the peer until it
        disconnects or we are shut down, and sending out the
        disconnect-notification. */
        void handle(streamed_tcp_conn_t *c,
            boost::optional<peer_id_t> expected_id,
            boost::optional<peer_address_t> expected_address,
            auto_drainer_t::lock_t) THROWS_NOTHING;

        connectivity_cluster_t *parent;

        message_handler_t *message_handler;

        /* `routing_table` is all the peers we can currently access and their
        addresses. Peers that are in the process of connecting or disconnecting
        may be in `routing_table` but not in
        `parent->thread_info.get()->connection_map`. */
        std::map<peer_id_t, peer_address_t> routing_table;

        /* Writes to `routing_table` are protected by this mutex so we never get
        redundant connections to the same peer. */
        mutex_t new_connection_mutex;

        variable_setter_t register_us_with_parent;

        map_insertion_sentry_t<peer_id_t, peer_address_t> routing_table_entry_for_ourself;
        connection_entry_t connection_to_ourself;

        /* For picking random threads */
        rng_t rng;

        auto_drainer_t drainer;

        /* This must be destroyed before `drainer` is. */
        streamed_tcp_listener_t listener;
    };

    connectivity_cluster_t() THROWS_NOTHING;
    ~connectivity_cluster_t() THROWS_NOTHING;

    /* `connectivity_service_t` public methods: */
    peer_id_t get_me() THROWS_NOTHING;
    std::set<peer_id_t> get_peers_list() THROWS_NOTHING;
    boost::uuids::uuid get_connection_session_id(peer_id_t) THROWS_NOTHING;

    /* `message_service_t` public methods: */
    connectivity_service_t *get_connectivity_service() THROWS_NOTHING;
    void send_message(peer_id_t, const boost::function<void(std::ostream &)> &) THROWS_NOTHING;

    /* Other public methods: */

    /* Returns the address of the given peer. Fatal error if we are not
    connected to the peer. */
    peer_address_t get_peer_address(peer_id_t) THROWS_NOTHING;

private:
    friend class run_t;

    class thread_info_t {
    public:
        /* `connection_map` holds open connections to other peers. It's the same
        on every thread. It has an entry for every peer that we are fully and
        officially connected to, not including us.  That means it's a subset of
        the nodes in `run_t::routing_table`. It also holds an
        `auto_drainer_t::lock_t` for each connection; that way, the connection
        can make sure nobody acquires a lock on its `auto_drainer_t` after it
        removes itself from `connection_map`. */
        std::map<peer_id_t, std::pair<run_t::connection_entry_t *, auto_drainer_t::lock_t> > connection_map;

        mutex_assertion_t lock;

        publisher_controller_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > > publisher;
    };

    /* `connectivity_service_t` private methods: */
    mutex_assertion_t *get_peers_list_lock() THROWS_NOTHING;
    publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peers_list_publisher() THROWS_NOTHING;

    /* `me` is our `peer_id_t`. */
    const peer_id_t me;

    one_per_thread_t<thread_info_t> thread_info;

    run_t *current_run;

    DISABLE_COPYING(connectivity_cluster_t);
};

#endif /* __RPC_CONNECTIVITY_CLUSTER_HPP__ */
