// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_CLIENT_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_CLIENT_HPP_

#include <map>
#include <set>
#include <string>

#include "containers/incremental_lenses.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/server_metadata.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/view.hpp"

class server_config_client_t : public home_thread_mixin_t {
public:
    server_config_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t>
            *_directory_view,
        watchable_map_t<std::pair<peer_id_t, server_id_t>, empty_value_t>
            *_peer_connections_map);

    /* `get_server_config_map()` returns the server IDs and current configurations of
    every connected server. */
    watchable_map_t<server_id_t, server_config_versioned_t> *get_server_config_map() {
        return &server_config_map;
    }

    /* `get_peer_to_server_map()` and `get_server_to_peer_map()` allow conversion back
    and forth between server IDs and peer IDs. */
    watchable_map_t<peer_id_t, server_id_t> *get_peer_to_server_map() {
        return &peer_to_server_map;
    }
    watchable_map_t<server_id_t, peer_id_t> *get_server_to_peer_map() {
        return &server_to_peer_map;
    }

    /* This map contains the pair (X, Y) if we can see server X and server X can see
    server Y. */
    watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *get_connections_map() {
        return &connections_map;
    }

    /* `set_config()` changes the config of the server with the given server ID. */
    bool set_config(
        const server_id_t &server_id,
        const name_string_t &old_server_name,   /* for error messages */
        const server_config_t &new_server_config,
        signal_t *interruptor,
        std::string *error_out);

private:
    void install_server_metadata(
        const peer_id_t &peer_id,
        const cluster_directory_metadata_t &metadata);
    void on_directory_change(
        const peer_id_t &peer_id,
        const cluster_directory_metadata_t *metadata);
    void on_peer_connections_map_change(
        const std::pair<peer_id_t, server_id_t> &key,
        const empty_value_t *value);

    mailbox_manager_t *const mailbox_manager;
    watchable_map_t<peer_id_t, cluster_directory_metadata_t> * const directory_view;
    watchable_map_t< std::pair<peer_id_t, server_id_t>, empty_value_t>
        * const peer_connections_map;

    watchable_map_var_t<server_id_t, server_config_versioned_t> server_config_map;
    watchable_map_var_t<peer_id_t, server_id_t> peer_to_server_map;
    watchable_map_var_t<server_id_t, peer_id_t> server_to_peer_map;
    watchable_map_var_t<std::pair<server_id_t, server_id_t>, empty_value_t>
        connections_map;

    /* We use this to produce reasonable results when multiple peers have the same server
    ID. In general multiple peers cannot have the same server ID, but a server might
    conceivably shut down and then reconnect with a new peer ID before we had dropped the
    original connection. */
    std::multimap<server_id_t, peer_id_t> all_server_to_peer_map;

    watchable_map_t<peer_id_t, cluster_directory_metadata_t>::all_subs_t directory_subs;
    watchable_map_t<std::pair<peer_id_t, server_id_t>, empty_value_t>::all_subs_t
        peer_connections_map_subs;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_CLIENT_HPP_ */

