// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_UNSATISFIABLE_GOALS_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_UNSATISFIABLE_GOALS_HPP_

#include <list>
#include <map>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/json.hpp"
#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"

template <class metadata_t> class semilattice_read_view_t;

class unsatisfiable_goals_issue_t : public global_issue_t {
public:
    unsatisfiable_goals_issue_t(
            const namespace_id_t &namespace_id,
            const datacenter_id_t &primary_datacenter,
            const std::map<datacenter_id_t, int> &replica_affinities,
            const std::map<datacenter_id_t, int> &actual_machines_in_datacenters);

    std::string get_description() const;
    cJSON *get_json_description();
    unsatisfiable_goals_issue_t *clone() const;

    namespace_id_t namespace_id;
    datacenter_id_t primary_datacenter;
    std::map<datacenter_id_t, int> replica_affinities;
    std::map<datacenter_id_t, int> actual_machines_in_datacenters;

private:
    DISABLE_COPYING(unsatisfiable_goals_issue_t);
};

class unsatisfiable_goals_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit unsatisfiable_goals_issue_tracker_t(boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view);
    ~unsatisfiable_goals_issue_tracker_t();

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;

    DISABLE_COPYING(unsatisfiable_goals_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_UNSATISFIABLE_GOALS_HPP_ */
