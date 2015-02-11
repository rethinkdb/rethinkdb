// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_WRITE_MANAGER_HPP_
#define RPC_DIRECTORY_WRITE_MANAGER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/new_semaphore.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/connectivity/cluster.hpp"

class message_service_t;

template<class metadata_t>
class directory_write_manager_t {
public:
    directory_write_manager_t(
        connectivity_cluster_t *connectivity_cluster,
        connectivity_cluster_t::message_tag_t message_tag,
        const clone_ptr_t<watchable_t<metadata_t> > &value) THROWS_NOTHING;

private:
    class initialization_writer_t;
    class update_writer_t;

    void on_connection_change(
        const peer_id_t &peer_id,
        const connectivity_cluster_t::connection_pair_t *pair) THROWS_NOTHING;
    void on_value_change() THROWS_NOTHING;

    connectivity_cluster_t *connectivity_cluster;
    connectivity_cluster_t::message_tag_t message_tag;
    clone_ptr_t<watchable_t<metadata_t> > value;

    fifo_enforcer_source_t metadata_fifo_source;
    std::map<peer_id_t, connectivity_cluster_t::connection_pair_t> last_connections;
    /* protects `metadata_fifo_source` and `last_connections` */
    mutex_assertion_t mutex_assertion;

    /* Any time we want to write to the network, we acquire this first. */
    new_semaphore_t semaphore;

    /* Destructor order is important here. First we must destroy the subscriptions, so
    that they don't initiate any new coroutines that would need to lock `drainer`. Then
    we destroy `drainer`, which blocks until all the coroutines are done. Only after all
    the coroutines are done is it safe to destroy the other members. */

    auto_drainer_t drainer;

    typename watchable_t<metadata_t>::subscription_t
        value_change_subscription;
    typename watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>
        ::all_subs_t connections_change_subscription;

    DISABLE_COPYING(directory_write_manager_t);
};

#endif /* RPC_DIRECTORY_WRITE_MANAGER_HPP_ */
