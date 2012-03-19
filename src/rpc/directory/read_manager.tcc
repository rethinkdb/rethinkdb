#include "errors.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "concurrency/wait_any.hpp"

template<class metadata_t>
directory_read_manager_t<metadata_t>::directory_read_manager_t(connectivity_service_t *super) THROWS_NOTHING :
    super_connectivity_service(super),
    connectivity_subscription(
        boost::bind(&directory_read_manager_t::on_connect, this, _1),
        boost::bind(&directory_read_manager_t::on_disconnect, this, _1)
        )
{
    connectivity_service_t::peers_list_freeze_t freeze(super_connectivity_service);
    rassert(super_connectivity_service->get_peers_list().empty());
    connectivity_subscription.reset(super_connectivity_service, &freeze);
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::~directory_read_manager_t() THROWS_NOTHING {
    rassert(super_connectivity_service->get_peers_list().empty());
}

template<class metadata_t>
clone_ptr_t<directory_rview_t<metadata_t> > directory_read_manager_t<metadata_t>::get_root_view() THROWS_NOTHING {
    return clone_ptr_t<directory_rview_t<metadata_t> >(new root_view_t(this));
}

template<class metadata_t>
peer_id_t directory_read_manager_t<metadata_t>::get_me() THROWS_NOTHING {
    return super_connectivity_service->get_me();
}

template<class metadata_t>
std::set<peer_id_t> directory_read_manager_t<metadata_t>::get_peers_list() THROWS_NOTHING {
    std::set<peer_id_t> peers_list;
    for (typename boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it = thread_info.get()->peers_list.begin();
            it != thread_info.get()->peers_list.end(); it++) {
        peers_list.insert((*it).first);
    }
    return peers_list;
}

template<class metadata_t>
boost::uuids::uuid directory_read_manager_t<metadata_t>::get_connection_session_id(peer_id_t peer) THROWS_NOTHING {
    rassert(thread_info.get()->peers_list.count(peer) != 0);
    return super_connectivity_service->get_connection_session_id(peer);
}

template<class metadata_t>
connectivity_service_t *directory_read_manager_t<metadata_t>::get_connectivity_service() THROWS_NOTHING {
    /* We are the `connectivity_service_t` for our own
    `directory_read_service_t`. */
    return this;
}

template<class metadata_t>
typename directory_read_manager_t<metadata_t>::root_view_t *directory_read_manager_t<metadata_t>::root_view_t::clone() const THROWS_NOTHING {
    return new root_view_t(parent);
}

template<class metadata_t>
boost::optional<metadata_t> directory_read_manager_t<metadata_t>::root_view_t::get_value(peer_id_t peer) THROWS_NOTHING {
    typename boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it =
        parent->thread_info.get()->peers_list.find(peer);
    if (it == parent->thread_info.get()->peers_list.end()) {
        return boost::optional<metadata_t>();
    } else {
        return boost::optional<metadata_t>((*it).second->peer_value);
    }
}

template<class metadata_t>
directory_read_service_t *directory_read_manager_t<metadata_t>::root_view_t::get_directory_service() THROWS_NOTHING {
    return parent;
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::root_view_t::root_view_t(directory_read_manager_t *p) THROWS_NOTHING :
    parent(p) { }

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_connect(peer_id_t peer) THROWS_NOTHING {
    assert_thread();
    rassert(sessions.count(peer) == 0);
    sessions.insert(peer, new session_t(
        super_connectivity_service->get_connection_session_id(peer)
        ));
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_message(peer_id_t source_peer, std::istream &stream) THROWS_NOTHING {
    switch (stream.get()) {
        case 'I': {
            /* Initial message from another peer */
            metadata_t initial_value;
            fifo_enforcer_source_t::state_t metadata_fifo_state;
            {
                boost::archive::binary_iarchive archive(stream);
                archive >> initial_value;
                archive >> metadata_fifo_state;
            }

            /* Spawn a new coroutine because we might not be on the home thread
            and `on_message()` isn't supposed to block very long */
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_initialization, this,
                source_peer, super_connectivity_service->get_connection_session_id(source_peer),
                initial_value, metadata_fifo_state,
                auto_drainer_t::lock_t(per_thread_drainers.get())
                ));

            break;
        }

        case 'U': {
            /* Update from another peer */
            metadata_t new_value;
            fifo_enforcer_write_token_t metadata_fifo_token;
            {
                boost::archive::binary_iarchive archive(stream);
                archive >> new_value;
                archive >> metadata_fifo_token;
            }

            /* Spawn a new coroutine because we might not be on the home thread
            and `on_message()` isn't supposed to block very long */
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_update, this,
                source_peer, super_connectivity_service->get_connection_session_id(source_peer),
                new_value, metadata_fifo_token,
                auto_drainer_t::lock_t(per_thread_drainers.get())
                ));

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
    rassert(sessions.count(peer) == 1);
    session_t *session_to_destroy = sessions.release(sessions.find(peer)).release();

    bool got_initialization = session_to_destroy->got_initial_message.is_pulsed();

    /* Start interrupting any running calls to `propagate_update()`. We need to
    explicitly interrupt them rather than letting them finish on their own
    because, if the network reordered messages, they might wait indefinitely for
    a message that will now never come. */
    coro_t::spawn_sometime(boost::bind(
        &directory_read_manager_t::interrupt_updates_and_free_session, this,
        session_to_destroy,
        auto_drainer_t::lock_t(&global_drainer)
        ));

    /* Notify every thread that the peer has disconnected */
    if (got_initialization) {
        fifo_enforcer_write_token_t propagation_fifo_token = propagation_fifo_source.enter_write();
        for (int i = 0; i < get_num_threads(); i++) {
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_disconnect_on_thread, this,
                i, propagation_fifo_token, peer,
                auto_drainer_t::lock_t(&global_drainer)
                ));
        }
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_initialization(peer_id_t peer, boost::uuids::uuid session_id, metadata_t initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING {
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

    /* Dispatch notice of the initialization to every thread */
    fifo_enforcer_write_token_t propagation_fifo_token = propagation_fifo_source.enter_write();
    for (int i = 0; i < get_num_threads(); i++) {
        coro_t::spawn_sometime(boost::bind(
            &directory_read_manager_t::propagate_initialize_on_thread, this,
            i, propagation_fifo_token, peer, initial_value,
            auto_drainer_t::lock_t(&global_drainer)
            ));
    }

    /* Create a metadata FIFO sink and pulse the `got_initial_message` cond so
    that instances of `propagate_update()` can proceed */
    session->metadata_fifo_sink.reset(new fifo_enforcer_sink_t(metadata_fifo_state));
    session->got_initial_message.pulse();
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_update(peer_id_t peer, boost::uuids::uuid session_id, metadata_t new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t per_thread_keepalive) THROWS_NOTHING {
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
        fifo_enforcer_sink_t::exit_write_t fifo_exit(
            session->metadata_fifo_sink.get(),
            metadata_fifo_token
            );
        wait_interruptible(&fifo_exit, session_keepalive.get_drain_signal());

        fifo_enforcer_write_token_t propagation_fifo_token =
            propagation_fifo_source.enter_write();
        for (int i = 0; i < get_num_threads(); i++) {
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_update_on_thread, this,
                i, propagation_fifo_token, peer, new_value,
                auto_drainer_t::lock_t(&global_drainer)
                ));
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
void directory_read_manager_t<metadata_t>::interrupt_updates_and_free_session(session_t *session, UNUSED auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);

    /* This must be done in a separate coroutine because `session_t` has an
    `auto_drainer_t`, and the `auto_drainer_t` destructor can block. */
    delete session;
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_initialize_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &initial_value, UNUSED auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);
    on_thread_t thread_switcher(dest_thread);
    fifo_enforcer_sink_t::exit_write_t fifo_exit(&thread_info.get()->propagation_fifo_sink, propagation_fifo_token);
    fifo_exit.wait_lazily_unordered();

    rwi_lock_assertion_t::write_acq_t acq(&thread_info.get()->peers_list_lock);
    thread_info.get()->peers_list.insert(peer, new thread_peer_info_t(initial_value));
    thread_info.get()->peers_list_publisher.publish(
        boost::bind(&directory_read_manager_t::ping_connection_watcher, peer, _1)
        );

}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_update_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &new_value, UNUSED auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);
    on_thread_t thread_switcher(dest_thread);
    fifo_enforcer_sink_t::exit_write_t fifo_exit(&thread_info.get()->propagation_fifo_sink, propagation_fifo_token);
    fifo_exit.wait_lazily_unordered();

    typename boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it =
        thread_info.get()->peers_list.find(peer);
    rassert(it != thread_info.get()->peers_list.end());
    thread_peer_info_t *pi = (*it).second;
    rwi_lock_assertion_t::write_acq_t acq(&pi->peer_value_lock);
    pi->peer_value = new_value;
    pi->peer_value_publisher.publish(&directory_read_manager_t::ping_value_watcher);
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_disconnect_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, UNUSED auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);
    on_thread_t thread_switcher(dest_thread);
    cond_t non_interruptor;
    fifo_enforcer_sink_t::exit_write_t fifo_exit(&thread_info.get()->propagation_fifo_sink, propagation_fifo_token);
    wait_interruptible(&fifo_exit, &non_interruptor);

    rwi_lock_assertion_t::write_acq_t acq(&thread_info.get()->peers_list_lock);
    /* We need to remove the doomed peer from the peers list before we call the
    callbacks so that it doesn't appear if the callbacks call
    `get_peers_list()`. But we need to call the callbacks before we destroy it
    so that anything subscribed to its `peer_value_publisher` unsubscribes
    itself before the publisher is destroyed. */
    typename boost::ptr_map<peer_id_t, thread_peer_info_t>::auto_type doomed_peer =
        thread_info.get()->peers_list.release(thread_info.get()->peers_list.find(peer));
    thread_info.get()->peers_list_publisher.publish(
        boost::bind(&directory_read_manager_t::ping_disconnection_watcher, peer, _1)
        );
    doomed_peer.reset();
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::ping_connection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING {
    if (connect_cb_and_disconnect_cb.first) {
        connect_cb_and_disconnect_cb.first(peer);
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::ping_value_watcher(const boost::function<void()> &callback) THROWS_NOTHING {
    callback();
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::ping_disconnection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) THROWS_NOTHING {
    if (connect_cb_and_disconnect_cb.second) {
        connect_cb_and_disconnect_cb.second(peer);
    }
}

template<class metadata_t>
rwi_lock_assertion_t *directory_read_manager_t<metadata_t>::get_peers_list_lock() THROWS_NOTHING {
    return &thread_info.get()->peers_list_lock;
}

template<class metadata_t>
publisher_t< std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *directory_read_manager_t<metadata_t>::get_peers_list_publisher() THROWS_NOTHING {
    return thread_info.get()->peers_list_publisher.get_publisher();
}

template<class metadata_t>
rwi_lock_assertion_t *directory_read_manager_t<metadata_t>::get_peer_value_lock(peer_id_t peer) THROWS_NOTHING {
    typename boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it =
        thread_info.get()->peers_list.find(peer);
    rassert(it != thread_info.get()->peers_list.end());
    return &(*it).second->peer_value_lock;
}

template<class metadata_t>
publisher_t<
        boost::function<void()>
        > *directory_read_manager_t<metadata_t>::get_peer_value_publisher(peer_id_t peer, peer_value_freeze_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(this, peer);
    typename boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it =
        thread_info.get()->peers_list.find(peer);
    rassert(it != thread_info.get()->peers_list.end());
    return (*it).second->peer_value_publisher.get_publisher();
}
