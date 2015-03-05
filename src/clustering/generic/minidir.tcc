// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/generic/minidir.hpp"

#include "concurrency/wait_any.hpp"

template<class key_t, class value_t>
minidir_read_manager_t<key_t, value_t>::minidir_read_manager_t(
        mailbox_manager_t *mm) :
    mailbox_manager(mm),
    update_mailbox(mailbox_manager,
        std::bind(&minidir_read_manager_t::on_update, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7)),
    connection_subs(mailbox_manager->get_connectivity_cluster()->get_connections(),
        std::bind(&minidir_read_manager_t::on_connection_change, this, ph::_1, ph::_2),
        false)
    { }

template<class key_t, class value_t>
void minidir_read_manager_t<key_t, value_t>::on_connection_change(
        const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair) {
    auto it = peer_map.find(peer_id);
    if (pair == nullptr && it != peer_map.end()) {
        peer_data_t *peer_data = it->second.release();
        peer_map.erase(it);
        auto_drainer_t::lock_t this_keepalive = drainer.lock();
        coro_t::spawn_sometime([peer_data, this_keepalive /* important to capture */]() {
            delete peer_data;
        });
    }
}

template<class key_t, class value_t>
void minidir_read_manager_t<key_t, value_t>::on_update(
        signal_t *interruptor,
        const peer_id_t &peer_id,
        const minidir_link_id_t &link_id,
        fifo_enforcer_write_token_t fifo_token,
        bool closing_link,
        const boost::optional<key_t> &key,
        const boost::optional<value_t> &value) {
    scoped_ptr_t<peer_data_t> *peer_data_ptr = &peer_map[peer_id];
    if (!peer_data_ptr->has()) {
        /* Acquire a lock on the connection session over which this message arrived. */
        auto_drainer_t::lock_t conn_keepalive;
        connectivity_cluster_t::connection_t *conn =
            mailbox_manager->get_connectivity_cluster()->get_connection(
                peer_id, &conn_keepalive);
        if (conn == nullptr) {
            /* The connection over which this message arrived was just closed. Ignore the
            message. */
            return;
        }
        peer_data_ptr->init(new peer_data_t(conn_keepalive));
    }
    peer_data_t *peer_data = peer_data_ptr->get();

    auto_drainer_t::lock_t peer_keepalive = peer_data->drainer.lock();
    scoped_ptr_t<typename peer_data_t::link_data_t> *link_data_ptr =
        &peer_data->link_map[link_id];
    if (!link_data_ptr->has()) {
        link_data_ptr->init(new typename peer_data_t::link_data_t);
    }
    typename peer_data_t::link_data_t *link_data = link_data_ptr->get();

    fifo_enforcer_sink_t::exit_write_t exit_write(&link_data->fifo_sink, fifo_token);
    wait_any_t waiter(&exit_write, peer_keepalive.get_drain_signal());
    wait_interruptible(&waiter, interruptor);
    if (peer_keepalive.get_drain_signal()->is_pulsed()) {
        return;
    }
    if (closing_link) {
        guarantee(!static_cast<bool>(key));
        guarantee(!static_cast<bool>(value));
        exit_write.end();
        peer_data->link_map.erase(link_id);
    } else {
        guarantee(static_cast<bool>(key));
        auto it = link_data->map.find(*key);
        if (static_cast<bool>(value)) {
            if (it != link_data->map.end()) {
                /* We are updating an existing key */
                it->second.change([&](value_t *v) {
                    *v = *value;
                    return true;
                });
            } else {
                /* We are creating a new key */
                link_data->map.insert(std::make_pair(*key,
                    typename watchable_map_var_t<key_t, value_t>::entry_t(
                        &map_var, *key, *value)));
            }
        } else {
            /* We are deleting an existing key. (Or maybe the key doesn't exist, and this
            message was redundant. That's possible too.) */
            if (it != link_data->map.end()) {
                link_data->map.erase(it);
            }
        }
    }
}

template<class key_t, class value_t>
minidir_write_manager_t<key_t, value_t>::minidir_write_manager_t(
        mailbox_manager_t *_mm,
        watchable_map_t<key_t, value_t> *_values,
        watchable_map_t<reader_id_t, minidir_bcard_t<key_t, value_t> > *_readers) :
    mailbox_manager(_mm),
    values(_values),
    readers(_readers),
    stopping(false),
    /* The `reader_subs` constructor takes care of sending all the initial messages
    because we pass `true` as the third parameter. We pass `false` to the other two
    constructors to avoid redundancy. */
    connection_subs(mailbox_manager->get_connectivity_cluster()->get_connections(),
        std::bind(&minidir_write_manager_t::on_connection_change, this, ph::_1, ph::_2),
        false),
    value_subs(values,
        std::bind(&minidir_write_manager_t::on_value_change, this, ph::_1, ph::_2),
        false),
    reader_subs(readers,
        std::bind(&minidir_write_manager_t::on_reader_change, this, ph::_1, ph::_2),
        true)
    { }

