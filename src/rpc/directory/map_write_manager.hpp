// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_WRITE_MAP_MANAGER_HPP_
#define RPC_DIRECTORY_WRITE_MAP_MANAGER_HPP_

#include "concurrency/auto_drainer.hpp"
#include "concurrency/new_semaphore.hpp"
#include "concurrency/watchable_map.hpp"
#include "rpc/connectivity/cluster.hpp"

template<class key_t, class value_t>
class directory_map_write_manager_t {
public:
    directory_map_write_manager_t(
        connectivity_cluster_t *connectivity_cluster,
        connectivity_cluster_t::message_tag_t message_tag,
        watchable_map_t<key_t, value_t> *value);

private:
    /* For each connection, we have an instance of `conn_info_t` in `conns` and a
    corresponding `stream_to_conn()` coroutine. `on_connections_change()` is responsible
    for creating the `conn_info_t` and spawning the coroutine; the coroutine is
    responsible for stopping itself and removing the `conn_info_t`. The coroutine's job
    is to check for keys marked as dirty in `dirty_keys` and send those key-value pairs
    over the network. */

    class update_writer_t;

    class conn_info_t {
    public:
        conn_info_t() : pulse_on_dirty(nullptr) { }
        /* Whenever a key-value pair changes, the key will be inserted into `dirty_keys`
        and `pulse_on_dirty` will be pulsed if it is non-null. */
        std::set<key_t> dirty_keys;
        cond_t *pulse_on_dirty;
    };

    void on_connection_change(
        const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair);
    void stream_to_conn(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        auto_drainer_t::lock_t this_keepalive,
        typename std::map<connectivity_cluster_t::connection_t *, conn_info_t>::iterator
            conns_entry);

    connectivity_cluster_t *connectivity_cluster;
    connectivity_cluster_t::message_tag_t message_tag;
    watchable_map_t<key_t, value_t> *value;

    std::map<connectivity_cluster_t::connection_t *, conn_info_t> conns;
    uint64_t timestamp;

    /* Destructor order is important here. First we must destroy the subscriptions, so
    that they don't initiate any new coroutines that would need to lock `drainer`. Then
    we destroy `drainer`, which blocks until all the coroutines are done. Only after all
    the coroutines are done is it safe to destroy the other members. */

    auto_drainer_t drainer;

    typename watchable_map_t<key_t, value_t>::all_subs_t value_subs;
    typename watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>
        ::all_subs_t connections_subs;
};

#endif /* RPC_DIRECTORY_WRITE_MAP_MANAGER_HPP_ */

