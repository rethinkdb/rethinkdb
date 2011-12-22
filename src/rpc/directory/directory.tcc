template<class metadata_t>
directory_manager_t<metadata_t>::directory_manager_t(metadata_service_t *ms, const metadata_t &initial_metadata) THROWS_NOTHING :
    super_message_service(ms),
    sub_message_service(this),
    root_view(boost::make_shared<root_view_t>(this)),
    our_value(initial_metadata),
    connectivity_subscription(std::make_pair(
        boost::bind(&directory_manager_t::on_connect, this, _1),
        boost::bind(&directory_manager_t::on_disconnect, this, _1)
        )),
    message_handler_registration(
        super_message_service,
        boost::bind(&directory_manager_t::on_message, this, _1, _2)
        )
{
    on_connect(get_me(), auto_drainer_t::lock_t(&drainer));
}

template<class metadata_t>
directory_manager_t<metadata_t>::~directory_manager_t() THROWS_NOTHING {
    on_disconnect(get_me(), auto_drainer_t::lock_t(&drainer));
}

template<class metadata_t>
boost::shared_ptr<directory_rwview_t<metadata_t> > directory_manager_t<metadata_t>::get_root_view() THROWS_NOTHING {
    auto_drainer.assert_not_draining();
    return root_view;
}

template<class metadata_t>
peer_id_t directory_manager_t<metadata_t>::get_me() {
    return super_message_service->get_connectivity()->get_me();
}

template<class metadata_t>
std::set<peer_id_t> directory_manager_t<metadata_t>::get_peers_list() {
    std::set<peer_id_t> peers_list;
    for (boost::ptr_map<peer_id_t, thread_peer_info_t>::iterator it = thread_info.get().peers.begin();
            it != thread_info.get().peers.end(); it++) {
        peers_list.append((*it).first);
    }
    return peers_list;
}

template<class metadata_t>
connectivity_service_t *directory_manager_t<metadata_t>::get_connectivity() {
    /* We are the `connectivity_service_t` for our own `directory_service_t`. */
    return this;
}

template<class metadata_t>
message_service_t *directory_manager_t<metadata_t>::get_sub_message_service() {
    return &sub_message_service;
}

template<class metadata_t>
void directory_manager_t<metadata_t>::write_initial_message(std::ostream &stream, const metadata_t &initial_value, fifo_enforcer_source_t::state_t state) THROWS_NOTHING {
    stream.put('I');
    boost::archive::binary_oarchive archive(stream);
    archive << initial_value;
    archive << state;
}

template<class metadata_t>
void directory_manager_t<metadata_t>::write_update_message(std::ostream &stream, const metadata_t &new_value, fifo_enforcer_write_token_t fifo_token) THROWS_NOTHING {
    stream.put('U');
    boost::archive::binary_oarchive archive(stream);
    archive << new_value;
    archive << fifo_token;
}

template<class metadata_t>
void directory_manager_t<metadata_t>::write_sub_message(std::ostream &stream, const boost::function<void(std::ostream &)> &sub_function) THROWS_NOTHING {
    stream.put('S');
    sub_function(stream);
}

template<class metadata_t>
void directory_manager_t<metadata_t>::on_connect(peer_id_t peer) {
    assert_thread();
    mutex_assertion_t::acq_t acq(&connectivity_mutex_assertion);

    /* We could start receiving update messages now, so create a drainer for the
    peer. */
    rassert(peer_drainers.count(peer) == 0);
    peer_drainers.insert(peer, new auto_drainer_t);

    /* Send an initial copy of our metadata to the peer. */
    mutex_assertion_t::acq_t acq(&our_value_lock_assertion);
    coro_t::spawn_sometime(boost::bind(
        &directory_manager_t::greet_peer,
        auto_drainer_t::lock_t(&drainer),
        our_value,
        change_fifo_source.get_state()
        ));
}

