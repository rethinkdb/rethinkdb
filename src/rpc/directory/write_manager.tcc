template<class metadata_t>
directory_write_manager_t<metadata_t>::directory_write_manager_t(
        message_service_t *sub,
        const metadata_t &initial_metadata) THROWS_NOTHING :
    message_service(sub),
    our_value(initial_metadata),
    connectivity_subscription(
        boost::bind(&directory_write_manager_t::on_connect, this, _1),
        NULL)
{
    connectivity_service_t::peers_list_freeze_t freeze(message_service->get_connectivity_service());
    rassert(message_service->get_connectivity_service()->get_peers_list().empty());
    connectivity_subscription.reset(message_service->get_connectivity_service(), &freeze);
}

template<class metadata_t>
clone_ptr_t<directory_wview_t<metadata_t> > directory_write_manager_t<metadata_t>::get_root_view() THROWS_NOTHING {
    return clone_ptr_t<directory_wview_t<metadata_t> >(new root_view_t(this));
}

template<class metadata_t>
typename directory_write_manager_t<metadata_t>::root_view_t *directory_write_manager_t<metadata_t>::root_view_t::clone() {
    return new root_view_t(parent);
}

template<class metadata_t>
metadata_t directory_write_manager_t<metadata_t>::root_view_t::get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(parent);
    return parent->our_value;
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::root_view_t::set_our_value(const metadata_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(parent);
    /* Acquire this lock to avoid the case where a new peer copies the metadata
    value after we change it but also gets an update sent. (That would lead to a
    crash on the receiving end because the receiving FIFO would get a duplicate
    update.) */
    connectivity_service_t::peers_list_freeze_t freeze(parent->message_service->get_connectivity_service());
    parent->our_value = new_value_for_us;
    fifo_enforcer_write_token_t metadata_fifo_token = parent->metadata_fifo_source.enter_write();
    std::set<peer_id_t> peers = parent->message_service->get_connectivity_service()->get_peers_list();
    for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
        coro_t::spawn_sometime(boost::bind(
            &directory_write_manager_t::send_update, parent,
            *it,
            new_value_for_us, metadata_fifo_token,
            auto_drainer_t::lock_t(&parent->drainer)
            ));
    }
}

template<class metadata_t>
directory_write_service_t *directory_write_manager_t<metadata_t>::root_view_t::get_directory_service() THROWS_NOTHING {
    return parent;
}

template<class metadata_t>
directory_write_manager_t<metadata_t>::root_view_t::root_view_t(directory_write_manager_t *p) THROWS_NOTHING :
    parent(p) { }

template<class metadata_t>
void directory_write_manager_t<metadata_t>::on_connect(peer_id_t peer) THROWS_NOTHING {
    coro_t::spawn_sometime(boost::bind(
        &directory_write_manager_t::send_initialization, this,
        peer,
        our_value, metadata_fifo_source.get_state(),
        auto_drainer_t::lock_t(&drainer)
        ));
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::send_initialization(peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state, auto_drainer_t::lock_t) THROWS_NOTHING {
    message_service->send_message(peer,
        boost::bind(&directory_write_manager_t::write_initialization, _1, initial_value, metadata_fifo_state)
        );
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::send_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t) THROWS_NOTHING {
    message_service->send_message(peer,
        boost::bind(&directory_write_manager_t::write_update, _1, new_value, metadata_fifo_token)
        );
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::write_initialization(std::ostream &os, const metadata_t &initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state) THROWS_NOTHING {
    os.put('I');
    boost::archive::binary_oarchive archive(os);
    archive << initial_value;
    archive << metadata_fifo_state;
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::write_update(std::ostream &os, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token) THROWS_NOTHING {
    os.put('U');
    boost::archive::binary_oarchive archive(os);
    archive << new_value;
    archive << metadata_fifo_token;
}

template<class metadata_t>
mutex_t *directory_write_manager_t<metadata_t>::get_our_value_lock() THROWS_NOTHING {
    return &our_value_lock;
}

