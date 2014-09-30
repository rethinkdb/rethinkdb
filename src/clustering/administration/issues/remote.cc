#include "clustering/administration/issues/remote.hpp"

remote_issue_tracker_t::remote_issue_tracker_t(
        const clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, std::vector<local_issue_t> > > >
                &_issues_view,
        const clone_ptr_t<watchable_t<
            change_tracking_map_t<peer_id_t, machine_id_t> > >
                &_machine_id_translation_table) :
    issues_view(_issues_view),
    machine_id_translation_table(_machine_id_translation_table) { }

std::vector<scoped_ptr_t<issue_t> > remote_issue_tracker_t::get_issues() const {
    std::map<peer_id_t, std::vector<local_issue_t> > issues_by_peer =
        issues_view->get().get_inner();
    std::map<peer_id_t, machine_id_t> translation_table =
        machine_id_translation_table->get().get_inner();

    std::vector<local_issue_with_server_t> local_issues;
    for (auto const &peer_it : issues_by_peer) {
        auto const machine_id_it = translation_table.find(peer_it.first);
        if (machine_id_it == translation_table.end()) {
            /* This is unexpected. Did they disconnect after we got the
            issues list but before we got the translation table? In any
            case, do something safe. */
            continue;
        }

        for (auto const &local_issue : peer_it.second) {
            local_issue_with_server_t issue_with_server;
            std::vector<machine_id_t> affected_servers;
            affected_servers.push_back(machine_id_it->second);
            issue_with_server.issue = local_issue;
            issue_with_server.affected_servers = std::move(affected_servers);
            local_issues.push_back(std::move(issue_with_server));
        }
    }

    // Combine related issues from multiple servers
    reduce_outdated_index_issues(&local_issues);
    reduce_identical_issues<local_issue_t::server_ghost_data_t>(&local_issues);
    reduce_identical_issues<local_issue_t::server_down_data_t>(&local_issues);

    std::vector<scoped_ptr_t<issue_t> > res;
    for (auto const &local_issue : local_issues) {
        scoped_ptr_t<issue_t> new_issue;
        local_issue.issue.to_issue(local_issue.affected_servers, &new_issue);
        res.push_back(std::move(new_issue));
    }

    return res;
}

template <typename issue_type>
void remote_issue_tracker_t::reduce_identical_issues(
        std::vector<local_issue_with_server_t> *issues) {
    for (size_t i = 0; i < issues->size(); ++i) {
        auto data = boost::get<issue_type>(&issues->at(i).issue.issue_data);
        if (data == NULL) { continue; }
        for (size_t j = i + 1; j < issues->size(); ++j) {
            auto other_data = boost::get<issue_type>(&issues->at(j).issue.issue_data);
            if (other_data != NULL && data == other_data) {
                std::move(issues->at(j).affected_servers.begin(),
                          issues->at(j).affected_servers.end(),
                          std::back_inserter(issues->at(i).affected_servers));
                issues->erase(issues->begin() + i);
            }
        }
    }
}

// Outdated index issues are always combined into one
void remote_issue_tracker_t::reduce_outdated_index_issues(
        std::vector<local_issue_with_server_t> *issues) {
    for (size_t i = 0; i < issues->size(); ++i) {
        auto data = boost::get<local_issue_t::outdated_index_data_t>(
            &issues->at(i).issue.issue_data);
        if (data == NULL) { continue; }

        for (size_t j = i + 1; j < issues->size(); ++j) {
            auto other_data = boost::get<local_issue_t::outdated_index_data_t>(
                &issues->at(j).issue.issue_data);
            if (other_data != NULL) {
                std::move(issues->at(j).affected_servers.begin(),
                          issues->at(j).affected_servers.end(),
                          std::back_inserter(issues->at(i).affected_servers));
                for (auto const &table_it : other_data->indexes) {
                    if (!table_it.second.empty()) {
                        data->indexes[table_it.first].insert(table_it.second.begin(),
                                                             table_it.second.end());
                    }
                }
                issues->erase(issues->begin() + i);
            }
        }
    }
}