template<class metadata_t>
void directory_manager_t<metadata_t>::on_message(peer_id_t source_peer, std::istream &stream) THROWS_NOTHING {
    switch (stream.get()) {
        case 'I': {
            metadata_t initial_value;
            fifo_enforcer_source_t::state_t state;
            {
                boost::archive::binary_iarchive archive(stream);
                archive >> initial_value;
                archive >> state;
            }
            mutex_assertion_t::acq_t acq(&connectivity_mutex_assertion);
            fifo_enforcer_write_token_t connectivity_fifo_token =
                connectivity_fifo_source.enter_write();
            for (int i = 0; i < get_num_threads(); i++) {
                coro_t::spawn_sometime(boost::bind(
                    &directory_manager_t::propagate_initial, this,
                    i, connectivity_fifo_token, global_keepalive,
                    source_peer, intiial_value, state
                    ));
            }
            break;
        }
        case 'U': {
            metadata_t new_value;
            fifo_enforcer_write_token_t fifo_token;
            {
                boost::archive::binary_iarchive archive(stream);
                archive >> new_value;
                archive >> fifo_token;
            }
            mutex_assertion_t::acq_t acq(&connectivity_mutex_assertion);
            fifo_enforcer_read_token_t connectivity_fifo_token =
                connectivity_fifo_source.enter_read();
            for (int i = 0; i < get_num_threads(); i++) {
                coro_t::spawn_sometime(boost::bind(
                    &directory_manager_t::propagate_update, this,
                    i, connectivity_fifo_token, auto_drainer_t::lock_t(&drainer),
                    source_peer, new_value, fifo_token,
                    auto_drainer_t::lock_t(&peer_drainers[source_peer])
                    ));
            break;
        }
        case 'S': {
            rassert(!sub_message_service.callback.empty());
            sub_message_service.callback(source_peer, stream);
            break;
        }
        default: {
            crash("Unexpected message character");
        }
    }
}

template<class metadata_t>
void directory_manager_t<metadata_t>::on_disconnect(peer_id_t peer, auto_drainer_t::lock_t keepalive) {
    assert_thread();
    mutex_assertion_t::acq_t acq(&connectivity_mutex_assertion);

    /* Start interrupting active instances of `propagate_update()` */
    rassert(peer_drainers.count(peer) == 1);
    std::auto_ptr<auto_drainer_t> doomed_peer_drainer = peer_drainers.release(peer_drainers.find(peer));
    coro_t::spawn_sometime(boost::bind(
        &directory_manager_t::drain_propagate_update, this,
        auto_drainer_t::lock_t(&drainer),
        doomed_peer_drainer.release()
        ));

    /* Start notifying everyone that the peer is gone */
    fifo_enforcer_write_token_t connectivity_fifo_token =
        connectivity_fifo_source.enter_write();
    for (int i = 0; i < get_num_threads(); i++) {
        coro_t::spawn_sometime(boost::bind(
            &directory_manager_t::propagate_disconnect, this,
            i, connectivity_fifo_token, auto_drainer_t::lock_t(&drainer),
            peer
            ));
    }
}

template<class metadata_t>
void greet_peer(auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t state) THROWS_NOTHING {
    assert_thread();
    global_keepalive.assert_is_holding(&drainer);
    super_message_service->send_message(peer,
        boost::bind(&directory_manager_t::write_initial_message, _1, initial_value, state)
        );
}

template<class metadata_t>
void directory_manager_t<metadata_t>::propagate_initial_on_thread(int dest_thread, fifo_enforcer_write_token_t connectivity_fifo_token, UNUSED auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t fifo_state) THROWS_NOTHING {
    /* All notifications start from the main thread */
    assert_thread();

    on_thread_t thread_switcher(dest_thread);
    thread_info_t *ti = thread_info.get();

    /* Acquire the `connectivity_fifo_sink` so that we are sure that this
    notification is arriving in the correct order w.r.t. other notifications.
    We don't ever interrupt the FIFO acquisition because nothing should hold
    the `connectivity_fifo_sink` for very long once the shutdown process starts.
    */
    cond_t non_interruptor;
    fifo_enforcer_sink_t::exit_write_t connectivity_fifo_exit(&ti->connectivity_fifo_sink, connectivity_fifo_token, &non_interruptor);

    /* Make an entry for the new peer and notify listeners */
    mutex_assertion_t::acq_t freeze(&ti->peers_list_lock);
    ti->peers.insert(peer, new thread_peer_info_t(initial_value, fifo_state));
    ti->peers_list_publisher.publish(boost::bind(&directory_manager_t<metadata_t>::ping_connection_watcher, peer, _1));
}

template<class metadata_t>
void propagate_update_everywhere(auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t change_fifo_token, auto_drainer_t::lock_t peer_keepalive) THROWS_NOTHING {
    
}


template<class metadata_t>
void directory_manager_t<metadata_t>::propagate_update_on_thread(int dest_thread, fifo_enforcer_read_token_t connectivity_fifo_token, UNUSED auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t fifo_token, auto_drainer_t::lock_t peer_keepalive) THROWS_NOTHING {
    /* All notifications start from the main thread */
    assert_thread();

    cross_thread_signal_t interruptor2(dest_thread, interruptor);
    on_thread_t thread_switcher(dest_thread);
    thread_info_t *ti = thread_info.get();

    cond_t non_interruptor;
    fifo_enforcer_sink_t::exit_read_t connectivity_fifo_exit(&ti->connectivity_fifo_sink, connectivity_fifo_token, &non_interruptor);

    peer_info_t *peer_info = &ti->peers[peer];

    try {
        /* Make sure we're applying updates in the correct order */
        fifo_enforcer_sink_t::exit_write_t fifo_exit(&peer_info->change_fifo_sink, change_fifo_token, &interruptor2);

        /* Apply the update and notify listeners */
        mutex_assertion_t::acq_t freeze(&peer_info->peer_value_lock);
        peer_info->value = new_value;
        peer_info->peer_value_publisher.publish(&directory_manager_t<metadata_t>::ping_value_watcher);

    } catch (interrupted_exc_t) {
        /* Ignore; this means that we lost the connection to the peer, so
        there's no need to deliver the change notification. It's absolutely
        necessary that we allow interruption here because if we got updates out
        of order from the peer and then lost the connection, then the update
        we're blocking on might never arrive. */
    }
}

template<class metadata_t>
void directory_manager_t<metadata_t>::drain_propagate_update(UNUSED auto_drainer_t::lock_t global_keepalive, auto_drainer_t::lock_t *destroy_me) THROWS_NOTHING {
    assert_thread();

    /* Destroying this will interrupt any active instances of
    `propagate_update()`, and block until they have all returned. */
    delete destroy_me;
}

template<class metadata_t>
void directory_manager_t<metadata_t>::propagate_disconnect(int dest_thread, fifo_enforcer_write_token_t connectivity_fifo_token, auto_drainer_t::lock_t global_keepalive, peer_id_t peer) THROWS_NOTHING {
    /* All notifications start from the main thread */
    assert_thread();

    on_thread_t thread_switcher(dest_thread);
    thread_info_t *ti = thread_info.get();

    cond_t non_interruptor;
    fifo_enforcer_sink_t::exit_write_t connectivity_fifo_exit(&ti->connectivity_fifo_sink, connectivity_fifo_token, &non_interruptor);

    /* Remove the entry for the peer and notify listeners */
    mutex_assertion_t::acq_t freeze(&ti->peers_list_lock);
    ti->peers.erase(peer);
    ti->peers_list_publisher.publish(boost::bind(&directory_manager_t<metadata_t>::ping_disconnection_watcher, peer, _1));
}

template<class metadata_t>
boost::optional<metadata_t> directory_manager_t<metadata_t>::root_view_t::get_value(peer_id_t peer) THROWS_NOTHING {
    boost::ptr_map<peer_id_t, peer_info_t>::iterator it = parent->thread_info.get()->peers.find(peer);
    if (it == parent->thread_info.get()->peers.end()) {
        peer_info_t *peer_info = &(*it).second;
        return boost::optional<metadata_t>(peer_info->value);
    } else {
        return boost::optional<metadata_t>();
    }
}

template<class metadata_t>
directory_t *directory_manager_t<metadata_t>::root_view_t::get_directory() THROWS_NOTHING {
    return parent;
}

template<class metadata_t>
void directory_manager_t<metadata_t>::root_view_t::set_our_value(const metadata_t &new_value_for_us, directory_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
    parent->assert_thread()
    proof->assert_is_holding(parent);
    drainer.assert_not_draining();

    std::set<peer_id_t> peers;
    fifo_enforcer_write_token_t fifo_token;
    {
        /* Update `our_value` and copy peers list atomically, so that every peer
        gets a single message with the new value. */
        connectivity_service_t::peers_list_freeze_t freeze(super_message_service->get_connectivity());
        our_value = new_value_for_us;
        fifo_token = fifo_source.enter_write();
        peers = super_message_service->get_connectivity()->get_peers_list();
    }

    /* Propagate to all machines. `propagate_our_value_to_machine` special-cases
    the case of propagating to ourself. */
    pmap(peers.begin(), peers.end(), boost::bind(
        &directory_manager_t::propagate_our_value_to_machine, this,
        _1, new_value_for_us, fifo_token,
        keepalive
        ));

    /* We shouldn't start shutting down until after `set_our_value()` has
    returned; it's the responsibility of the caller to enforce this. */
    drainer.assert_not_draining();
}

template<class metadata_t>
directory_manager_t<metadata_t>::root_view_t::root_view_t(directory_manager_t *p) THROWS_NOTHING :
    parent(p) { }

template<class metadata_t>
mutex_assertion_t *directory_manager_t<metadata_t>::get_peers_list_lock() {
    return &thread_info.get()->peers_list_lock;
}

template<class metadata_t>
publisher_t<std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *directory_manager_t<metadata_t>::get_peer_list_publisher(peers_list_freeze_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(this);
    return thread_info.get()->peers_list_publisher.get_publisher();
}

template<class metadata_t>
mutex_assertion_t *directory_manager_t<metadata_t>::get_peer_value_lock(peer_id_t peer) THROWS_NOTHING {
    return &thread_info.get()->peers[peer].peer_value_lock;
}

template<class metadata_t>
publisher_t<
        boost::function<void()>
        > *directory_manager_t<metadata_t>::get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING {
    return thread_info.get()->peers[peer].peer_value_publisher.get_publisher();
}
