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
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
            _semilattice_view);

    /* This map contains all known servers, not counting permanently removed ones. */
    clone_ptr_t<watchable_t<std::multimap<name_string_t, server_id_t> > >
    get_name_to_server_id_map() {
        return name_to_server_id_map.get_watchable();
    }

    /* The inverse of `get_name_to_server_id_map()`. */
    clone_ptr_t<watchable_t<std::map<server_id_t, name_string_t> > >
    get_server_id_to_name_map() {
        return server_id_to_name_map.get_watchable();
    }

    /* This is equivalent to a lookup in `get_server_id_to_name_map` */
    boost::optional<name_string_t> get_name_for_server_id(const server_id_t &m) {
        boost::optional<name_string_t> out;
        server_id_to_name_map.apply_read(
            [&](const std::map<server_id_t, name_string_t> *map) {
                auto it = map->find(m);
                if (it != map->end()) {
                    out = boost::optional<name_string_t>(it->second);
                }
            });
        return out;
    }

    /* This map contains all connected servers, including permanently removed ones,
       and excluding proxy servers. */
    clone_ptr_t<watchable_t<std::map<server_id_t, peer_id_t> > >
    get_server_id_to_peer_id_map() {
        return server_id_to_peer_id_map.get_watchable();
    }

    /* This is equivalent to a lookup in `get_server_id_to_peer_id_map` */
    boost::optional<peer_id_t> get_peer_id_for_server_id(const server_id_t &m) {
        boost::optional<peer_id_t> out;
        server_id_to_peer_id_map.apply_read(
            [&](const std::map<server_id_t, peer_id_t> *map) {
                auto it = map->find(m);
                if (it != map->end()) {
                    out = boost::optional<peer_id_t>(it->second);
                }
            });
        return out;
    }

    /* This map contains all connected servers, including permanently removed ones,
       and excluding proxy servers. */
    clone_ptr_t<watchable_t<std::map<peer_id_t, server_id_t> > >
    get_peer_id_to_server_id_map() {
        return peer_id_to_server_id_map.get_watchable();
    }

    /* This is equivalent to a lookup in `get_peer_id_to_server_id_map` */
    boost::optional<server_id_t> get_server_id_for_peer_id(const peer_id_t &peer) {
        boost::optional<server_id_t> out;
        peer_id_to_server_id_map.apply_read(
            [&](const std::map<peer_id_t, server_id_t> *map) {
                auto it = map->find(peer);
                if (it != map->end()) {
                    out = boost::optional<server_id_t>(it->second);
                }
            });
        return out;
    }

    /* Returns all servers with the given tag, counting disconnected servers but not
    counting permanently removed ones. */
    std::set<server_id_t> get_servers_with_tag(const name_string_t &tag);

    /* `change_server_name` changes the name of the peer named `old_name` to `new_name`.
    On success, returns `true`. On failure, returns `false` and sets `*error_out` to an
    informative message. */
    bool change_server_name(
        const server_id_t &server_id,
        const name_string_t &server_name,   /* for error messages */
        const name_string_t &new_name,
        signal_t *interruptor,
        std::string *error_out);

    /* `change_server_tags` changes the tags of the server with the given server ID. On
    success, returns `true`. On failure, returns `false` and sets `*error_out` to an
    informative message. */
    bool change_server_tags(
        const server_id_t &server,
        const name_string_t &server_name,   /* for error messages */
        const std::set<name_string_t> &new_tags,
        signal_t *interruptor,
        std::string *error_out);

    /* `change_server_cache_size` changes the cache size of the server with the given
    server ID. On success, returns `true`. On failure, returns `false` and sets
    `*error_out` to an informative message. */
    bool change_server_cache_size(
        const server_id_t &server,
        const name_string_t &server_name,   /* for error messages */
        const boost::optional<uint64_t> &new_cache_size_bytes,
        signal_t *interruptor,
        std::string *error_out);

    /* `permanently_remove_server` permanently removes the server with the given name,
    provided that it is not currently visible. On success, returns `true`. On failure,
    returns `false` and sets `*error_out` to an informative message. */
    bool permanently_remove_server(const name_string_t &name, std::string *error_out);

private:
    /* Helper function for `change_server_*()` */
    bool do_change(
        const server_id_t &server,
        const name_string_t &server_name,   /* for error messages */
        const std::string &what_is_changing,   /* for error messages */
        const std::function<void(
            const server_config_business_card_t &bc,
            const mailbox_t<void(std::string)>::address_t &reply_addr
            )> &sender,
        signal_t *interruptor,
        std::string *error_out);

    void recompute_name_to_server_id_map();
    void recompute_server_id_to_peer_id_map();

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> > > directory_view;
    boost::shared_ptr< semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
        semilattice_view;

    watchable_variable_t< std::map<server_id_t, peer_id_t> > server_id_to_peer_id_map;
    watchable_variable_t< std::map<peer_id_t, server_id_t> > peer_id_to_server_id_map;
    watchable_variable_t< std::multimap<name_string_t, server_id_t> >
        name_to_server_id_map;
    watchable_variable_t< std::map<server_id_t, name_string_t> > server_id_to_name_map;

    watchable_t< change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> >::subscription_t directory_subs;
    semilattice_readwrite_view_t<servers_semilattice_metadata_t>::subscription_t
        semilattice_subs;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_CLIENT_HPP_ */

