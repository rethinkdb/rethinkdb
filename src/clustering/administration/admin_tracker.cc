// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/admin_tracker.hpp"

#include <map>
#include <vector>

#include "clustering/administration/main/watchable_fields.hpp"
#include "rpc/semilattice/view/field.hpp"

admin_tracker_t::admin_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > cluster_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &directory_view,
        local_issue_aggregator_t *local_issue_aggregator) :
    issue_multiplexer(cluster_view),

    remote_issue_tracker(
        &issue_multiplexer,
        directory_view->incremental_subview(
            incremental_field_getter_t<std::vector<local_issue_t>,
                                       cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::local_issues)),
        directory_view->incremental_subview(
            incremental_field_getter_t<machine_id_t, cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::machine_id))),

    name_conflict_issue_tracker(&issue_multiplexer,
                                cluster_view),

    server_issue_tracker(
        local_issue_aggregator,
        cluster_view,
        directory_view->incremental_subview(
            incremental_field_getter_t<machine_id_t, cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::machine_id))),

    outdated_index_tracker(local_issue_aggregator),

    last_seen_tracker(
        metadata_field(&cluster_semilattice_metadata_t::machines, cluster_view),
        directory_view->incremental_subview(
            incremental_field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)))
{
}

admin_tracker_t::~admin_tracker_t() {}
