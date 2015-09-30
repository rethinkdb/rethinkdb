// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/issues/issue.hpp"
#include "clustering/table_manager/multi_table_manager.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/pump_coro.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/store.hpp"

class outdated_index_issue_t : public issue_t {
public:
    typedef std::map<namespace_id_t, std::set<std::string> > index_map_t;

    outdated_index_issue_t();
    explicit outdated_index_issue_t(index_map_t indexes);
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

class outdated_index_issue_tracker_t : public issue_tracker_t {
public:
    explicit outdated_index_issue_tracker_t(table_meta_client_t *_table_meta_client);

    std::vector<scoped_ptr_t<issue_t> > get_issues(signal_t *interruptor) const;

    static void log_outdated_indexes(multi_table_manager_t *multi_table_manager,
                                     const cluster_semilattice_metadata_t &metadata,
                                     signal_t *interruptor);

private:
    table_meta_client_t *table_meta_client;
    DISABLE_COPYING(outdated_index_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_ */
