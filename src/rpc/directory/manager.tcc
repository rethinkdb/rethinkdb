template<class metadata_t>
directory_readwrite_manager_t<metadata_t>::directory_readwrite_manager_t(
        message_service_t *super,
        const metadata_t &initial_metadata) THROWS_NOTHING :
    directory_read_manager_t<metadata_t>(super->get_connectivity_service()),
    directory_write_manager_t<metadata_t>(super, initial_metadata)
    { }

template<class metadata_t>
clone_ptr_t<directory_rwview_t<metadata_t> > directory_readwrite_manager_t<metadata_t>::get_root_view() THROWS_NOTHING {
    return clone_ptr_t<directory_rwview_t<metadata_t> >(new root_view_t(this));
}

template<class metadata_t>
typename directory_readwrite_manager_t<metadata_t>::root_view_t *directory_readwrite_manager_t<metadata_t>::root_view_t::clone() const THROWS_NOTHING {
    return new root_view_t(parent);
}

template<class metadata_t>
directory_readwrite_service_t *directory_readwrite_manager_t<metadata_t>::root_view_t::get_directory_service() THROWS_NOTHING {
    return parent;
}

template<class metadata_t>
boost::optional<metadata_t> directory_readwrite_manager_t<metadata_t>::root_view_t::get_value(peer_id_t peer) THROWS_NOTHING {
    return read_view->get_value(peer);
}

template<class metadata_t>
metadata_t directory_readwrite_manager_t<metadata_t>::root_view_t::get_our_value(directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
    return write_view->get_our_value(proof);
}

template<class metadata_t>
void directory_readwrite_manager_t<metadata_t>::root_view_t::set_our_value(const metadata_t &new_value_for_us, directory_write_service_t::our_value_lock_acq_t *proof) THROWS_NOTHING {
    write_view->set_our_value(new_value_for_us, proof);
}

template<class metadata_t>
directory_readwrite_manager_t<metadata_t>::root_view_t::root_view_t(directory_readwrite_manager_t *p) THROWS_NOTHING :
    parent(p),
    read_view(parent->directory_read_manager_t<metadata_t>::get_root_view()),
    write_view(parent->directory_write_manager_t<metadata_t>::get_root_view())
    { }
