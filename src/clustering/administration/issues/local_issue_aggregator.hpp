// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_

#include <vector>

#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "concurrency/watchable.hpp"
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

/* RSI(raft): Remove this completely */
#if 0
class server_disconnected_issue_t : public local_issue_t {
public:
    server_disconnected_issue_t();
    explicit server_disconnected_issue_t(const server_id_t &_disconnected_server_id);

    const datum_string_t &get_name() const { return server_disconnected_issue_type; }
    bool is_critical() const { return true; }

    server_id_t disconnected_server_id;
private:
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;

    static const datum_string_t server_disconnected_issue_type;
    static const issue_id_t base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(server_disconnected_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_disconnected_issue_t);

class server_ghost_issue_t : public local_issue_t {
public:
    server_ghost_issue_t();
    server_ghost_issue_t(const server_id_t &_ghost_server_id,
                         const std::string &_hostname,
                         int64_t _pid);

    const datum_string_t &get_name() const { return server_ghost_issue_type; }
    bool is_critical() const { return false; }

    server_id_t ghost_server_id;
    std::string hostname;
    int64_t pid;

private:
    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;

    static const datum_string_t server_ghost_issue_type;
    static const issue_id_t base_issue_id;
};

RDB_DECLARE_SERIALIZABLE(server_ghost_issue_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_ghost_issue_t);
#endif

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
        field_copier_t<std::vector<local_t>, local_issues_t> copier;
    };

private:
    watchable_variable_t<local_issues_t> issues_watchable;
    DISABLE_COPYING(local_issue_aggregator_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_ISSUE_AGGREGATOR_HPP_ */
