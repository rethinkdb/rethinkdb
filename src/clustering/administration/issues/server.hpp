// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_

#include <vector>
#include <string>

#include "clustering/administration/issues/local_issue_aggregator.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "containers/incremental_lenses.hpp"

class server_issue_tracker_t : public local_issue_tracker_t {
public:
    server_issue_tracker_t(
        local_issue_aggregator_t *_parent,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > >
            &_machine_to_peer);
    ~server_issue_tracker_t();

    static void combine(local_issues_t *local_issues,
                        std::vector<scoped_ptr_t<issue_t> > *issues_out);

private:
    void recompute();

    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
        cluster_sl_view;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > >
        machine_to_peer;
    watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> >::subscription_t
        machine_to_peer_subs;

    DISABLE_COPYING(server_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_ */
