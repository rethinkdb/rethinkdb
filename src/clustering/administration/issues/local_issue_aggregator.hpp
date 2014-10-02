// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_

#include <vector>

#include "clustering/administration/issues/local.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

// All local issue types are declared here to avoid circular dependencies
class log_write_issue_t : public local_issue_t {
public:
    log_write_issue_t();
    explicit log_write_issue_t(const std::string &_message);

    const datum_string_t &get_name() const { return log_write_issue_type; }
    bool is_critical() const { return false; }

    std::string message;
private:
    ql::datum_t build_info(const metadata_t &metadata) const;
    datum_string_t build_description(const ql::datum_t &info) const;

    static const datum_string_t log_write_issue_type;
    static const uuid_u base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(log_write_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(log_write_issue_t);

class outdated_index_issue_t : public local_issue_t {
public:
    typedef std::map<namespace_id_t, std::set<std::string> > index_map_t;

    outdated_index_issue_t();
    explicit outdated_index_issue_t(const index_map_t &indexes);
    const datum_string_t &get_name() const { return outdated_index_issue_type; }
    bool is_critical() const { return false; }

    index_map_t indexes;
private:
    ql::datum_t build_info(const metadata_t &metadata) const;
    datum_string_t build_description(const ql::datum_t &info) const;

    static const datum_string_t outdated_index_issue_type;
    static const uuid_u base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(outdated_index_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(outdated_index_issue_t);

class server_down_issue_t : public local_issue_t {
public:
    server_down_issue_t();
    explicit server_down_issue_t(const machine_id_t &_down_server_id);

    const datum_string_t &get_name() const { return server_down_issue_type; }
    bool is_critical() const { return true; }

    machine_id_t down_server_id;
private:
    ql::datum_t build_info(const metadata_t &metadata) const;
    datum_string_t build_description(const ql::datum_t &info) const;

    static const datum_string_t server_down_issue_type;
    static const issue_id_t base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(server_down_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_down_issue_t);

class server_ghost_issue_t : public local_issue_t {
public:
    server_ghost_issue_t();
    explicit server_ghost_issue_t(const machine_id_t &_ghost_server_id);

    const datum_string_t &get_name() const { return server_ghost_issue_type; }
    bool is_critical() const { return false; }

    machine_id_t ghost_server_id;
private:
    ql::datum_t build_info(const metadata_t &metadata) const;
    datum_string_t build_description(const ql::datum_t &info) const;

    static const datum_string_t server_ghost_issue_type;
    static const issue_id_t base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(server_ghost_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_ghost_issue_t);

// Every local issue type should have the following:
//  - an entry in the local_issues_t
//  - a local_issue_t::make_*_issue function for the local issue type
struct local_issues_t {
    std::vector<log_write_issue_t> log_write_issues;
    std::vector<server_down_issue_t> server_down_issues;
    std::vector<server_ghost_issue_t> server_ghost_issues;
    std::vector<outdated_index_issue_t> outdated_index_issues;
};

RDB_DECLARE_SERIALIZABLE(local_issues_t);
RDB_DECLARE_EQUALITY_COMPARABLE(local_issues_t);

class local_issue_aggregator_t : public home_thread_mixin_t {
public:
    local_issue_aggregator_t();

    clone_ptr_t<watchable_t<local_issues_t> > get_issues_watchable();

private:
    friend class local_issue_tracker_t;

    watchable_variable_t<local_issues_t> issues_watchable;

    DISABLE_COPYING(local_issue_aggregator_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_ */
