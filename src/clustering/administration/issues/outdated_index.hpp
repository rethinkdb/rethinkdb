// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/issues/issue.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/pump_coro.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/store.hpp"

template <class> class semilattice_read_view_t;

class outdated_index_report_impl_t;

class outdated_index_issue_t : public issue_t {
public:
    typedef std::map<namespace_id_t, std::set<std::string> > index_map_t;

    outdated_index_issue_t();
    explicit outdated_index_issue_t(const index_map_t &indexes);
    const datum_string_t &get_name() const { return outdated_index_issue_type; }
    bool is_critical() const { return false; }
    void add_server(const server_id_t &server) {
        reporting_server_ids.insert(server);
    }

    index_map_t indexes;
    std::set<server_id_t> reporting_server_ids;

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

class outdated_index_issue_tracker_t :
    public home_thread_mixin_t {
public:
    outdated_index_issue_tracker_t() { }

    scoped_ptr_t<outdated_index_report_t> create_report(const namespace_id_t &ns_id);

    std::vector<outdated_index_issue_t> get_issues();

    static void combine(std::vector<outdated_index_issue_t> &&issues,
                        std::vector<scoped_ptr_t<issue_t> > *issues_out);

private:
    friend class outdated_index_report_impl_t;
    void remove_report(outdated_index_report_impl_t *report);

    void log_outdated_indexes(namespace_id_t ns_id,
                              std::set<std::string> indexes,
                              auto_drainer_t::lock_t keepalive);

    std::set<namespace_id_t> logged_namespaces;
    one_per_thread_t<outdated_index_issue_t::index_map_t> outdated_indexes;
    one_per_thread_t<std::set<outdated_index_report_t *> > index_reports;

    DISABLE_COPYING(outdated_index_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_ */
