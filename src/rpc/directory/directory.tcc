directory_manager_t::directory_manager_t(metadata_service_t *ms, const metadata_t &initial_metadata) THROWS_NOTHING :
    super_message_service(ms),
    sub_message_service(this),
    root_view(boost::make_shared<root_view_t>(this)),
    thread_info(super_message_service->get_me(), initial_metadata)
{
    super_message_service->set_message_callback(
        boost::bind(&directory_manager_t::on_message, this, _1, _2, _3));
}

boost::shared_ptr<directory_rwview_t<metadata_t> > directory_manager_t::get_root_view() THROWS_NOTHING {
    auto_drainer.assert_not_draining();
    return root_view;
}

void directory_manager_t::propagate_on_thread(int thread, peer_id_t peer, const metadata_t &new_metadata, auto_drainer_t::lock_t) THROWS_NOTHING {
    on_thread_t thread_switcher(thread);
    peer_info_t *peer_info = thread_info.get()->peers[peer];
    mutex_assertion_t::acq_t freeze(&peer_info->peer_value_lock);
    peer_info->value = new_metadata;
    peer_info->peer_value_publisher.publish(&directory_manager_t::call_value_callback);
}

boost::optional<metadata_t> directory_manager_t::root_view_t::get_value(peer_id_t peer) THROWS_NOTHING {
    parent->auto_drainer.assert_not_draining();
    boost::ptr_map<peer_id_t, peer_info_t>::iterator it = parent->thread_info.get()->peers.find(peer);
    if (it == parent->thread_info.get()->peers.end()) {
        peer_info_t *peer_info = (*it).second;
        return boost::optional<metadata_t>(peer_info->value);
    } else {
        return boost::optional<metadata_t>();
    }
}

directory_t *directory_manager_t::root_view_t::get_directory() THROWS_NOTHING {
    return parent;
}

void directory_manager_t::root_view_t::set_our_value(const metadata_t &new_value_for_us, directory_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
    parent->assert_thread()
    proof->assert_is_holding(parent);
    parent->auto_drainer.assert_not_draining();

    std::set<peer_id_t> peers;
    transition_timestamp_t change_timestamp;
    {
        /* Update `our_value` and copy peers list atomically, so that every peer
        gets a single message with the new value */
        connectivity_service_t::peers_list_freeze_t freeze(super_message_service->get_connectivity());
        our_value = new_value_for_us;
        change_timestamp = transition_timestamp_t::starting_from(our_value_timestamp);
        our_value_timestamp = change_timestamp.after();
        peers = super_message_service->get_connectivity()->get_peers_list();
    }

    ... spawn coroutine to propagate to other machines; guarantee ordering somehow; use auto_drainer_t ...
    pmap(get_num_threads(), boost::bind(&directory_manager_t::propagate_on_thread, _1, new_value_for_us, keepalive));

    parent->auto_drainer.assert_not_draining();
}

directory_manager_t::root_view_t::root_view_t(directory_manager_t *p) THROWS_NOTHING :
    parent(p) { }

mutex_assertion_t *directory_manager_t::get_peers_list_lock() {
    return &threads[get_thread_id()]->peers_list_lock;
}

publisher_t<std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *directory_manager_t::get_peer_list_publisher(peers_list_freeze_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(this);
    return threads[get_thread_id()]->peers_list_publisher.get_publisher();
}

mutex_assertion_t *directory_manager_t::get_peer_value_lock(peer_id_t peer) THROWS_NOTHING {
    return &threads[get_thread_id()]->peers[peer]->peer_value_lock;
}

publisher_t<
        boost::function<void()>
        > *directory_manager_t::get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING {
    return threads[get_thread_id()]->peers[peer]->peer_value_publisher.get_publisher();
}

mutex_t *directory_manager_t::get_our_value_lock() THROWS_NOTHING {
    assert_thread();
    return &our_value_lock;
}
