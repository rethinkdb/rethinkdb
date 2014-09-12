// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ADMIN_TRACKER_HPP_
#define CLUSTERING_ADMINISTRATION_ADMIN_TRACKER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/view.hpp"

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local_to_global.hpp"
#include "clustering/administration/issues/server.hpp"
#include "clustering/administration/issues/name_conflict.hpp"
#include "clustering/administration/issues/outdated_index.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/last_seen_tracker.hpp"

template <class> class watchable_t;

class admin_tracker_t {
public:
    admin_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > cluster_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &directory_view,
        local_issue_aggregator_t *local_issue_aggregator);

    ~admin_tracker_t();

    issue_multiplexer_t issue_multiplexer;

    // Global issues
    remote_issue_tracker_t remote_issue_tracker;
    name_conflict_issue_tracker_t name_conflict_issue_tracker;

    // Local issues
    server_issue_tracker_t server_issue_tracker;
    outdated_index_issue_tracker_t outdated_index_tracker;

    last_seen_tracker_t last_seen_tracker;
};

#endif  // CLUSTERING_ADMINISTRATION_ADMIN_TRACKER_HPP_
