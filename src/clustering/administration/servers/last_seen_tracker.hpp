// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_LAST_SEEN_TRACKER_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_LAST_SEEN_TRACKER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/servers/server_metadata.hpp"
#include "concurrency/watchable.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/semilattice/view.hpp"

class last_seen_tracker_t {
public:
    last_seen_tracker_t(
            const boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > &servers_view,
            const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, server_id_t> > > &server_id_map);

    /* Fetches the time the given server [dis]connected, on the assumption that it is
    currently [dis]connected. If the server was permanently removed, the behavior is
    undefined. */
    microtime_t get_connected_time(const server_id_t &server_id) {
        auto it = connected_times.find(server_id);
        if (it != connected_times.end()) {
            return it->second;
        } else {
            /* The server just connected, but we haven't noticed that it's here yet. So
            the connection time is now. */
            return current_microtime();
        }
    }
    microtime_t get_disconnected_time(const server_id_t &server_id) {
        auto it = disconnected_times.find(server_id);
        if (it != disconnected_times.end()) {
            return it->second;
        } else {
            /* The server just disconnected, but we haven't noticed that it's gone yet.
            So the disconnection time is now. */
            return current_microtime();
        }
    }

private:
    void update();
    void on_servers_view_change();
    void on_server_id_map_change();

    boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > servers_view;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, server_id_t> > > server_id_map;
    semilattice_read_view_t<servers_semilattice_metadata_t>::subscription_t servers_view_subs;
    watchable_t<change_tracking_map_t<peer_id_t, server_id_t> >::subscription_t server_id_map_subs;

    /* Servers are only present in this map if they are not connected but not
    declared dead. */
    std::map<server_id_t, microtime_t> disconnected_times;

    /* Servers are only present in this map if they are connected and not declared
    dead. */
    std::map<server_id_t, microtime_t> connected_times;

    DISABLE_COPYING(last_seen_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_LAST_SEEN_TRACKER_HPP_ */
