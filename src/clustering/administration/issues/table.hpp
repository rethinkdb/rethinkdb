// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_TABLE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_TABLE_HPP_

#include <vector>

#include "clustering/administration/issues/issue.hpp"
#include "containers/scoped.hpp"

class server_config_client_t;
class table_meta_client_t;
class table_basic_config_t;

class table_issue_tracker_t : public issue_tracker_t {
public:
    table_issue_tracker_t(server_config_client_t *_server_config_client,
                          table_meta_client_t *_table_meta_client);
    ~table_issue_tracker_t();

    std::vector<scoped_ptr_t<issue_t> > get_issues(signal_t *interruptor) const;

private:
    void check_table(const namespace_id_t &table_id,
                     std::vector<scoped_ptr_t<issue_t> > *issues_out,
                     signal_t *interruptor) const;

    server_config_client_t *server_config_client;
    table_meta_client_t *table_meta_client;

    DISABLE_COPYING(table_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_TABLE_HPP_ */
