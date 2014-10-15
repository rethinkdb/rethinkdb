// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_NAME_CLIENT_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_NAME_CLIENT_HPP_

#include <map>
#include <set>
#include <string>

#include "containers/incremental_lenses.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/name_metadata.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/view.hpp"

class server_name_client_t : public home_thread_mixin_t {
public:
    server_name_client_t(
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<machines_semilattice_metadata_t> >
            _semilattice_view);

    /* This map contains all known servers, not counting permanently removed ones. */
    clone_ptr_t<watchable_t<std::multimap<name_string_t, machine_id_t> > >
    get_name_to_machine_id_map() {
        return name_to_machine_id_map.get_watchable();
    }

    /* The inverse of `get_name_to_machine_id_map()`. */
    clone_ptr_t<watchable_t<std::map<machine_id_t, name_string_t> > >
    get_machine_id_to_name_map() {
        return machine_id_to_name_map.get_watchable();
    }

    /* This is equivalent to a lookup in `get_machine_id_to_name_map` */
    boost::optional<name_string_t> get_name_for_machine_id(const machine_id_t &m) {
        boost::optional<name_string_t> out;
        machine_id_to_name_map.apply_read(
            [&](const std::map<machine_id_t, name_string_t> *map) {
                auto it = map->find(m);
                if (it != map->end()) {
                    out = boost::optional<name_string_t>(it->second);
                }
            });
        return out;
    }

    /* This map contains all known servers, not counting permanently removed ones. If a
    machine is not connected, it will have a nil `peer_id_t`. */
    clone_ptr_t<watchable_t<std::map<machine_id_t, peer_id_t> > >
    get_machine_id_to_peer_id_map() {
        return machine_id_to_peer_id_map.get_watchable();
    }

    /* This is equivalent to a lookup in `get_machine_id_to_peer_id_map` */
    boost::optional<peer_id_t> get_peer_id_for_machine_id(const machine_id_t &m) {
        boost::optional<peer_id_t> out;
        machine_id_to_peer_id_map.apply_read(
            [&](const std::map<machine_id_t, peer_id_t> *map) {
                auto it = map->find(m);
                if (it != map->end()) {
                    out = boost::optional<peer_id_t>(it->second);
                }
            });
        return out;
    }

    /* This map contains all connected servers, not counting permanently removed ones. */
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > >
    get_peer_id_to_machine_id_map() {
        return peer_id_to_machine_id_map.get_watchable();
    }

    /* This is equivalent to a lookup in `get_peer_id_to_machine_id_map` */
    boost::optional<machine_id_t> get_machine_id_for_peer_id(const peer_id_t &peer) {
        boost::optional<machine_id_t> out;
        peer_id_to_machine_id_map.apply_read(
            [&](const std::map<peer_id_t, machine_id_t> *map) {
                auto it = map->find(peer);
                if (it != map->end()) {
                    out = boost::optional<machine_id_t>(it->second);
                }
            });
        return out;
    }

    /* Returns all servers with the given tag, not counting permanently removed ones.
    RSI(reql_admin): Decide if this should count disconnected servers or not. */
    std::set<machine_id_t> get_servers_with_tag(const name_string_t &tag);

    /* `rename_server` changes the name of the peer named `old_name` to `new_name`. On
    success, returns `true`. On failure, returns `false` and sets `*error_out` to an
    informative message. */
    bool rename_server(
        const machine_id_t &server_id,
        const name_string_t &server_name,   /* for error messages */
        const name_string_t &new_name,
        signal_t *interruptor,
        std::string *error_out);

    /* `retag_server` changes the tags of the server with the given machine ID. On
    success, returns `true`. On failure, returns `false` and sets `*error_out` to an
    informative message. */
    bool retag_server(
        const machine_id_t &server,
        const name_string_t &server_name,   /* for error messages */
        const std::set<name_string_t> &new_tags,
        signal_t *interruptor,
        std::string *error_out);

    /* `permanently_remove_server` permanently removes the server with the given name,
    provided that it is not currently visible. On success, returns `true`. On failure,
    returns `false` and sets `*error_out` to an informative message. */
    bool permanently_remove_server(const name_string_t &name, std::string *error_out);

private:
    void recompute_name_to_machine_id_map();
    void recompute_machine_id_to_peer_id_map();

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> > > directory_view;
    boost::shared_ptr< semilattice_readwrite_view_t<machines_semilattice_metadata_t> >
        semilattice_view;

    watchable_variable_t< std::map<machine_id_t, peer_id_t> > machine_id_to_peer_id_map;
    watchable_variable_t< std::map<peer_id_t, machine_id_t> > peer_id_to_machine_id_map;
    watchable_variable_t< std::multimap<name_string_t, machine_id_t> >
        name_to_machine_id_map;
    watchable_variable_t< std::map<machine_id_t, name_string_t> > machine_id_to_name_map;

    watchable_t< change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> >::subscription_t directory_subs;
    semilattice_readwrite_view_t<machines_semilattice_metadata_t>::subscription_t
        semilattice_subs;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_NAME_CLIENT_HPP_ */

