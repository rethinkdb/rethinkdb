// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_NON_TRANSITIVE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_NON_TRANSITIVE_HPP_

#include "clustering/administration/issues/issue.hpp"

class non_transitive_issue_t : public issue_t {
public:
    non_transitive_issue_t() : issue_t(nil_uuid()) { }
    non_transitive_issue_t(const std::string &_message) :
        issue_t(from_hash(base_type_id, _message)),
        message(_message) { }
    bool build_info_and_description(
        const metadata_t &,
        server_config_client_t *config_client,
        table_meta_client_t *,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const final;

    bool is_critical() const final { return false; }

    void add_server(const server_id_t &server) {
        reporting_server_ids.insert(server);
    }

    const datum_string_t &get_name() const final {
        return non_transitive_issue_type;
    }

    std::string message;
    static const datum_string_t non_transitive_issue_type;
    static const uuid_u base_type_id;;
    std::set<server_id_t> reporting_server_ids;
};

class non_transitive_issue_tracker_t : public issue_tracker_t {
public:
    non_transitive_issue_tracker_t(
        server_config_client_t *server_config_client);
    ~non_transitive_issue_tracker_t();

    std::vector<scoped_ptr_t<issue_t> > get_issues(signal_t *interruptor) const;

private:
    server_config_client_t *server_config_client;

    DISABLE_COPYING(non_transitive_issue_tracker_t);
};

#endif // CLUSTERING_ADMINISTRATION_ISSUES_NON_TRANSITIVE_HPP_
