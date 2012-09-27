#include "rpc/directory/read_manager.hpp"

#include <map>

#include "concurrency/wait_any.hpp"
#include "containers/archive/archive.hpp"

template<class metadata_t>
directory_read_manager_t<metadata_t>::directory_read_manager_t(connectivity_service_t *conn_serv) THROWS_NOTHING :
    connectivity_service(conn_serv),
    variable(std::map<peer_id_t, metadata_t>()),
    connectivity_subscription(this) {
    connectivity_service_t::peers_list_freeze_t freeze(connectivity_service);
    guarantee(connectivity_service->get_peers_list().empty());
    connectivity_subscription.reset(connectivity_service, &freeze);
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::~directory_read_manager_t() THROWS_NOTHING {
    guarantee(connectivity_service->get_peers_list().empty());
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_connect(peer_id_t peer) THROWS_NOTHING {
    assert_thread();
    std::pair<typename boost::ptr_map<peer_id_t, session_t>::iterator, bool> res = sessions.insert(peer, new session_t(connectivity_service->get_connection_session_id(peer)));
    guarantee(res.second);
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_message(peer_id_t source_peer, read_stream_t *s) THROWS_NOTHING {
    uint8_t code = 0;
    {
        int res = deserialize(s, &code);
        guarantee(!res);  // We do unreachable below...
    }

    switch (code) {
        case 'I': {
            /* Initial message from another peer */
            metadata_t initial_value;
            fifo_enforcer_state_t metadata_fifo_state;
            {
                int res = deserialize(s, &initial_value);
                guarantee(!res);  // In the spirit of unreachable...
                res = deserialize(s, &metadata_fifo_state);
                guarantee(!res);
            }

            /* Spawn a new coroutine because we might not be on the home thread
            and `on_message()` isn't supposed to block very long */
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_initialization, this,
                source_peer, connectivity_service->get_connection_session_id(source_peer),
                initial_value, metadata_fifo_state,
                auto_drainer_t::lock_t(per_thread_drainers.get())));

            break;
        }

        case 'U': {
            /* Update from another peer */
            metadata_t new_value;
            fifo_enforcer_write_token_t metadata_fifo_token;
            {
                int res = deserialize(s, &new_value);
                guarantee(!res);  // In the spirit of unreachable...
                res = deserialize(s, &metadata_fifo_token);
                guarantee(!res);  // In the spirit of unreachable...

                // TODO Don't fail catastrophically just because there's bad data on the stream.
            }

            /* Spawn a new coroutine because we might not be on the home thread
            and `on_message()` isn't supposed to block very long */
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_update, this,
                source_peer, connectivity_service->get_connection_session_id(source_peer),
                new_value, metadata_fifo_token,
                auto_drainer_t::lock_t(per_thread_drainers.get())));

            break;
        }

        default: unreachable();
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_disconnect(peer_id_t peer) THROWS_NOTHING {
    ASSERT_FINITE_CORO_WAITING;
    assert_thread();

    /* Remove the `global_peer_info_t` object from the table */
    typename boost::ptr_map<peer_id_t, session_t>::iterator it = sessions.find(peer);
    guarantee(it != sessions.end());
    session_t *session_to_destroy = sessions.release(it).release();

    bool got_initialization = session_to_destroy->got_initial_message.is_pulsed();

    /* Start interrupting any running calls to `propagate_update()`. We need to
    explicitly interrupt them rather than letting them finish on their own
    because, if the network reordered messages, they might wait indefinitely for
    a message that will now never come. */
    coro_t::spawn_sometime(boost::bind(
        &directory_read_manager_t::interrupt_updates_and_free_session, this,
        session_to_destroy,
        auto_drainer_t::lock_t(&global_drainer)));

    /* Notify that the peer has disconnected */
    if (got_initialization) {
        mutex_assertion_t::acq_t acq(&variable_lock);
        std::map<peer_id_t, metadata_t> map = variable.get_watchable()->get();
        size_t num_erased = map.erase(peer);
        guarantee(num_erased == 1);
        variable.set_value(map);
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_initialization(peer_id_t peer, uuid_t session_id, metadata_t initial_value, fifo_enforcer_state_t metadata_fifo_state, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING {
    per_thread_keepalive.assert_is_holding(per_thread_drainers.get());
    on_thread_t thread_switcher(home_thread());

    ASSERT_FINITE_CORO_WAITING;
    /* Check to make sure that the peer didn't die while we were coming from the
    thread on which `on_message()` was run */
    typename boost::ptr_map<peer_id_t, session_t>::iterator it = sessions.find(peer);
    if (it == sessions.end()) {
        /* The peer disconnected since we got the message; ignore. */
        return;
    }
    session_t *session = (*it).second;
    if (session->session_id != session_id) {
        /* The peer disconnected and then reconnected since we got the message;
        ignore. */
        return;
    }

    /* Notify that the peer has connected */
    {
        mutex_assertion_t::acq_t acq(&variable_lock);
        std::map<peer_id_t, metadata_t> map = variable.get_watchable()->get();

        std::pair<typename std::map<peer_id_t, metadata_t>::iterator, bool> res
            = map.insert(std::make_pair(peer, initial_value));
        guarantee(res.second);

        variable.set_value(map);
    }

    /* Create a metadata FIFO sink and pulse the `got_initial_message` cond so
    that instances of `propagate_update()` can proceed */
    // TODO: Do we want this .reset() here?  It is not easily provable
    // that it'll be initialized only once.
    session->metadata_fifo_sink.reset();
    session->metadata_fifo_sink.init(new fifo_enforcer_sink_t(metadata_fifo_state));
    session->got_initial_message.pulse();
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_update(peer_id_t peer, uuid_t session_id, metadata_t new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING {
    per_thread_keepalive.assert_is_holding(per_thread_drainers.get());
    on_thread_t thread_switcher(home_thread());

    /* Check to make sure that the peer didn't die while we were coming from the
    thread on which `on_message()` was run */
    typename boost::ptr_map<peer_id_t, session_t>::iterator it = sessions.find(peer);
    if (it == sessions.end()) {
        /* The peer disconnected since we got the message; ignore. */
        return;
    }
    session_t *session = (*it).second;
    if (session->session_id != session_id) {
        /* The peer disconnected and then reconnected since we got the message;
        ignore. */
        return;
    }
    auto_drainer_t::lock_t session_keepalive(&session->drainer);

    try {
        /* Wait until we got an initialization message from this peer */
        {
            wait_any_t waiter(&session->got_initial_message, session_keepalive.get_drain_signal());
            waiter.wait_lazily_unordered();
            if (session_keepalive.get_drain_signal()->is_pulsed()) {
                throw interrupted_exc_t();
            }
        }

        /* Exit this peer's `metadata_fifo_sink` so that we perform the updates
        in the same order as they were performed at the source. */
        fifo_enforcer_sink_t::exit_write_t fifo_exit(session->metadata_fifo_sink.get(),
                                                     metadata_fifo_token);
        wait_interruptible(&fifo_exit, session_keepalive.get_drain_signal());

        {
            mutex_assertion_t::acq_t acq(&variable_lock);
            std::map<peer_id_t, metadata_t> map = variable.get_watchable()->get();

            typename std::map<peer_id_t, metadata_t>::iterator var_it = map.find(peer);
            if (var_it == map.end()) {
                guarantee(!std_contains(sessions, peer));
                //The session was deleted we can ignore this update.
                return;
            }
            var_it->second = new_value;
            variable.set_value(map);
        }
    } catch (interrupted_exc_t) {
        /* Here's what happened: `on_disconnect()` was called for the peer. It
        spawned `interrupt_updates_and_free_session()`, which deleted the
        `session_t` for this peer, thereby calling the destructor for the peer's
        `auto_drainer_t`, for which `session_keepalive` is an
        `auto_drainer_t::lock_t`. `session_keepalive.get_drain_signal()` was
        pulsed, so an `interrupted_exc_t` was thrown.

        The peer has disconnected, so we can safely ignore this update; its
        entry in the thread-local peer table will be deleted soon anyway. */
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::interrupt_updates_and_free_session(session_t *session, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);

    /* This must be done in a separate coroutine because `session_t` has an
    `auto_drainer_t`, and the `auto_drainer_t` destructor can block. */
    delete session;
}

template class directory_read_manager_t<int>;

#include "clustering/administration/metadata.hpp"
template class directory_read_manager_t<cluster_directory_metadata_t>;

#include "mock/test_cluster_group.hpp"
#include "mock/dummy_protocol.hpp"
template class directory_read_manager_t<mock::test_cluster_directory_t<mock::dummy_protocol_t> >;

#include "clustering/reactor/directory_echo.hpp"
template class directory_read_manager_t<directory_echo_wrapper_t<std::string> >;
