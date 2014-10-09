// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_MAP_WRITE_MANAGER_TCC_
#define RPC_DIRECTORY_MAP_WRITE_MANAGER_TCC_

#include "rpc/directory/map_write_manager.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "containers/archive/boost_types.hpp"

#define MAX_OUTSTANDING_DIRECTORY_WRITES 4

template<class key_t, class value_t>
directory_map_write_manager_t<key_t, value_t>::directory_map_write_manager_t(
        connectivity_cluster_t *_connectivity_cluster,
        connectivity_cluster_t::message_tag_t _message_tag,
        watchable_map_t<key_t, value_t> *_value) :
    connectivity_cluster(_connectivity_cluster),
    message_tag(_message_tag),
    value(_value),
    timestamp(0),
    semaphore(MAX_OUTSTANDING_DIRECTORY_WRITES),
    value_subs(value,
        [this](const key_t &key, const value_t *) {
            ++timestamp;
            auto_drainer_t::lock_t this_keepalive(&drainer);
            for (const auto &pair : last_connections) {
                coro_t::spawn_sometime(boost::bind(
                    &directory_map_write_manager_t::send_update, this,
                    pair.first, pair.second, this_keepalive, key));
            }
        },
        /* `on_connections_change()` will take care of sending initial messages */
        false),
    connections_subs(
        [this]() {
            this->on_connections_change();
        })
{
    typename watchable_t<connectivity_cluster_t::connection_map_t>::freeze_t
        connections_freeze(connectivity_cluster->get_connections());
    connections_subs.reset(connectivity_cluster->get_connections(), &connections_freeze);
    on_connections_change();
}

template<class key_t, class value_t>
void directory_map_write_manager_t<key_t, value_t>::on_connections_change() {
    connectivity_cluster_t::connection_map_t current_connections =
        connectivity_cluster->get_connections()->get();
    for (auto pair : current_connections) {
        connectivity_cluster_t::connection_t *connection = pair.second.first;
        auto_drainer_t::lock_t connection_keepalive = pair.second.second;
        if (last_connections.count(connection) == 0) {
            last_connections.insert(std::make_pair(connection, connection_keepalive));
            auto_drainer_t::lock_t this_keepalive(&drainer);
            value->read_all([&](const key_t &key, const value_t *) {
                coro_t::spawn_sometime(boost::bind(
                    &directory_map_write_manager_t::send_update, this,
                    connection, connection_keepalive, this_keepalive, key));
            });
        }
    }
    for (auto next = last_connections.begin(); next != last_connections.end();) {
        auto pair = next++;
        if (current_connections.count(pair->first->get_peer_id()) == 0) {
            last_connections.erase(pair->first);
        }
    }
}

template<class key_t, class value_t>
class directory_map_write_manager_t<key_t, value_t>::update_writer_t :
    public cluster_send_message_write_callback_t
{
public:
    update_writer_t(
            uint64_t _timestamp, const key_t &_key, boost::optional<value_t> &&_value) :
        timestamp(_timestamp), key(_key), value(std::move(_value)) { }
    void write(cluster_version_t cv, write_stream_t *s) {
        write_message_t wm;
        serialize_for_version(cv, &wm, timestamp);
        serialize_for_version(cv, &wm, key);
        serialize_for_version(cv, &wm, value);
        int res = send_write_message(s, &wm);
        if (res) {
            throw fake_archive_exc_t();
        }
    }
private:
    uint64_t timestamp;
    key_t key;
    boost::optional<value_t> value;
};

template<class key_t, class value_t>
void directory_map_write_manager_t<key_t, value_t>::send_update(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        auto_drainer_t::lock_t this_keepalive,
        const key_t &key) {
    wait_any_t interruptor(
        connection_keepalive.get_drain_signal(),
        this_keepalive.get_drain_signal());
    try {
        new_semaphore_acq_t acq(&semaphore, 1);
        wait_interruptible(acq.acquisition_signal(), &interruptor);
        update_writer_t writer(timestamp, key, value->get_key(key));
        connectivity_cluster->send_message(
            connection, connection_keepalive, message_tag, &writer);
    } catch (interrupted_exc_t) {
        /* Do nothing. The connection was closed, or we're shutting down. */
    }
}

#endif /* RPC_DIRECTORY_MAP_WRITE_MANAGER_TCC_ */

