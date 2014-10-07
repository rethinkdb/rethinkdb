// Copyright 2010-2012 RethinkDB, all rights reserved.
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
    class update_writer_t;

    void on_connections_change();
    void send_update(
        connectivity_cluster_t::connection_t *connection,
        auto_drainer_t::lock_t connection_keepalive,
        auto_drainer_t::lock_t this_keepalive,
        const key_t &key);

    connectivity_cluster_t *connectivity_cluster;
    connectivity_cluster_t::message_tag_t message_tag;
    watchable_map_t<key_t, value_t> *value;

    std::map<connectivity_cluster_t::connection_t *, auto_drainer_t::lock_t>
        last_connections;
    uint64_t timestamp;

    /* Any time we want to write to the network, we acquire this first. */
    new_semaphore_t semaphore;

    /* Destructor order is important here. First we must destroy the subscriptions, so
    that they don't initiate any new coroutines that would need to lock `drainer`. Then
    we destroy `drainer`, which blocks until all the coroutines are done. Only after all
    the coroutines are done is it safe to destroy the other members. */

    auto_drainer_t drainer;

    typename watchable_map_t<key_t, value_t>::all_subs_t value_subs;
    typename watchable_t<connectivity_cluster_t::connection_map_t>::subscription_t
        connections_subs;
};

#endif /* RPC_DIRECTORY_WRITE_MAP_MANAGER_HPP_ */

