// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "time.hpp"
#include "clustering/administration/issues/issue.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

// Every local issue should have the following:
//  - an entry in the local_issue_t::issue_data variant
//  - a local_issue_t::make_*_issue function for the local issue type
//  - an implementation for converting to a global issue in local_issue_t::to_issue

struct local_issue_t {
    // outdated_index_map_t is declared here to avoid a circular dependency
    typedef std::map<namespace_id_t, std::set<std::string> > outdated_index_map_t;

    struct server_down_data_t { machine_id_t server_id; };
    struct server_ghost_data_t { machine_id_t server_id; };
    struct outdated_index_data_t { outdated_index_map_t indexes; };
    struct log_write_data_t { std::string message; };

    static local_issue_t make_server_down_issue(const machine_id_t &server_id);
    static local_issue_t make_server_ghost_issue(const machine_id_t &server_id);
    static local_issue_t make_outdated_index_issue(const outdated_index_map_t &indexes);
    static local_issue_t make_log_write_issue(const std::string &message);

    local_issue_t();
    ~local_issue_t();

    void to_issue(const std::vector<machine_id_t> &affected_servers,
                  scoped_ptr_t<issue_t> *issue_out) const;

    microtime_t issue_time;
    boost::variant<server_down_data_t,
                   server_ghost_data_t,
                   outdated_index_data_t,
                   log_write_data_t> issue_data;
};

RDB_DECLARE_SERIALIZABLE(local_issue_t);
RDB_DECLARE_SERIALIZABLE(local_issue_t::server_down_data_t);
RDB_DECLARE_SERIALIZABLE(local_issue_t::server_ghost_data_t);
RDB_DECLARE_SERIALIZABLE(local_issue_t::outdated_index_data_t);
RDB_DECLARE_SERIALIZABLE(local_issue_t::log_write_data_t);

RDB_DECLARE_EQUALITY_COMPARABLE(local_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(local_issue_t::server_down_data_t);
RDB_DECLARE_EQUALITY_COMPARABLE(local_issue_t::server_ghost_data_t);
RDB_DECLARE_EQUALITY_COMPARABLE(local_issue_t::outdated_index_data_t);
RDB_DECLARE_EQUALITY_COMPARABLE(local_issue_t::log_write_data_t);

class local_issue_aggregator_t;

class local_issue_tracker_t {
public:
    explicit local_issue_tracker_t(local_issue_aggregator_t *_parent);
    ~local_issue_tracker_t();

protected:
    void update(const std::vector<local_issue_t> &issues);

private:
    local_issue_aggregator_t *parent;
};

class local_issue_aggregator_t : public home_thread_mixin_t {
public:
    local_issue_aggregator_t();

    clone_ptr_t<watchable_t<std::vector<local_issue_t> > > get_issues_watchable();

private:
    friend class local_issue_tracker_t;
    void add_tracker(local_issue_tracker_t *tracker);
    void remove_tracker(local_issue_tracker_t *tracker);
    void update_tracker_issues(local_issue_tracker_t *tracker,
                               const std::vector<local_issue_t> &issues);
    void recompute();

    std::map<local_issue_tracker_t *, std::vector<local_issue_t> > local_issue_trackers;
    watchable_variable_t<std::vector<local_issue_t> > issues_watchable;

    DISABLE_COPYING(local_issue_aggregator_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_ */
