// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_

#include <list>
#include <map>
#include <set>
#include <string>

#include "containers/incremental_lenses.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/store.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local.hpp"

class log_write_issue_t : public issue_t {
public:
    explicit log_write_issue_t(const std::string &_message,
                               const std::vector<machine_id_t> &_affected_servers);
    const datum_string_t &get_name() const { return log_write_issue_type; }
    bool is_critical() const { return false; }

private:
    void build_info_and_description(const metadata_t &metadata,
                                    ql::datum_t *info_out,
                                    datum_string_t *desc_out) const;

    static const datum_string_t log_write_issue_type;
    static const uuid_u base_issue_id;
    const std::string message;
    const std::vector<machine_id_t> affected_servers;
};

class outdated_index_report_impl_t;

class log_write_issue_tracker_t :
    public local_issue_tracker_t,
    public home_thread_mixin_t {
public:
    explicit log_write_issue_tracker_t(local_issue_aggregator_t *_parent);
    ~log_write_issue_tracker_t();

    class entry_t {
    public:
        explicit entry_t(log_write_issue_tracker_t *_parent);
        ~entry_t();

        void set_message(const std::string &_message);

    private:
        log_write_issue_tracker_t *parent;
        std::string message;
    };

private:
    entry_t *active_entry;
    DISABLE_COPYING(log_write_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_ */
