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

class auto_reconnector_t {
public:
    auto_reconnector_t(
        connectivity_cluster_t *connectivity_cluster,
        connectivity_cluster_t::run_t *connectivity_cluster_run);

private:
    void on_connect_or_disconnect();

    /* `try_reconnect()` runs in a coroutine. It automatically terminates
    itself if the server ever reconnects. */
    void try_reconnect(server_id_t server, auto_drainer_t::lock_t drainer);

    void pulse_if_server_declared_dead(server_id_t server, cond_t *c);
    void pulse_if_server_reconnected(server_id_t server, cond_t *c);

    connectivity_cluster_t *connectivity_cluster;
    connectivity_cluster_t::run_t *connectivity_cluster_run;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, server_id_t> > > server_id_translation_table;
    boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > server_metadata;

    /* `addresses` contains the last known address of every server we've ever seen,
    unless it has been declared dead. */
    std::map<server_id_t, peer_address_t> addresses;

    /* `server_ids` contains the server IDs of servers that are currently connected. We
    detect connection and disconnection events by comparing this to the list of currently
    connected servers we get from the `connectivity_cluster_t`. */
    std::map<peer_id_t, server_id_t> server_ids;

    auto_drainer_t drainer;

    watchable_map_t<peer_id_t, connectivity_cluster_t::connection_pair_t>::all_subs_t
        connection_subs;

    DISABLE_COPYING(auto_reconnector_t);
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_AUTO_RECONNECT_HPP_ */
