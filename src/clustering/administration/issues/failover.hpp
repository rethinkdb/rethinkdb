// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_FAILOVER_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_FAILOVER_HPP_

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"

template <class metadata_t> class semilattice_read_view_t;

class failover_issue_t : public global_issue_t {
public:
    explicit failover_issue_t(uuid_u _namespace_id);

    std::string get_description() const;
    cJSON *get_json_description();

    failover_issue_t *clone() const;
private:
    uuid_u namespace_id;
};

class failover_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit failover_issue_tracker_t(boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _cluster_view);

    ~failover_issue_tracker_t();

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > cluster_view;
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_FAILOVER_HPP_ */
