#ifndef __RPC_DIRECTORY_READ_MANAGER_HPP__
#define __RPC_DIRECTORY_READ_MANAGER_HPP__

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "concurrency/fifo_enforcer.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"
#include "rpc/directory/read_view.hpp"

/* Because the directory manager is so complex, it's broken out into several
sub-components. `directory_read_manager_t` is the read-only sub-component of the
directory manager. It doesn't publish a directory value of its own; that's the
job of `directory_write_manager_t`. */

template<class metadata_t>
class directory_read_manager_t :
    public connectivity_service_t,
    public virtual directory_read_service_t,
    public home_thread_mixin_t,
    public message_handler_t
{
public:
    directory_read_manager_t(connectivity_service_t *super_connectivity_service) THROWS_NOTHING;
    ~directory_read_manager_t() THROWS_NOTHING;

    clone_ptr_t<directory_rview_t<metadata_t> > get_root_view() THROWS_NOTHING;

    /* `connectivity_service_t` interface */
    peer_id_t get_me() THROWS_NOTHING;
    std::set<peer_id_t> get_peers_list() THROWS_NOTHING;
    boost::uuids::uuid get_connection_session_id(peer_id_t) THROWS_NOTHING;

    /* `directory_read_service_t` interface */
    connectivity_service_t *get_connectivity_service() THROWS_NOTHING;

private:
    /* `get_root_view()` returns an instance of this. */
    class root_view_t : public directory_rview_t<metadata_t> {
    public:
        root_view_t *clone() THROWS_NOTHING;
        boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING;
        directory_read_service_t *get_directory_service() THROWS_NOTHING;
    private:
        friend class directory_read_manager_t;
        explicit root_view_t(directory_read_manager_t *) THROWS_NOTHING;
        directory_read_manager_t *parent;
    };

    class thread_peer_info_t {
    public:
        thread_peer_info_t(const metadata_t &md) THROWS_NOTHING : peer_value(md) { }
        rwi_lock_assertion_t peer_value_lock;
        metadata_t peer_value;
        publisher_controller_t<
                boost::function<void()>
                > peer_value_publisher;
    };

    class thread_info_t {
    public:
        fifo_enforcer_sink_t propagation_fifo_sink;
        rwi_lock_assertion_t peers_list_lock;
        publisher_controller_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > > peers_list_publisher;
        boost::ptr_map<peer_id_t, thread_peer_info_t> peers_list;
    };

    /* A `session_t` is created for every peer that connects to us and destroyed
    when they disconnect. A new `session_t` is created if they reconnect. */
    class session_t {
    public:
        session_t(boost::uuids::uuid si) : session_id(si) { }
        /* We get this by calling `get_connection_session_id()` on the
        `connectivity_service_t` from `super_connectivity_service`. */
        boost::uuids::uuid session_id;
        cond_t got_initial_message;
        boost::scoped_ptr<fifo_enforcer_sink_t> metadata_fifo_sink;
        auto_drainer_t drainer;
    };

    /* Note that connection and initialization are different things. Connection
    means that we can communicate with another peer at a low level.
    Initialization means that we have received initial metadata from the peer.
    Peers only show up on our own `connectivity_service_t` once initialization
    has happened. */

    /* These will be called in a blocking fashion by the connectivity service
    (or message service, in the case of `on_message()`) */
    void on_connect(peer_id_t peer) THROWS_NOTHING;
    void on_message(peer_id_t, std::istream &) THROWS_NOTHING;
    void on_disconnect(peer_id_t peer) THROWS_NOTHING;

    /* These are meant to be spawned in new coroutines */
    void propagate_initialization(peer_id_t peer, boost::uuids::uuid session_id, metadata_t new_value, fifo_enforcer_source_t::state_t metadata_fifo_state, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING;
    void propagate_update(peer_id_t peer, boost::uuids::uuid session_id, metadata_t new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING;
    void interrupt_updates_and_free_session(session_t *session, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;

    /* These are meant to be spawned in new coroutines also. */
    void propagate_initialize_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &initial_value, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;
    void propagate_update_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &new_value, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;
    void propagate_disconnect_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;

    /* These are used as callbacks for `publisher_controller_t::publish()` */
    static void ping_connection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING;
    static void ping_value_watcher(const boost::function<void()> &callback) THROWS_NOTHING;
    static void ping_disconnection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING;

    /* `connectivity_service_t` methods */
    rwi_lock_assertion_t *get_peers_list_lock() THROWS_NOTHING;
    publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peers_list_publisher() THROWS_NOTHING;

    /* `directory_rservice_t` methods */
    rwi_lock_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING;
    publisher_t<
            boost::function<void()>
            > *get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;

    /* The connectivity service telling us which peers are connected */
    connectivity_service_t *super_connectivity_service;

    one_per_thread_t<thread_info_t> thread_info;

    /* Used to make sure that are not reordered en route to other threads of
    this machine. */
    fifo_enforcer_source_t propagation_fifo_source;

    boost::ptr_map<peer_id_t, session_t> sessions;

    /* Instances of `interrupt_updates_and_free_global_peer_info()` and
    `propagate_*_on_thread()` hold a lock on this drainer. */
    auto_drainer_t global_drainer;

    /* Instances of `propagate_initialization()` and `propagate_update()` each
    hold a lock on one of these drainers. */
    one_per_thread_t<auto_drainer_t> per_thread_drainers;

    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
};

#include "rpc/directory/read_manager.tcc"

#endif /* __RPC_DIRECTORY_READ_MANAGER_HPP__ */
