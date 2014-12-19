// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_REMOTE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_REMOTE_HPP_

#include <vector>

#include "clustering/administration/issues/issue.hpp"
#include "clustering/administration/issues/local.hpp"
#include "containers/uuid.hpp"
#include "clustering/administration/servers/server_metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "containers/incremental_lenses.hpp"
#include "http/json/json_adapter.hpp"

class remote_issue_tracker_t : public issue_tracker_t {
public:
    remote_issue_tracker_t(
        const clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, local_issues_t> > >
                &_issues_view,
        const clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, server_id_t> > >
                &_server_id_translation_table);

    std::vector<scoped_ptr_t<issue_t> > get_issues() const;

private:
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, local_issues_t> > >
        issues_view;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, server_id_t> > >
        server_id_translation_table;

    DISABLE_COPYING(remote_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_REMOTE_HPP_ */
