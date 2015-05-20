// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/remote.hpp"
#include "clustering/administration/issues/local_issue_aggregator.hpp"
#include "clustering/administration/issues/outdated_index.hpp"
#include "clustering/administration/issues/log_write.hpp"
#include "clustering/administration/issues/server.hpp"

template <typename T>
void add_issues_from_server(const std::vector<T> &source,
                            std::vector<T> *dest,
                            const server_id_t &server) {
    size_t original_size = dest->size();
    std::copy(source.begin(), source.end(), std::back_inserter(*dest));
    
    for (size_t i = original_size; i < dest->size(); ++i) {
        dest->at(i).add_server(server);
    }
}


remote_issue_tracker_t::remote_issue_tracker_t(
        const clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, local_issues_t> > >
                &_issues_view,
        const clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, server_id_t> > >
                &_server_id_translation_table) :
    issues_view(_issues_view),
    server_id_translation_table(_server_id_translation_table) { }

std::vector<scoped_ptr_t<issue_t> > remote_issue_tracker_t::get_issues() const {
    std::map<peer_id_t, local_issues_t> issues_by_peer =
        issues_view->get().get_inner();
    std::map<peer_id_t, server_id_t> translation_table =
        server_id_translation_table->get().get_inner();

    local_issues_t local_issues;
    for (auto const &peer_it : issues_by_peer) {
        auto const server_id_it = translation_table.find(peer_it.first);
        if (server_id_it == translation_table.end()) {
            /* This is unexpected. Did they disconnect after we got the
            issues list but before we got the translation table? In any
            case, do something safe. */
            continue;
        }

        add_issues_from_server(peer_it.second.log_write_issues,
                               &local_issues.log_write_issues,
                               server_id_it->second);

        add_issues_from_server(peer_it.second.outdated_index_issues,
                               &local_issues.outdated_index_issues,
                               server_id_it->second);
    }

    // Combine related issues from multiple servers
    std::vector<scoped_ptr_t<issue_t> > res;
    log_write_issue_tracker_t::combine(&local_issues, &res);
    outdated_index_issue_tracker_t::combine(&local_issues, &res);
    return res;
}
