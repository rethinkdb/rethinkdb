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
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/issues/name_conflict.hpp"
#include "clustering/administration/issues/unsatisfiable_goals.hpp"
#include "clustering/administration/issues/vector_clock_conflict.hpp"
#include "clustering/administration/last_seen_tracker.hpp"
#include "clustering/administration/metadata.hpp"

template <class> class watchable_t;

struct admin_tracker_t {
    admin_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > cluster_view,
        boost::shared_ptr<semilattice_read_view_t<auth_semilattice_metadata_t> > auth_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &directory_view);

    ~admin_tracker_t();

    global_issue_aggregator_t issue_aggregator;
    remote_issue_collector_t remote_issue_tracker;
    global_issue_aggregator_t::source_t remote_issue_tracker_feed;
    machine_down_issue_tracker_t machine_down_issue_tracker;
    global_issue_aggregator_t::source_t machine_down_issue_tracker_feed;
    name_conflict_issue_tracker_t name_conflict_issue_tracker;
    global_issue_aggregator_t::source_t name_conflict_issue_tracker_feed;
    vector_clock_conflict_issue_tracker_t vector_clock_conflict_issue_tracker;
    global_issue_aggregator_t::source_t vector_clock_issue_tracker_feed;

    unsatisfiable_goals_issue_tracker_t unsatisfiable_goals_issue_tracker;
    global_issue_aggregator_t::source_t unsatisfiable_goals_issue_tracker_feed;

    last_seen_tracker_t last_seen_tracker;
};

#endif  // CLUSTERING_ADMINISTRATION_ADMIN_TRACKER_HPP_
