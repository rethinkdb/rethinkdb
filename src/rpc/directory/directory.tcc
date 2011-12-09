directory_cluster_t::directory_cluster_t(const metadata_t &initial_metadata) THROWS_NOTHING :
    root_view(boost::make_shared<root_view_t>(this)),
    threads(new boost::scoped_ptr<thread_info_t>[get_num_threads]),
    auto_drainer(new auto_drainer_t);
{
    pmap(get_num_threads(), boost::bind(&directory_cluster_t::construct_on_thread, _1, initial_metadata));
}

directory_cluster_t::~directory_cluster_t() THROWS_NOTHING {
    auto_drainer.reset();
    pmap(get_num_threads(), boost::bind(&directory_cluster_t::destruct_on_thread, _1));
}

boost::shared_ptr<directory_rwview_t<metadata_t> > directory_cluster_t::get_root_view() THROWS_NOTHING {
    return root_view;
}

void directory_cluster_t::construct_on_thread(int thread, const metadata_t &initial_metadata) THROWS_NOTHING {
    on_thread_t thread_switcher(i);
    thread_info_t *thread_info = new thread_info_t;
    threads[i].reset(thread_info);
    peer_info_t *peer_info = new peer_info_t;
    thread_info->peers.insert(get_me(), peer_info);
    peer_info->value = initial_metadata;
}

void directory_cluster_t::destruct_on_thread(int thread) THROWS_NOTHING {
    on_thread_t thread_switcher(i);
    threads[i].reset();
}

void directory_cluster_t::propagate_on_thread(int thread, peer_id_t peer, const metadata_t &new_metadata) THROWS_NOTHING {
    on_thread_t thread_switcher(i);
    peer_info_t *peer_info = threads[i]->peers[peer];
    mutex_assertion_t::acq_t freeze(&peer_info->peer_value_lock);
    peer_info->value = new_metadata;
    peer_info->peer_value_publisher.publish(&directory_cluster_t::call_value_callback);
}

boost::optional<metadata_t> directory_cluster_t::root_view_t::get_value(peer_id_t peer) THROWS_NOTHING {
    thread_info_t *thread_info = parent->threads[get_thread_id()].get();
    boost::ptr_map<peer_id_t, peer_info_t>::iterator it = thread_info->peers.find(peer);
    if (it == thread_info->peers.end()) {
        peer_info_t *peer_info = (*it).second;
        return boost::optional<metadata_t>(peer_info->value);
    } else {
        return boost::optional<metadata_t>();
    }
}

directory_t *directory_cluster_t::root_view_t::get_directory() THROWS_NOTHING {
    return parent;
}

peer_id_t directory_cluster_t::root_view_t::get_me() THROWS_NOTHING {
    return parent->get_me();
}

void directory_cluster_t::root_view_t::set_our_value(const metadata_t &new_value_for_us, directory_t::peer_value_lock_acq_t *proof) THROWS_NOTHING {
    parent->assert_thread()
    proof->assert_is_holding(parent);
    ... spawn coroutine to propagate to other machines; guarantee ordering somehow; use auto_drainer_t ...
    pmap(get_num_threads(), boost::bind(&directory_cluster_t::propagate_on_thread, _1, new_value_for_us));
}

directory_cluster_t::root_view_t::root_view_t(directory_cluster_t *p) THROWS_NOTHING :
    parent(p) { }

mutex_assertion_t *directory_cluster_t::get_peers_list_lock() {
    return &threads[get_thread_id()]->peers_list_lock;
}

publisher_t<std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *directory_cluster_t::get_peer_list_publisher(peers_list_freeze_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(this);
    return threads[get_thread_id()]->peers_list_publisher.get_publisher();
}

mutex_assertion_t *directory_cluster_t::get_peer_value_lock(peer_id_t peer) THROWS_NOTHING {
    return &threads[get_thread_id()]->peers[peer]->peer_value_lock;
}

publisher_t<
        boost::function<void()>
        > *directory_cluster_t::get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING {
    return threads[get_thread_id()]->peers[peer]->peer_value_publisher.get_publisher();
}

mutex_t *directory_cluster_t::get_our_value_lock() THROWS_NOTHING {
    assert_thread();
    return &our_value_lock;
}
