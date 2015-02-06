// Copyright 2010-2015 RethinkDB, all rights reserved.

template<class key_t, class value_t>
minidir_read_manager_t<key_t, value_t>::minidir_read_manager_t(
        mailbox_manager_t *mm) :
    mailbox_manager(mm),
    update_mailbox(mailbox_manager,
        std::bind(&minidir_read_manager_t::on_update, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6))
{
}

template<class key_t, class value_t>
minidir_read_manager_t<key_t, value_t>::peer_data_t::peer_data_t(
        minidir_read_manager_t *_parent,
        peer_id_t _peer) :
    parent(_parent), peer(_peer),
    disconnect_watcher(parent->mailbox_manager, peer)
{
    signal_t::subscription_t::reset(&disconnect_watcher);
}

template<class key_t, class value_t>
void minidir_read_manager_t<key_t, value_t>::peer_data_t::run() {
    if (!parent->stopping) {
        coro_t::spawn_sometime(std::bind(
            &minidir_read_manager_t::destroy_peer,
            parent, peer_id, parent->drainer.lock()));
    }
}

template<class key_t, class value_t>
void minidir_read_manager_t<key_t, value_t>::destroy_peer(
        peer_id_t peer,
        auto_drainer_t::lock_t keepalive) {
    ASSERT_NO_CORO_WAITING;
    auto it = peer_map.find(peer);
    guarantee(it != peer_map.end());
    guarantee(it->second.has());
    guarantee(it->second->disconnect_watcher.is_pulsed());
    peer_map.erase(it);
}

template<class key_t, class value_t>
void minidir_read_manager_t<key_t, value_t>::on_update(
        signal_t *interruptor,
        const peer_id_t &peer_id,
        const uuid_u &source_id,
        firo_enforcer_write_token_t fifo_token,
        bool destroy_source,
        const key_t &key,
        const boost::optional<value_t> &value) {
    scoped_ptr_t<peer_data_t> *peer_data_ptr = &peer_map[peer_id];
    if (!peer_data_ptr->has()) {
        /* This will construct a `disconnect_watcher_t` for the peer. The correctness of
        this code relies on the assumption that it's impossible for us to receive a
        message over a connection, then for that connection to disconnect and reconnect,
        then for the mailbox callback to be called for that connection. */
        peer_data_ptr->init(new peer_data_t(this, peer_id));
    }
    peer_data_t *peer_data = peer_data_ptr->get();
    auto_drainer_t::lock_t peer_keepalive = peer_data->drainer.lock();
    source_data_t *source_data = peer_data->source_map[source_id];
    fifo_enforcer_sink_t::exit_write_t exit_write(&source_data->fifo_sink, fifo_token);
    wait_any_t waiter(&exit_write, peer_keepalive.get_drain_signal());
    wait_interruptible(&waiter, interruptor);
    if (peer_keepalive.get_drain_signal()->is_pulsed()) {
        return;
    }
    if (destroy_source) {
        guarantee(key == key_t());
        guarantee(!static_cast<bool>(value));
        exit_write.exit();
        peer_data->source_map.erase(source_id);
    } else {
        if (static_cast<bool>(value)) {
            guarantee(source_data->map.count(key) == 0);
            source_data->map.insert(std::make_pair(key,
                watchable_map_var_t<key_t, value_t>::entry_t(&map, key, value)));
        } else {
            auto it = source_data->map.find(key);
            guarantee(it != source_data->map.end());
            source_data->map.erase(it);
        }
    }
}
