// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_

#include <vector>
#include <set>
#include <string>

#include "clustering/administration/issues/issue.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

class name_collision_issue_tracker_t : public issue_tracker_t {
public:
    name_collision_issue_tracker_t(
        server_config_client_t *_server_config_client,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        table_meta_client_t *_table_meta_client);

    ~name_collision_issue_tracker_t();

    std::vector<scoped_ptr_t<issue_t> > get_issues(signal_t *interruptor) const;

private:
    server_config_client_t *server_config_client;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
        cluster_sl_view;
    table_meta_client_t *table_meta_client;

    DISABLE_COPYING(name_collision_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_NAME_COLLISION_HPP_ */
