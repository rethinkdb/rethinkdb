// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_

#include <vector>

#include "clustering/administration/issues/local.hpp"
#include "concurrency/watchable.hpp"
#include "concurrency/watchable_transform.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

// All local issue types are declared here to avoid circular dependencies
// This is because local issue trackers require definitions for the metadata,
// but the metadata requires definitions of all local issue types.
//
// Every local issue type should have the following:
//  - an issue_t subclass defined in this file
//  - an entry in the local_issues_t for that issue_t subclass
//  - handling in remote_issue_tracker_t
//  - a combine() method in the issue tracker
class log_write_issue_t : public local_issue_t {
public:
    log_write_issue_t();
    explicit log_write_issue_t(const std::string &_message);

    const datum_string_t &get_name() const { return log_write_issue_type; }
    bool is_critical() const { return false; }

    std::string message;
private:
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;

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
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;

    static const datum_string_t outdated_index_issue_type;
    static const uuid_u base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(outdated_index_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(outdated_index_issue_t);

class local_issues_t {
public:
    std::vector<log_write_issue_t> log_write_issues;
    std::vector<outdated_index_issue_t> outdated_index_issues;
};

RDB_DECLARE_SERIALIZABLE(local_issues_t);
RDB_DECLARE_EQUALITY_COMPARABLE(local_issues_t);

class local_issue_aggregator_t : public home_thread_mixin_t {
public:
    local_issue_aggregator_t();

    clone_ptr_t<watchable_t<local_issues_t> > get_issues_watchable();

    template <typename local_t>
    class subscription_t {
    public:
        subscription_t(local_issue_aggregator_t *parent,
                       const clone_ptr_t<watchable_t<std::vector<local_t> > > &source,
                       std::vector<local_t> local_issues_t::*field) :
            copier(field, source, &parent->issues_watchable) { }
    private:
        watchable_field_copier_t<std::vector<local_t>, local_issues_t> copier;
    };

private:
    watchable_variable_t<local_issues_t> issues_watchable;
    DISABLE_COPYING(local_issue_aggregator_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_ */
