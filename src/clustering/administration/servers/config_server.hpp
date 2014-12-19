// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_SERVER_HPP_

#include <set>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/server_metadata.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/semilattice/view.hpp"

/* A `server_config_server_t` is responsible for managing a server's name. Server names are
set on the command line when RethinkDB is first started, and they can be changed at
runtime. They are stored in the semilattices and persisted across restarts.

A server's name entry in the semilattices should only be modified by that server itself,
although other servers may delete the entry if the server is being permanently removed;
so all rename requests must be first sent to the `server_config_server_t` for the server
being renamed.

A server's tags and cache size are also handled and configured in the same way. */

class server_config_server_t : public home_thread_mixin_t {
public:
    server_config_server_t(
        mailbox_manager_t *_mailbox_manager,
        server_id_t _my_server_id,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
            _semilattice_view
        );

    server_config_business_card_t get_business_card();

    clone_ptr_t<watchable_t<name_string_t> > get_my_name() {
        guarantee(!permanently_removed_cond.is_pulsed());
        return my_name.get_watchable();
    }

    clone_ptr_t<watchable_t<std::set<name_string_t> > > get_my_tags() {
        guarantee(!permanently_removed_cond.is_pulsed());
        return my_tags.get_watchable();
    }

    /* Returns the actual cache size, not the cache size setting. If the cache size
    setting is "auto", the actual cache size will be some reasonable automatically
    selected value; otherwise, the actual cache size will be the cache size setting. */
    clone_ptr_t<watchable_t<uint64_t> > get_actual_cache_size_bytes() {
        return actual_cache_size_bytes.get_watchable();
    }

    signal_t *get_permanently_removed_signal() {
        return &permanently_removed_cond;
    }

private:
    /* `on_change_name_request()` is called in response to a rename request over the
    network. It renames unconditionally, without checking for name collisions. */
    void on_change_name_request(
        signal_t *interruptor,
        const name_string_t &new_name,
        mailbox_t<void(std::string)>::address_t ack_addr);

    /* `on_change_tags_request()` is called in response to a tag change request over the
    network */
    void on_change_tags_request(
        signal_t *interruptor,
        const std::set<name_string_t> &new_tags,
        mailbox_t<void(std::string)>::address_t ack_addr);

    /* `on_change_cache_size_request()` is called in response to a cache-size change
    request over the network. */
    void on_change_cache_size_request(
        signal_t *interruptor,
        boost::optional<uint64_t> new_cache_size,
        mailbox_t<void(std::string)>::address_t ack_addr);

    /* `on_semilattice_change()` checks if we have been permanently removed. It does not
    block. */
    void on_semilattice_change();

    void update_actual_cache_size(const boost::optional<uint64_t> &setting);

    mailbox_manager_t *mailbox_manager;
    server_id_t my_server_id;
    watchable_variable_t<name_string_t> my_name;
    watchable_variable_t<std::set<name_string_t> > my_tags;
    watchable_variable_t<uint64_t> actual_cache_size_bytes;
    cond_t permanently_removed_cond;

    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> > > directory_view;
    boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
        semilattice_view;

    server_config_business_card_t::change_name_mailbox_t change_name_mailbox;
    server_config_business_card_t::change_tags_mailbox_t change_tags_mailbox;
    server_config_business_card_t::change_cache_size_mailbox_t change_cache_size_mailbox;
    semilattice_readwrite_view_t<servers_semilattice_metadata_t>::subscription_t
        semilattice_subs;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_CONFIG_SERVER_HPP_ */

