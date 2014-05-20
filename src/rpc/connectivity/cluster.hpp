// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_CLUSTER_HPP_
#define RPC_CONNECTIVITY_CLUSTER_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "containers/archive/tcp_conn_stream.hpp"
#include "containers/map_sentries.hpp"
#include "containers/uuid.hpp"
#include "perfmon/perfmon.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"
#include "utils.hpp"

namespace boost {
template <class> class optional;
}

class co_semaphore_t;
class heartbeat_manager_t;

class peer_address_set_t {
public:
    size_t erase(const peer_address_t &addr) {
        size_t erased = 0;
        for (std::vector<peer_address_t>::iterator it = vec.begin(); it != vec.end(); ++it) {
            if (*it == addr) {
                vec.erase(it);
                ++erased;
                break;
            }
        }
        return erased;
    }
    size_t size() const { return vec.size(); }
    typedef std::vector<peer_address_t>::const_iterator iterator;
    iterator begin() const { return vec.begin(); }
    iterator end() const { return vec.end(); }
    iterator find(const peer_address_t &addr) const {
        return std::find(vec.begin(), vec.end(), addr);
    }
    iterator insert(const peer_address_t &addr) {
        guarantee(find(addr) == vec.end());
        return vec.insert(vec.end(), addr);
    }
    bool empty() const { return vec.empty(); }
private:
    std::vector<peer_address_t> vec;
};

class connectivity_cluster_t :
    public connectivity_service_t,
    public message_service_t,
    public home_thread_mixin_debug_only_t
{
public:
    static const std::string cluster_proto_header;
    static const std::string cluster_version_string;
    static const std::string cluster_arch_bitsize;
    static const std::string cluster_build_mode;

    class run_t {
    public:
        run_t(connectivity_cluster_t *parent,
              const std::set<ip_address_t> &local_addresses,
              const peer_address_t &canonical_addresses,
              int port,
              message_handler_t *message_handler,
              int client_port,
              heartbeat_manager_t *_heartbeat_manager)
            THROWS_ONLY(address_in_use_exc_t, tcp_socket_exc_t);

        ~run_t();

        /* Attaches the cluster this node is part of to another existing
        cluster. May only be called on home thread. Returns immediately (it does
        its work in the background). */
        void join(const peer_address_t &address) THROWS_NOTHING;

        std::set<ip_and_port_t> get_ips() const;
        int get_port();

    private:
        friend class connectivity_cluster_t;

        class connection_entry_t : public home_thread_mixin_debug_only_t {
        public:
            /* The constructor registers us in every thread's `connection_map`;
            the destructor deregisters us. Both also notify all subscribers. */
            connection_entry_t(run_t *, peer_id_t, tcp_conn_stream_t *,
                               const peer_address_t &peer) THROWS_NOTHING;
            ~connection_entry_t() THROWS_NOTHING;

            /* NULL for our "connection" to ourself */
            tcp_conn_stream_t *conn;

            /* `connection_t` contains the addresses so that we can call
            `get_peers_list()` on any thread. Otherwise, we would have to go
            cross-thread to access the routing table. */
            peer_address_t address;

            /* Unused for our connection to ourself */
            mutex_t send_mutex;

            uuid_u session_id;

            perfmon_collection_t pm_collection;
            perfmon_sampler_t pm_bytes_sent;
            perfmon_membership_t pm_collection_membership, pm_bytes_sent_membership;

        private:
            /* We only hold this information so we can deregister ourself */
            run_t *parent;
            peer_id_t peer;

            struct entry_installation_t {
                auto_drainer_t drainer_;
                connection_entry_t *that_;

                explicit entry_installation_t(connection_entry_t *that);
                ~entry_installation_t();
            };

            /* These are one-per-thread; we destroy them as we deregister
            ourselves on each thread. Things that want to send a message via
            this `connection_entry_t` acquire the drainer for their thread when
            they look us up in `thread_info_t::connection_map`. */
            scoped_ptr_t<one_per_thread_t<entry_installation_t> > entries;
        };

        /* Sets a variable to a value in its constructor; sets it to NULL in its
        destructor. This is kind of silly. The reason we need it is that we need
        the variable to be set before the constructors for some other fields of
        the `run_t` are constructed. */
        class variable_setter_t {
        public:
            variable_setter_t(run_t **var, run_t *val) : variable(var) , value(val) {
                guarantee(*variable == NULL);
                *variable = value;
            }

            ~variable_setter_t() THROWS_NOTHING {
                guarantee(*variable == value);
                *variable = NULL;
            }
        private:
            run_t **variable;
            run_t *value;
            DISABLE_COPYING(variable_setter_t);
        };

        void on_new_connection(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn, auto_drainer_t::lock_t lock) THROWS_NOTHING;

        /* `connectivity_cluster_t::connect_to_peer` is spawned for each known
        ip address of a peer which we want to connect to, all but one should
        fail */
        void connect_to_peer(const peer_address_t *addr,
                             int index,
                             boost::optional<peer_id_t> expected_id,
                             auto_drainer_t::lock_t drainer_lock,
                             bool *successful_join,
                             co_semaphore_t *rate_control) THROWS_NOTHING;

        /* `connectivity_cluster_t::join_blocking()` is spawned in a new
        coroutine by `connectivity_cluster_t::join()`. It's also run by
        `connectivity_cluster_t::handle()` when we hear about a new peer from a
        peer we are connected to. */
        void join_blocking(const peer_address_t hosts,
                           boost::optional<peer_id_t>,
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
            bool *successful_join) THROWS_NOTHING;

        connectivity_cluster_t *parent;

        message_handler_t *message_handler;

        heartbeat_manager_t *heartbeat_manager;

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
        connection_entry_t connection_to_ourself;

        /* For picking random threads */
        rng_t rng;

        auto_drainer_t drainer;

        /* This must be destroyed before `drainer` is. */
        scoped_ptr_t<tcp_listener_t> listener;

        /* A place to put our stats */
    };

    connectivity_cluster_t() THROWS_NOTHING;
    ~connectivity_cluster_t() THROWS_NOTHING;

    /* `connectivity_service_t` public methods: */
    peer_id_t get_me() THROWS_NOTHING;
    std::set<peer_id_t> get_peers_list() THROWS_NOTHING;
    uuid_u get_connection_session_id(peer_id_t) THROWS_NOTHING;

    /* `message_service_t` public methods: */
    connectivity_service_t *get_connectivity_service() THROWS_NOTHING;
    void send_message(peer_id_t, send_message_write_callback_t *callback) THROWS_NOTHING;
    void kill_connection(peer_id_t) THROWS_NOTHING;

    /* Other public methods: */

    /* Returns the address of the given peer. Fatal error if we are not
    connected to the peer. This function will result in the lookup of the
    peer's hostnames (if any) */
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

        rwi_lock_assertion_t lock;

        publisher_controller_t<peers_list_callback_t *> publisher;
    };

    /* `connectivity_service_t` private methods: */
    rwi_lock_assertion_t *get_peers_list_lock() THROWS_NOTHING;
    publisher_t<peers_list_callback_t *> *get_peers_list_publisher() THROWS_NOTHING;

    /* `me` is our `peer_id_t`. */
    const peer_id_t me;

    one_per_thread_t<thread_info_t> thread_info;

#ifndef NDEBUG
    rng_t debug_rng;
#endif

    run_t *current_run;

    perfmon_collection_t connectivity_collection;
    perfmon_membership_t stats_membership;

    DISABLE_COPYING(connectivity_cluster_t);
};

#endif /* RPC_CONNECTIVITY_CLUSTER_HPP_ */
