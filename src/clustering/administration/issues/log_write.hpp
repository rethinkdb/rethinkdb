// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_

#include <string>

#include "clustering/administration/issues/local_issue_aggregator.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/store.hpp"

class log_write_issue_tracker_t :
    public home_thread_mixin_t {
public:
    explicit log_write_issue_tracker_t(local_issue_aggregator_t *parent);
    ~log_write_issue_tracker_t();

    void report_success();
    void report_error(const std::string &message);

    static void combine(local_issues_t *issues,
                        std::vector<scoped_ptr_t<issue_t> > *issues_out);

private:
    void do_update();

    boost::optional<std::string> error_message;

    watchable_variable_t<std::vector<log_write_issue_t> > issues;
    local_issue_aggregator_t::subscription_t<log_write_issue_t> subs;
    DISABLE_COPYING(log_write_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOG_WRITE_HPP_ */
