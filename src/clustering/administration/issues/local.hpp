// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_

#include <string>
#include <vector>

#include "clustering/administration/issues/issue.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

class local_issue_aggregator_t;
class local_issues_t;

class local_issue_t : public issue_t {
public:
    local_issue_t();
    explicit local_issue_t(const issue_id_t &id);
    virtual ~local_issue_t();

    void add_server(const server_id_t &server);

    std::vector<server_id_t> affected_server_ids;
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_ */
