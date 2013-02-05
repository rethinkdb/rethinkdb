// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <map>
#include <list>

#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "rpc/semilattice/view/field.hpp"

admin_tracker_t::admin_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &directory_view) :
    issue_aggregator(),

    remote_issue_tracker(
        directory_view->subview(
            field_getter_t<std::list<local_issue_t>, cluster_directory_metadata_t>(&cluster_directory_metadata_t::local_issues)),
        directory_view->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))),
    remote_issue_tracker_feed(&issue_aggregator, &remote_issue_tracker),

    machine_down_issue_tracker(
        semilattice_view,
        directory_view->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))),
    machine_down_issue_tracker_feed(&issue_aggregator, &machine_down_issue_tracker),

    name_conflict_issue_tracker(semilattice_view),
    name_conflict_issue_tracker_feed(&issue_aggregator, &name_conflict_issue_tracker),

    vector_clock_conflict_issue_tracker(semilattice_view),
    vector_clock_issue_tracker_feed(&issue_aggregator, &vector_clock_conflict_issue_tracker),

    mc_pinnings_shards_mismatch_issue_tracker(metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_view)),
    mc_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &mc_pinnings_shards_mismatch_issue_tracker),

    dummy_pinnings_shards_mismatch_issue_tracker(metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_view)),
    dummy_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &dummy_pinnings_shards_mismatch_issue_tracker),

    unsatisfiable_goals_issue_tracker(semilattice_view),
    unsatisfiable_goals_issue_tracker_feed(&issue_aggregator, &unsatisfiable_goals_issue_tracker),

    last_seen_tracker(
        metadata_field(&cluster_semilattice_metadata_t::machines, semilattice_view),
        directory_view->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id)))
{
}

admin_tracker_t::~admin_tracker_t() {}
