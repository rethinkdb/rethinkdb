#ifndef RPC_DIRECTORY_READ_MANAGER_HPP_
#define RPC_DIRECTORY_READ_MANAGER_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>

#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"

template<class metadata_t>
class directory_read_manager_t :
    public home_thread_mixin_t,
    public message_handler_t,
    private peers_list_callback_t {
public:
    explicit directory_read_manager_t(connectivity_service_t *connectivity_service) THROWS_NOTHING;
    ~directory_read_manager_t() THROWS_NOTHING;

    clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > > get_root_view() THROWS_NOTHING {
        return variable.get_watchable();
    }

private:
    /* A `session_t` is created for every peer that connects to us and destroyed
    when they disconnect. A new `session_t` is created if they reconnect. */
    class session_t {
    public:
        explicit session_t(uuid_t si) : session_id(si) { }
        /* We get this by calling `get_connection_session_id()` on the
        `connectivity_service_t` from `super_connectivity_service`. */
        const uuid_t session_id;
        cond_t got_initial_message;
        boost::scoped_ptr<fifo_enforcer_sink_t> metadata_fifo_sink;
        auto_drainer_t drainer;
    };

    /* Note that connection and initialization are different things. Connection
    means that we can send messages to the peer. Initialization means that we
    have received initial metadata from the peer. Peers only show up in the
    `watchable_t` once initialization is complete. */

    /* These will be called in a blocking fashion by the connectivity service
    (or message service, in the case of `on_message()`) */
    void on_message(peer_id_t, read_stream_t *) THROWS_NOTHING;
    void on_connect(peer_id_t peer) THROWS_NOTHING;
    void on_disconnect(peer_id_t peer) THROWS_NOTHING;

    /* These are meant to be spawned in new coroutines */
    void propagate_initialization(peer_id_t peer, uuid_t session_id, metadata_t new_value, fifo_enforcer_source_t::state_t metadata_fifo_state, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING;
    void propagate_update(peer_id_t peer, uuid_t session_id, metadata_t new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING;
    void interrupt_updates_and_free_session(session_t *session, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING;

    /* The connectivity service telling us which peers are connected */
    connectivity_service_t *connectivity_service;

    watchable_variable_t<std::map<peer_id_t, metadata_t> > variable;
    mutex_assertion_t variable_lock;

    boost::ptr_map<peer_id_t, session_t> sessions;

    /* Instances of `propagate_initialization()` and `propagate_update()` hold
    a lock on one of these drainers. */
    one_per_thread_t<auto_drainer_t> per_thread_drainers;

    /* Instances of `interrupt_updates_and_free_session()` and hold a lock on
    this drainer. */
    auto_drainer_t global_drainer;

    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
};

#include "rpc/directory/read_manager.tcc"

#endif /* RPC_DIRECTORY_READ_MANAGER_HPP_ */
