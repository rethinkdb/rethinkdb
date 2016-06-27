// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_NETWORK_LOGGER_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_NETWORK_LOGGER_HPP_

#include <map>
#include <string>

#include "clustering/administration/servers/server_metadata.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

/* This class is responsible for writing log messages when a peer connects or
disconnects from us */

class network_logger_t {
public:
    network_logger_t(
        peer_id_t us,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *directory_view);

    /* This contains an entry for a given server ID if we can see a server with that ID.
    This will be piped over the network to other servers to form the full connections
    map. */
    watchable_map_t<server_id_t, empty_value_t> *get_local_connections_map() {
        return &local_connections;
    }

private:
    void on_change(const peer_id_t &peer_id, const cluster_directory_metadata_t *value);

    peer_id_t us;
    watchable_map_t<peer_id_t, cluster_directory_metadata_t> *directory_view;

    /* Whenever the directory changes, we compare the directory to
    `connected_servers` and `connected_proxies` to see what servers have
    connected or disconnected. */
    std::map<peer_id_t, server_id_t> connected_proxies;
    std::map<peer_id_t, std::pair<server_id_t, name_string_t> > connected_servers;

    watchable_map_var_t<server_id_t, empty_value_t> local_connections;

    watchable_map_t<peer_id_t, cluster_directory_metadata_t>::all_subs_t directory_subscription;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_NETWORK_LOGGER_HPP_ */
