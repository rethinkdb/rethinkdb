// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_AUTO_RECONNECT_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_AUTO_RECONNECT_HPP_

#include <map>
#include <utility>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/servers/server_metadata.hpp"
#include "concurrency/watchable.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/semilattice/view.hpp"

class server_config_client_t;

class auto_reconnector_t {
public:
    auto_reconnector_t(
        connectivity_cluster_t *connectivity_cluster,
        connectivity_cluster_t::run_t *connectivity_cluster_run,
        server_config_client_t *server_config_client,
        const int join_delay_secs,
        const int give_up_ms);

private:
    void on_connect_or_disconnect(const peer_id_t &peer_id);

    /* `try_reconnect()` runs in a coroutine. It automatically terminates
    itself if the server ever reconnects. */
    void try_reconnect(const server_id_t &server, auto_drainer_t::lock_t drainer);

    connectivity_cluster_t *const connectivity_cluster;
    connectivity_cluster_t::run_t *const connectivity_cluster_run;
    server_config_client_t *const server_config_client;

    /* `addresses` contains the last known address of every server we've ever seen */
    std::map<server_id_t, peer_address_t> addresses;

    /* `server_ids` contains the server IDs of servers that are currently connected. We
    detect connection and disconnection events by comparing this to the list of currently
    connected servers we get from the `connectivity_cluster_t`. */
    std::map<peer_id_t, server_id_t> server_ids;

    /* `stop_conds` contains an entry for each running instance of `try_reconnect()`.
    It's used to interrupt the coroutines if the server reconnects. */
    std::multimap<server_id_t, cond_t *> stop_conds;

    int join_delay_secs;
    int give_up_ms;

    auto_drainer_t drainer;

    watchable_map_t<peer_id_t, server_id_t>::all_subs_t
        server_id_subs;
    watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>::all_subs_t
        connection_subs;

    DISABLE_COPYING(auto_reconnector_t);
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_AUTO_RECONNECT_HPP_ */
