// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_

#include <vector>
#include <string>

#include "clustering/administration/issues/local_issue_aggregator.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "containers/incremental_lenses.hpp"

// RSI(raft): Delete these files, or make major changes
#if 0

class server_config_server_t;
class server_config_client_t;

class server_issue_tracker_t {
public:
    server_issue_tracker_t(
        local_issue_aggregator_t *parent,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory_view,
        server_config_client_t *_server_config_client,
        server_config_server_t *_server_config_server);
    ~server_issue_tracker_t();

    static void combine(local_issues_t *local_issues,
                        std::vector<scoped_ptr_t<issue_t> > *issues_out);

private:
    void recompute();

    watchable_variable_t<std::vector<server_disconnected_issue_t> > disconnected_issues;
    watchable_variable_t<std::vector<server_ghost_issue_t> > ghost_issues;
    local_issue_aggregator_t::subscription_t<server_disconnected_issue_t> disconnected_subs;
    local_issue_aggregator_t::subscription_t<server_ghost_issue_t> ghost_subs;

    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
        cluster_sl_view;
    watchable_map_t<peer_id_t, cluster_directory_metadata_t> *directory_view;
    server_config_client_t *server_config_client;
    server_config_server_t *server_config_server;

    /* Note that we subscribe both to `directory_view` and to `server_config_client`'s 
    `get_server_id_to_peer_id_map()`. Since `get_server_id_to_peer_id_map()` is computed
    from the directory, these will always be updated in quick succession. But because our
    callback is delivered synchronously, it's possible that the callback will be
    called after one updates but before the other updates. Since we don't consider a
    server connected unless it appears in both, we need to make sure that we also get a
    callback after the second update occurs. So we subscribe to both. */
    semilattice_read_view_t<cluster_semilattice_metadata_t>::subscription_t
        cluster_sl_subs;
    watchable_map_t<peer_id_t, cluster_directory_metadata_t>::all_subs_t directory_subs;
    watchable_t<std::map<server_id_t, peer_id_t> >::subscription_t
        server_config_client_subs;
    DISABLE_COPYING(server_issue_tracker_t);
};

#endif

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_ */