template<class key_t, class value_t>
minidir_write_manager_t<key_t, value_t>::~minidir_write_manager_t() {
    ASSERT_FINITE_CORO_WAITING;
    /* Send a closing message to every existing link */
    for (auto &&peer_pair : peer_map) {
        for (auto &&link_pair : peer_pair.second->link_map) {
            spawn_closing_link(link_pair.second.get());
        }
    }
    /* Delete all the links. */
    peer_map.clear();
    /* Prevent `on_reader_change()` from creating any new links while we wait for the
    coroutines we just started with `spawn_closing_link()` to finish. */
    stopping = true;
}

template<class key_t, class value_t>
void minidir_write_manager_t<key_t, value_t>::on_connection_change(
        const peer_id_t &peer,
        const connectivity_cluster_t::connection_pair_t *pair) {
    auto it = peer_map.find(peer);
    if (pair == nullptr && it != peer_map.end()) {
        peer_map.erase(it);
    } else if (pair != nullptr && it == peer_map.end()) {
        readers->read_all(std::bind(
            &minidir_write_manager_t::on_reader_change, this, ph::_1, ph::_2));
    }
}

template<class key_t, class value_t>
void minidir_write_manager_t<key_t, value_t>::on_value_change(
        const key_t &key,
        const value_t *value) {
    boost::optional<value_t> value_optional;
    if (value != nullptr) {
        value_optional = *value;
    }
    for (auto &&peer_pair : peer_map) {
        for (auto &&link_pair : peer_pair.second->link_map) {
            spawn_update(link_pair.second.get(), key, value_optional);
        }
    }
}

template<class key_t, class value_t>
void minidir_write_manager_t<key_t, value_t>::on_reader_change(
        const reader_id_t &reader_id,
        const minidir_bcard_t<key_t, value_t> *bcard) {
    if (stopping) {
        /* We're in the process of closing all our existing links. So it would be bad to
        open new links here (they would never get closed) and it there's no point in
        closing links. */
        return;
    }
    if (bcard != nullptr) {
        /* Find or make an entry in the peers map */
        peer_id_t peer_id = bcard->update_mailbox.get_peer();
        auto it = peer_map.find(peer_id);
        if (it == peer_map.end()) {
            auto_drainer_t::lock_t connection_keepalive;
            if (mailbox_manager->get_connectivity_cluster()->get_connection(peer_id,
                    &connection_keepalive) == nullptr) {
                /* We aren't connected to this server. Don't make an entry in the peers
                map. If we connect, then `on_connection_change()` will fix it. */
                return;
            }
            auto res = peer_map.insert(std::make_pair(
                peer_id, scoped_ptr_t<peer_data_t>(
                    new peer_data_t(connection_keepalive))));
            it = res.first;
        }

        auto jt = it->second->link_map.find(reader_id);
        if (jt != it->second->link_map.end()) {
            /* We already have an entry for this reader. No need to do anything. */
            return;
        }

        /* Create an entry in the link map for this reader */
        typename peer_data_t::link_data_t *link_data =
            new typename peer_data_t::link_data_t;
        it->second->link_map[reader_id].init(link_data);
        link_data->link_id = generate_uuid();
        link_data->bcard = *bcard;

        /* Send initial values to the new reader */
        values->read_all([&](const key_t &key, const value_t *value) {
            spawn_update(link_data, key, *value);
        });
    } else {
        /* Remove the entry in the link map. Unfortunately, we don't know which peer it's
        on, so we have to check them all. */
        for (auto it = peer_map.begin(); it != peer_map.end(); ++it) {
            auto jt = it->second->link_map.find(reader_id);
            if (jt != it->second->link_map.end()) {
                spawn_closing_link(jt->second.get());
                it->second->link_map.erase(jt);
                if (it->second->link_map.empty()) {
                    /* Garbage collect the now-empty `peer_map` entry */
                    peer_map.erase(it);
                }
                break;
            }
        }
    }
}

template<class key_t, class value_t>
void minidir_write_manager_t<key_t, value_t>::spawn_update(
        typename peer_data_t::link_data_t *link_data,
        const key_t &key,
        const boost::optional<value_t> &value) {
    minidir_link_id_t link_id = link_data->link_id;
    minidir_bcard_t<key_t, value_t> bcard = link_data->bcard;
    fifo_enforcer_write_token_t write_token = link_data->fifo_source.enter_write();
    auto_drainer_t::lock_t keepalive = drainer.lock();
    coro_t::spawn_sometime([this, keepalive /* important to capture */, bcard, link_id,
            write_token, key, value] {
        send(mailbox_manager, bcard.update_mailbox, mailbox_manager->get_me(),
            link_id, write_token, false, boost::make_optional(key), value);
    });
}

template<class key_t, class value_t>
void minidir_write_manager_t<key_t, value_t>::spawn_closing_link(
        typename peer_data_t::link_data_t *link_data) {
    minidir_link_id_t link_id = link_data->link_id;
    minidir_bcard_t<key_t, value_t> bcard = link_data->bcard;
    fifo_enforcer_write_token_t write_token = link_data->fifo_source.enter_write();
    auto_drainer_t::lock_t keepalive = drainer.lock();
    coro_t::spawn_sometime([this, keepalive /* important to capture */, bcard, link_id,
            write_token] {
        send(mailbox_manager, bcard.update_mailbox, mailbox_manager->get_me(),
            link_id, write_token, true, boost::optional<key_t>(),
            boost::optional<value_t>());
    });
}

