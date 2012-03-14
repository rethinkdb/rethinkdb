#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_MACHINE_DOWN_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_MACHINE_DOWN_HPP__

#include "clustering/administration/issues/global.hpp"

class machine_down_issue_t : public global_issue_t {
public:
    machine_down_issue_t(const machine_id_t &mid,) : machine_id(mid) { }

    std::string get_description() const {
        return "Machine " + uuid_to_str(machine_id) + " is inaccessible.";
    }

    machine_down_issue_t *clone() const {
        return machine_down_issue_t(machine_id);
    }

    machine_id_t machine_id;
};

class machine_down_issue_tracker_t : public global_issue_tracker_t {
public:
    machine_down_issue_tracker_t(
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view,
            const clone_ptr_t<directory_rview_t<machine_id_t> > &machine_id_translation_table);

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;
    clone_ptr_t<directory_rview_t<machine_id_t> > machine_id_translation_table;
};

#endif /* __CLUSTERING_ADMINISTRATION_ISSUES_MACHINE_DOWN_HPP__ */
