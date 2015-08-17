// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_

#include <string>

#include "clustering/administration/issues/issue.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/store.hpp"

class local_issue_server_t;

class log_write_issue_t : public issue_t {
public:
    log_write_issue_t();
    explicit log_write_issue_t(const std::string &_message);

    const datum_string_t &get_name() const { return log_write_issue_type; }
    bool is_critical() const { return false; }

    void add_server(const server_id_t &server) {
        reporting_server_ids.insert(server);
    }

    std::string message;
    std::set<server_id_t> reporting_server_ids;

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

class log_write_issue_tracker_t :
    public home_thread_mixin_t {
public:
    log_write_issue_tracker_t() { }

    std::vector<log_write_issue_t> get_issues();

    void report_success();
    void report_error(const std::string &message);

    static void combine(std::vector<log_write_issue_t> &&issues,
                        std::vector<scoped_ptr_t<issue_t> > *issues_out);

private:
    void do_update();

    boost::optional<std::string> error_message;

    DISABLE_COPYING(log_write_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_ */
