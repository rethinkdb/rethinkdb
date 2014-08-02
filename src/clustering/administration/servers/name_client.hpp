// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_NAME_CLIENT_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_NAME_CLIENT_HPP_

#include <map>
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

    /* All known servers, indexed by name. Warning: in the event of a name collision,
    this watchable will not update until the name collision has been resolved. The name
    collision should be resolved automatically in a fraction of a second, so this
    shouldn't be a big deal in practice, but be aware of it. */
    clone_ptr_t<watchable_t<std::map<name_string_t, machine_id_t> > >
    get_name_to_machine_id_map() {
        return name_to_machine_id_map.get_watchable();
    }

    /* All known servers, indexed by machine ID. */
    clone_ptr_t<watchable_t<std::map<machine_id_t, peer_id_t> > >
    get_machine_id_to_peer_id_map() {
        return machine_id_to_peer_id_map.get_watchable();
    }

    /* `rename_server` changes the name of the peer named `old_name` to `new_name`. On
    success, returns `true`. On failure, returns `false` and sets `*error_out` to an
    informative message. */
    bool rename_server(const name_string_t &old_name, const name_string_t &new_name,
        signal_t *interruptor, std::string *error_out);

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

    watchable_variable_t< std::map<name_string_t, machine_id_t> > name_to_machine_id_map;
    watchable_variable_t< std::map<machine_id_t, peer_id_t> > machine_id_to_peer_id_map;

    watchable_t< change_tracking_map_t<peer_id_t,
        cluster_directory_metadata_t> >::subscription_t directory_subs;
    semilattice_readwrite_view_t<machines_semilattice_metadata_t>::subscription_t
        semilattice_subs;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_NAME_CLIENT_HPP_ */

