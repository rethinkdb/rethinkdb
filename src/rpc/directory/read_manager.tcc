template<class metadata_t>
directory_read_manager_t<metadata_t>::directory_read_manager_t(message_rservice_t *sub) :
    super_message_service(sub),
    connectivity_subscription(
        std::make_pair(
            boost::bind(&directory_read_manager_t::on_connect, this, _1),
            boost::bind(&directory_read_manager_t::on_disconnect, this, _1)
            ),
        super_message_service->get_connectivity()
        ),
    message_handler_registration(
        super_message_service,
        boost::bind(&directory_read_manager_t::on_message, this, _1, _2)
        )
{
    rassert(super_message_service->get_connectivity()->get_peers_list().size() == 1);
    on_connect(get_me());
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::~directory_read_manager_t() {
    rassert(super_message_service->get_connectivity()->get_peers_list().size() == 1);
    on_disconnect(get_me());
}

template<class metadata_t>
clone_ptr_t<directory_rview_t<metadata_t> > directory_read_manager_t<metadata_t>::get_root_view() {
    return clone_ptr_t<directory_rview_t<metadata_t> >(new root_view_t(this));
}

template<class metadata_t>
peer_id_t directory_read_manager_t<metadata_t>::get_me() {
    return super_message_service()->get_connectivity()->get_me();
}

template<class metadata_t>
std::set<peer_id_t> directory_read_manager_t<metadata_t>::get_peers_list() {
    std::set<peer_id_t> peers_list;
    for (boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it = thread_info.get()->peers_list.begin();
            it != thread_info.get()->peers_list.end(); it++) {
        peers_list.append((*it).first);
    }
    return peers_list;
}

template<class metadata_t>
connectivity_service_t *directory_read_manager_t<metadata_t>::get_connectivity() {
    /* We are our own `connectivity_service_t` */
    return this;
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::root_view_t *directory_read_manager_t<metadata_t>::root_view_t::clone() THROWS_NOTHING {
    return new root_view_t(parent);
}

template<class metadata_t>
boost::optional<metadata_t> directory_read_manager_t<metadata_t>::root_view_t::get_value(peer_id_t peer) THROWS_NOTHING {
    boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it =
        thread_info.get()->peers_list.find(peer);
    if (it == thread_info.get()->peers_list.end()) {
        return boost::optional<metadata_t>();
    } else {
        return boost::optional<metadata_t>((*it).second.peer_value);
    }
}

template<class metadata_t>
directory_rservice_t *directory_read_manager_t<metadata_t>::root_view_t::get_directory() THROWS_NOTHING {
    return parent;
}

template<class metadata_t>
directory_read_manager_t<metadata_t>::root_view_t::root_view_t(directory_read_manager_t *p) THROWS_NOTHING :
    parent(p) { }

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_connect(peer_id_t peer) THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t acq(&mutex);
    rassert(peer_info.count(peer) == 0);
    peer_info.insert(peer, new global_peer_info_t);
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_message(peer_id_t source_peer, std::istream &stream) THROWS_NOTHING {
    assert_thread();
    switch (stream.get()) {
        case 'I': {
            /* Initial message from another peer */
            metadata_t initial_value;
            fifo_enforcer_source_t::state_t state;
            {
                boost::archive::binary_iarchive archive(stream);
                archive >> initial_value;
                archive >> state;
            }

            mutex_assertion_t::acq_t acq(&mutex);

            /* Dispatch notice of the initialization to every thread */
            fifo_enforcer_write_token_t propagation_fifo_token = propagation_fifo_source.enter_write();
            for (int i = 0; i < get_num_threads(); i++) {
                coro_t::spawn_sometime(boost::bind(
                    &directory_read_manager_t::propagate_initialize_on_thread, this,
                    i, propagation_fifo_token, source_peer, initial_value,
                    auto_drainer_t::lock_t(&global_drainer)
                    ));
            }

            /* Create FIFO sink so that updates will work, and pulse the
            initial-message cond so that they know that they can go */
            peer_info[source_peer].metadata_fifo_sink.reset(new fifo_enforcer_sink_t(state));
            peer_info[source_peer].got_initial_message.pulse();

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

            /* Spawn a new coroutine to handle the dispatching. This is because
            if the network reordered this message so it arrived before the
            initialization message, we won't have a FIFO sink to sink
            `metadata_fifo_token` into, so we need to be able to block. If
            `on_message()` blocks waiting for another message from the same
            peer, the result will be a deadlock. */
            coro_t::spawn_sometime(boost::bind(
                &directory_read_manager_t::propagate_update, this,
                source_peer, new_value, metadata_fifo_token,
                auto_drainer_t::lock_t(&peer_info[source_peer].drainer)
                ));

            break;
        }

        default: unreachable();
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::on_disconnect(peer_id_t peer) THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t acq(&mutex);

    /* Remove the `global_peer_info_t` object from the table */
    rassert(peer_info.count(peer) == 1);
    std::auto_ptr<global_peer_info_t> pi = peer_info.release(peer_info.find(peer));

    /* Start interrupting any running calls to `propagate_update()`. We need to
    explicitly interrupt them rather than letting them finish on their own
    because, if the network reordered messages, they might wait indefinitely for
    a message that will now never come. */
    coro_t::spawn_sometime(boost::bind(
        &directory_read_manager_t::interrupt_updates_and_free_global_peer_info, this,
        pi.release(),
        auto_drainer_t::lock_t(&global_drainer)
        ));

    /* Notify every thread that the peer has disconnected */
    fifo_enforcer_write_token_t propagation_fifo_token = propagation_fifo_source.enter_write();
    for (int i = 0; i < get_num_threads(); i++) {
        coro_t::spawn_sometime(boost::bind(
            &directory_read_manager_t::propagate_disconnect_on_thread, this,
            i, propagation_fifo_token, peer,
            auto_drainer_t::lock_t(&global_drainer)
            ));
    }
}

template<class metadata_t>
void propagate_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t peer_keepalive) THROWS_NOTHING {
    assert_thread();

    try {
        /* Wait until we got an initialization message from this peer */
        {
            wait_any_t waiter(&peer_info[peer].got_initial_message, peer_keepalive.get_drain_signal());
            waiter.wait_lazily_unordered();
            if (peer_keepalive.get_drain_signal()->is_pulsed()) {
                throw interrupted_exc_t;
            }
        }

        /* Exit this peer's `metadata_fifo_sink` so that we perform the updates
        in the same order as they were performed at the source. */
        fifo_enforcer_sink_t::exit_write_t fifo_exit(
            peer_info[peer].metadata_fifo_sink.get(),
            metadata_fifo_token,
            peer_keepalive.get_drain_signal()
            );

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
        spawned `interrupt_updates_and_free_global_peer_info()`, which deleted
        the `global_peer_info_t` for this peer, thereby calling the destructor
        for the peer's `auto_drainer_t`, for which `peer_keepalive` is an
        `auto_drainer_t::lock_t`. `peer_keepalive.get_drain_signal()` was
        pulsed, so an `interrupted_exc_t` was thrown.

        The peer has disconnected, so we can safely ignore this update; its
        entry in the thread-local peer table will be deleted soon anyway. */
    }
}

template<class metadata_t>
void interrupt_updates_and_free_global_peer_info(global_peer_info_t *peer_info, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();

    /* This must be done in a separate coroutine because `global_peer_info_t`
    has an `auto_drainer_t`, and the `auto_drainer_t` destructor can block. */
    delete peer_info;
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_initialize_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &initial_value, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);
    on_thread_t thread_switcher(dest_thread);
    fifo_enforcer_sink_t::exit_write_t fifo_exit(&thread_info.get()->propagation_fifo_sink);

    mutex_assertion_t::acq_t acq(&thread_info.get()->peers_list_lock);
    thread_info.get()->peers_list.insert(peer, new thread_peer_info_t(initial_value));
    thread_info.get()->peers_list_publisher.publish(
        boost::bind(&directory_read_manager_t::ping_connection_watcher, peer, _1)
        );
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_update_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, const metadata_t &new_value, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);
    on_thread_t thread_switcher(dest_thread);
    fifo_enforcer_sink_t::exit_write_t fifo_exit(&thread_info.get()->propagation_fifo_sink);

    peer_info_t *pi = &thread_info.get()->peers_list[peer];
    mutex_assertion_t::acq_t acq(&pi->peer_value_lock);
    pi->peer_value = new_value;
    pi->peer_value_publisher.publish(&directory_read_manager_t::ping_value_watcher);
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::propagate_disconnect_on_thread(int dest_thread, fifo_enforcer_write_token_t propagation_fifo_token, peer_id_t peer, auto_drainer_t::lock_t global_keepalive) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&global_drainer);
    on_thread_t thread_switcher(dest_thread);
    fifo_enforcer_sink_t::exit_write_t fifo_exit(&thread_info.get()->propagation_fifo_sink);

    mutex_assertion_t::acq_t acq(&thread_info.get()->peers_list_lock);
    /* We need to remove the doomed peer from the peers list before we call the
    callbacks so that it doesn't appear if the callbacks call
    `get_peers_list()`. But we need to call the callbacks before we destroy it
    so that anything subscribed to its `peer_value_publisher` unsubscribes
    itself before the publisher is destroyed. */
    std::auto_ptr<thread_peer_info_t> doomed_peer =
        thread_info.get()->peers_list.release(thread_info.get()->peers_list.find(peer));
    thread_info.get()->peers_list_publisher.publish(
        boost::bind(&directory_read_manager_t::ping_disconnection_watcher, peer, _1)
        );
    doomed_peer.reset();
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::ping_connection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) {
    if (connect_cb_and_disconnect_cb.first) {
        connect_cb_and_disconnect_cb.first(peer);
    }
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::ping_value_watcher(const boost::function<void()> &callback) {
    callback();
}

template<class metadata_t>
void directory_read_manager_t<metadata_t>::ping_disconnection_watcher(peer_id_t peer, const std::pair<boost::function<void(peer_id_t)>, boost::function<void(peer_id_t)> > &connect_cb_and_disconnect_cb) {
    if (connect_cb_and_disconnect_cb.second) {
        connect_cb_and_disconnect_cb.second(peer);
    }
}

template<class metadata_t>
mutex_assertion_t *directory_read_manager_t<metadata_t>::get_peers_list_lock() {
    return &thread_info.get()->peers_list_lock;
}

template<class metadata_t>
publisher_t<std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *directory_read_manager_t<metadata_t>::get_peers_list_publisher() {
    return thread_info.get()->peers_list_publisher.get_publisher();
}

template<class metadata_t>
mutex_assertion_t *directory_read_manager_t<metadata_t>::get_peer_value_lock(peer_id_t peer) THROWS_NOTHING {
    return &thread_info.get()->peers_list[peer].peer_value_lock;
}

template<class metadata_t>
publisher_t<
        boost::function<void()>
        > *directory_read_manager_t<metadata_t>::get_peer_value_publisher(peer_id_t peer, peer_value_freeze_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(this);
    return thread_info.get()->peers_list[peer].peer_value_publisher.get_publisher();
}
