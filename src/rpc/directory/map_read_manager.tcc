// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_MAP_READ_MANAGER_TCC_
#define RPC_DIRECTORY_MAP_READ_MANAGER_TCC_

#include "rpc/directory/map_read_manager.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "containers/archive/boost_types.hpp"

template<class key_t, class value_t>
directory_map_read_manager_t<key_t, value_t>::directory_map_read_manager_t(
        connectivity_cluster_t *cm,
        connectivity_cluster_t::message_tag_t tag) :
    cluster_message_handler_t(cm, tag)
{
    guarantee(get_connectivity_cluster()->get_connections()->get().empty());
}

template<class key_t, class value_t>
directory_map_read_manager_t<key_t, value_t>::~directory_map_read_manager_t() {
    guarantee(get_connectivity_cluster()->get_connections()->get().empty());
}

template<class key_t, class value_t>
void directory_map_read_manager_t<key_t, value_t>::on_message(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        read_stream_t *s)
        THROWS_ONLY(fake_archive_exc_t) {
    with_priority_t p(CORO_PRIORITY_DIRECTORY_CHANGES);
    uint64_t timestamp;
    archive_result_t res = deserialize<cluster_version_t::CLUSTER>(s, &timestamp);
    if (res != archive_result_t::SUCCESS) {
        throw fake_archive_exc_t();
    }
    key_t key;
    res = deserialize<cluster_version_t::CLUSTER>(s, &key);
    if (res != archive_result_t::SUCCESS) {
        throw fake_archive_exc_t();
    }
    boost::optional<value_t> value;
    res = deserialize<cluster_version_t::CLUSTER>(s, &value);
    if (res != archive_result_t::SUCCESS) {
        throw fake_archive_exc_t();
    }
    auto_drainer_t::lock_t this_keepalive(per_thread_drainers.get());
    coro_t::spawn_sometime(boost::bind(
        &directory_map_read_manager_t::do_update, this,
        connection->get_peer_id(), connection_keepalive, this_keepalive,
        timestamp, key, value));
}

template<class key_t, class value_t>
void directory_map_read_manager_t<key_t, value_t>::do_update(
        peer_id_t peer_id,
        auto_drainer_t::lock_t connection_keepalive,
        auto_drainer_t::lock_t this_keepalive,
        uint64_t timestamp,
        const key_t &key,
        const boost::optional<value_t> &value) {
    /* If we're the first call to `do_update()` for this connection, then we create the
    entry in `timestamps` for this peer, and then the coroutine stays alive and waits for
    the connection to end so it can clean up. If we're not the first call to
    `do_update()` for this connection, we just deliver our update and then return. */
    bool should_cleanup;
    {
        on_thread_t switcher(home_thread());
        auto pair = timestamps.insert(std::make_pair(
            peer_id, std::map<key_t, uint64_t>()));
        should_cleanup = pair.second;
        /* If there's no entry in `timestamps` for this key, or there is an entry but the
        timestamp is earlier, then we should deliver our update. Otherwise, we shouldn't,
        because we don't want to overwrite a later value. */
        auto pair2 = pair.first->second.insert(std::make_pair(key, timestamp));
        bool should_update = false;
        if (pair2.second) {
            should_update = true;
        } else {
            if (pair2.first->second < timestamp) {
                pair2.first->second = timestamp;
                should_update = true;
            }
        }
        if (should_update) {
            if (static_cast<bool>(value)) {
                map_var.set_key_no_equals(std::make_pair(peer_id, key), *value);
            } else {
                map_var.delete_key(std::make_pair(peer_id, key));
            }
        }
    }
    if (should_cleanup) {
        /* Wait until it's time to clean up, either because we're shutting down or the
        connection was lost */
        wait_any_t waiter(
            connection_keepalive.get_drain_signal(),
            this_keepalive.get_drain_signal());
        waiter.wait_lazily_unordered();
        on_thread_t switcher(home_thread());
        /* Remove the entry in `timestamps` and also any entries that we made in
        `map_var`, because the peer is disconnected, so we shouldn't show its entries. */
        timestamps.erase(peer_id);
        std::set<std::pair<peer_id_t, key_t> > to_erase;
        map_var.read_all(
            [&](const std::pair<peer_id_t, key_t> &k, const value_t *) {
                if (k.first == peer_id) {
                    to_erase.insert(k);
                }
            });
        for (const auto &k : to_erase) {
            map_var.delete_key(k);
        }
    }
}

#endif   /* RPC_DIRECTORY_MAP_READ_MANAGER_TCC_ */

