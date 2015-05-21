// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_MAP_WRITE_MANAGER_TCC_
#define RPC_DIRECTORY_MAP_WRITE_MANAGER_TCC_

#include "rpc/directory/map_write_manager.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/wait_any.hpp"
#include "containers/archive/boost_types.hpp"

template<class key_t, class value_t>
directory_map_write_manager_t<key_t, value_t>::directory_map_write_manager_t(
        connectivity_cluster_t *_connectivity_cluster,
        connectivity_cluster_t::message_tag_t _message_tag,
        watchable_map_t<key_t, value_t> *_value) :
    connectivity_cluster(_connectivity_cluster),
    message_tag(_message_tag),
    value(_value),
    timestamp(0),
    value_subs(value,
        [this](const key_t &key, const value_t *) {
            ++this->timestamp;
            for (auto &pair : conns) {
                pair.second.dirty_keys.insert(key);
                if (pair.second.pulse_on_dirty != nullptr) {
                    pair.second.pulse_on_dirty->pulse_if_not_already_pulsed();
                }
            }
        },
        /* `on_connections_change()` will take care of sending initial messages */
        initial_call_t::NO),
    connections_subs(
        connectivity_cluster->get_connections(),
        std::bind(&directory_map_write_manager_t::on_connection_change,
            this, ph::_1, ph::_2),
        initial_call_t::YES)
{
}

template<class key_t, class value_t>
class directory_map_write_manager_t<key_t, value_t>::update_writer_t :
    public cluster_send_message_write_callback_t
{
public:
    update_writer_t(
            uint64_t _timestamp, const key_t &_key, boost::optional<value_t> &&_value) :
        timestamp(_timestamp), key(_key), value(std::move(_value)) { }
    void write(write_stream_t *s) {
        write_message_t wm;
        serialize<cluster_version_t::CLUSTER>(&wm, timestamp);
        serialize<cluster_version_t::CLUSTER>(&wm, key);
        serialize<cluster_version_t::CLUSTER>(&wm, value);
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
void directory_map_write_manager_t<key_t, value_t>::on_connection_change(
        UNUSED const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair) {
    if (pair != nullptr) {
        auto res = conns.insert(std::make_pair(pair->first, conn_info_t()));
        if (res.second) {
            value->read_all([&](const key_t &key, const value_t *) {
                res.first->second.dirty_keys.insert(key);
            });
            coro_t::spawn_sometime(std::bind(
                &directory_map_write_manager_t::stream_to_conn, this,
                pair->first, pair->second, drainer.lock(), res.first));
        }
    }
}

template<class key_t, class value_t>
void directory_map_write_manager_t<key_t, value_t>::stream_to_conn(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        auto_drainer_t::lock_t this_keepalive,
        typename std::map<connectivity_cluster_t::connection_t *, conn_info_t>::iterator
            conns_entry) {
    try {
        wait_any_t interruptor(
            connection_keepalive.get_drain_signal(),
            this_keepalive.get_drain_signal());
        while (true) {
            /* Wait until there is at least one dirty key. */
            if (conns_entry->second.dirty_keys.empty()) {
                cond_t pulse_on_dirty;
                assignment_sentry_t<cond_t *> cond_sentry(
                    &conns_entry->second.pulse_on_dirty,
                    &pulse_on_dirty);
                wait_interruptible(&pulse_on_dirty, &interruptor);
            }

            /* Copy all dirty keys to a local variable, then iterate over that variable.
            The naive approach would be to always send the first dirty key in
            `conns_entry` until there are no dirty keys left; but that has starvation
            issues. */
            std::set<key_t> dirty_keys;
            std::swap(dirty_keys, conns_entry->second.dirty_keys);
            for (const key_t &key : dirty_keys) {
                /* If the key changed again since we copied `dirty_keys`, we'll be
                sending the newest value, because we didn't copy the value at the same
                time as we copied `dirty_keys`. So it's OK to remove the key from
                `dirty_keys` to prevent sending a redundant message. */
                conns_entry->second.dirty_keys.erase(key);
                update_writer_t writer(timestamp, key, value->get_key(key));
                connectivity_cluster->send_message(
                    connection, connection_keepalive, message_tag, &writer);
            }
        }
    } catch (const interrupted_exc_t &) {
        /* OK, we broke out of the loop */
    }
    conns.erase(conns_entry);
}

#endif /* RPC_DIRECTORY_MAP_WRITE_MANAGER_TCC_ */

