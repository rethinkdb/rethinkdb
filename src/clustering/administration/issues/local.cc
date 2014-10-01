// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/issues/local.hpp"

#include "errors.hpp"
#include <boost/variant/get.hpp>

#include "utils.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/boost_types.hpp"
#include "clustering/administration/issues/server.hpp"
#include "clustering/administration/issues/log_write.hpp"
#include "clustering/administration/issues/outdated_index.hpp"

RDB_IMPL_SERIALIZABLE_1(local_issue_t::server_down_data_t, server_id);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(local_issue_t::server_down_data_t);
RDB_IMPL_EQUALITY_COMPARABLE_1(local_issue_t::server_down_data_t, server_id);

RDB_IMPL_SERIALIZABLE_1(local_issue_t::server_ghost_data_t, server_id);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(local_issue_t::server_ghost_data_t);
RDB_IMPL_EQUALITY_COMPARABLE_1(local_issue_t::server_ghost_data_t, server_id);

RDB_IMPL_SERIALIZABLE_1(local_issue_t::outdated_index_data_t, indexes);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(local_issue_t::outdated_index_data_t);
RDB_IMPL_EQUALITY_COMPARABLE_1(local_issue_t::outdated_index_data_t, indexes);

RDB_IMPL_SERIALIZABLE_1(local_issue_t::log_write_data_t, message);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(local_issue_t::log_write_data_t);
RDB_IMPL_EQUALITY_COMPARABLE_1(local_issue_t::log_write_data_t, message);

RDB_IMPL_SERIALIZABLE_2(local_issue_t, issue_time, issue_data);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(local_issue_t);
RDB_IMPL_EQUALITY_COMPARABLE_2(local_issue_t, issue_time, issue_data);

local_issue_t::local_issue_t() { }

local_issue_t::~local_issue_t() { }

local_issue_t local_issue_t::make_server_down_issue(const machine_id_t &server_id) {
    local_issue_t server_down_issue;
    server_down_data_t data;
    data.server_id = server_id;
    server_down_issue.issue_data = data;
    server_down_issue.issue_time = current_microtime();
    return server_down_issue;
}

local_issue_t local_issue_t::make_server_ghost_issue(const machine_id_t &server_id) {
    local_issue_t server_ghost_issue;
    server_ghost_data_t data;
    data.server_id = server_id;
    server_ghost_issue.issue_data = data;
    server_ghost_issue.issue_time = current_microtime();
    return server_ghost_issue;
}

local_issue_t local_issue_t::make_outdated_index_issue(const outdated_index_map_t &indexes) {
    local_issue_t outdated_index_issue;
    outdated_index_data_t data;
    data.indexes = indexes;
    outdated_index_issue.issue_data = data;
    outdated_index_issue.issue_time = current_microtime();
    return outdated_index_issue;
}

local_issue_t local_issue_t::make_log_write_issue(const std::string &message) {
    local_issue_t log_write_issue;
    log_write_data_t data;
    data.message = message;
    log_write_issue.issue_data = data;
    log_write_issue.issue_time = current_microtime();
    return log_write_issue;
}

void local_issue_t::to_issue(const std::vector<machine_id_t> &affected_servers,
                             scoped_ptr_t<issue_t> *issue_out) const {
    if (auto server_down = boost::get<server_down_data_t>(&issue_data)) {
        issue_out->init(new server_down_issue_t(server_down->server_id, affected_servers));
    } else if (auto server_ghost = boost::get<server_ghost_data_t>(&issue_data)) {
        issue_out->init(new server_ghost_issue_t(server_ghost->server_id, affected_servers));
    } else if (auto outdated_index = boost::get<outdated_index_data_t>(&issue_data)) {
        issue_out->init(new outdated_index_issue_t(outdated_index->indexes));
    } else if (auto log_write = boost::get<log_write_data_t>(&issue_data)) {
        issue_out->init(new log_write_issue_t(log_write->message, affected_servers));
    } else {
        unreachable();
    }
}

local_issue_tracker_t::local_issue_tracker_t(local_issue_aggregator_t *_parent) : parent(_parent) {
    parent->add_tracker(this);
}

local_issue_tracker_t::~local_issue_tracker_t() {
    parent->remove_tracker(this);
}

void local_issue_tracker_t::update(const std::vector<local_issue_t> &issues) {
    parent->update_tracker_issues(this, issues);
}

local_issue_aggregator_t::local_issue_aggregator_t() :
    issues_watchable(std::vector<local_issue_t>()) { }

clone_ptr_t<watchable_t<std::vector<local_issue_t> > >
local_issue_aggregator_t::get_issues_watchable() {
    assert_thread();
    return issues_watchable.get_watchable();
}

void local_issue_aggregator_t::add_tracker(local_issue_tracker_t *tracker) {
    assert_thread();
    auto res = local_issue_trackers.insert(
        std::make_pair(tracker, std::vector<local_issue_t>()));
    guarantee(res.second == 1);
}

void local_issue_aggregator_t::remove_tracker(local_issue_tracker_t *tracker) {
    assert_thread();
    size_t res = local_issue_trackers.erase(tracker);
    guarantee(res == 1);
}

void local_issue_aggregator_t::update_tracker_issues(
        local_issue_tracker_t *tracker,
        const std::vector<local_issue_t> &issues) {
    assert_thread();
    auto tracker_it = local_issue_trackers.find(tracker);
    guarantee(tracker_it != local_issue_trackers.end());
    tracker_it->second = issues;
    recompute();
}

void local_issue_aggregator_t::recompute() {
    assert_thread();
    std::vector<local_issue_t> updated_issues;
    for (auto const &tracker_it : local_issue_trackers) {
        std::copy(tracker_it.second.begin(),
                  tracker_it.second.end(), 
                  std::back_inserter(updated_issues));
    }
    issues_watchable.set_value(updated_issues);
}
