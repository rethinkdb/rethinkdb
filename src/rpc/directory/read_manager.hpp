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
directory manager. It doesn't publish a directory value of its own. */

template<class metadata_t>
class directory_read_manager_t :
    public connectivity_service_t,
    public virtual directory_read_service_t,
    public home_thread_mixin_t
{
public:
    directory_read_manager_t(message_read_service_t *sub_message_service) THROWS_NOTHING;
    ~directory_read_manager_t() THROWS_NOTHING;

    clone_ptr_t<directory_rview_t<metadata_t> > get_root_view() THROWS_NOTHING;

    /* `connectivity_service_t` interface */
    peer_id_t get_me() THROWS_NOTHING;
    std::set<peer_id_t> get_peers_list() THROWS_NOTHING;

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
        mutex_assertion_t peer_value_lock;
        metadata_t peer_value;
        publisher_controller_t<
                boost::function<void()>
                > peer_value_publisher;
    };

    class thread_info_t {
    public:
        fifo_enforcer_sink_t propagation_fifo_sink;
        mutex_assertion_t peers_list_lock;
        publisher_controller_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > > peers_list_publisher;
        boost::ptr_map<peer_id_t, thread_peer_info_t> peers_list;
    };

    class global_peer_info_t {
    public:
        cond_t got_initial_message;
        boost::scoped_ptr<fifo_enforcer_sink_t> metadata_fifo_sink;
        auto_drainer_t drainer;
    };

    /* Note that connection and initialization are different things. Connection
    means that we can communicate with another peer at a low level.
    Initialization means that we have received initial metadata from the peer.
    Peers only show up on our own `connectivity_service_t` once initialization
    has happened. */

    /* These will be called in a blocking fashion by the message service */
    void on_connect(peer_id_t peer) THROWS_NOTHING;
    void on_message(peer_id_t source, std::istream &stream) THROWS_NOTHING;
    void on_disconnect(peer_id_t peer) THROWS_NOTHING;

    /* These are meant to be spawned in new coroutines */
    void propagate_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t peer_keepalive) THROWS_NOTHING;
    void interrupt_updates_and_free_global_peer_info(global_peer_info_t *peer_info, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;

    /* These are meant to be spawned in new coroutines also. */
    void propagate_initialize_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &initial_value, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;
    void propagate_update_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &new_value, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;
    void propagate_disconnect_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;

    /* These are used as callbacks for `publisher_controller_t::publish()` */
    static void ping_connection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING;
    static void ping_value_watcher(const boost::function<void()> &callback) THROWS_NOTHING;
    static void ping_disconnection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING;

    /* `connectivity_service_t` methods */
    mutex_assertion_t *get_peers_list_lock() THROWS_NOTHING;
    publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peers_list_publisher() THROWS_NOTHING;

    /* `directory_rservice_t` methods */
    mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING;
    publisher_t<
            boost::function<void()>
            > *get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;

    /* The message service that we use to communicate with other peers */
    message_read_service_t *super_message_service;

    one_per_thread_t<thread_info_t> thread_info;

    /* This lock protects `propagation_fifo_source` and `peer_info`. */
    mutex_assertion_t mutex;

    /* Used to make sure that are not reordered en route to other threads of
    this machine. */
    fifo_enforcer_source_t propagation_fifo_source;

    boost::ptr_map<peer_id_t, global_peer_info_t> peer_info;

    auto_drainer_t global_drainer;

    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
    message_read_service_t::handler_registration_t message_handler_registration;
};

#include "rpc/directory/read_manager.tcc"

#endif /* __RPC_DIRECTORY_READ_MANAGER_HPP__ */
