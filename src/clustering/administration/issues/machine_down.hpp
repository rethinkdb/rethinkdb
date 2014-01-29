// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_MACHINE_DOWN_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_MACHINE_DOWN_HPP_

#include <list>
#include <map>
#include <string>

#include "clustering/administration/issues/json.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"

template <class metadata_t> class semilattice_read_view_t;

class machine_down_issue_t : public global_issue_t {
public:
    explicit machine_down_issue_t(const machine_id_t &mid) : machine_id(mid) { }

    std::string get_description() const;
    cJSON *get_json_description();
    machine_down_issue_t *clone() const;

    machine_id_t machine_id;

private:
    DISABLE_COPYING(machine_down_issue_t);
};

class machine_ghost_issue_t : public global_issue_t {
public:
    explicit machine_ghost_issue_t(const machine_id_t &mid) : machine_id(mid) { }

    std::string get_description() const;

    cJSON *get_json_description();

    machine_ghost_issue_t *clone() const;

    machine_id_t machine_id;

private:
    DISABLE_COPYING(machine_ghost_issue_t);
};

class machine_down_issue_tracker_t : public global_issue_tracker_t {
public:
    machine_down_issue_tracker_t(
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view,
            const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > &machine_id_translation_table);
    ~machine_down_issue_tracker_t();

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > machine_id_translation_table;

    DISABLE_COPYING(machine_down_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_MACHINE_DOWN_HPP_ */
