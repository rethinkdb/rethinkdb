#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TO_GLOBAL_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TO_GLOBAL_HPP__

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local.hpp"
#include "rpc/directory/read_view.hpp"

class remote_issue_t : public global_issue_t {
public:
    remote_issue_t(const clone_ptr_t<local_issue_t> &_underlying, machine_id_t _source) :
        underlying(_underlying), source(_source) { }

    std::string get_description() const {
        return "On machine " + uuid_to_str(source) + ": " + underlying.get_description();
    }

    remote_issue_t *clone() const {
        return new remote_issue_t(underlying, source);
    }

    clone_ptr_t<local_issue_t> underlying;
    machine_id_t source;
};

class remote_issue_collector_t : public global_issue_tracker_t {
public:
    remote_issue_collector_t(
            const clone_ptr_t<directory_rview_t<std::list<clone_ptr_t<local_issue_t> > > > &_issues_view, 
            const clone_ptr_t<directory_rview_t<machine_id_t> > &_machine_id_translation_table) :
        issues_view(_issues_view), machine_id_translation_table(_machine_id_translation_table) { }

    std::list<clone_ptr_t<global_issue_t> > get_issues() {
        std::list<clone_ptr_t<global_issue_t> > l;
        connectivity_service_t::peers_list_freeze_t freeze(issues_view->get_directory_service()->get_connectivity_service());
        std::set<peer_id_t> peers = issues_view->get_directory_service()->get_connectivity_service()->get_peers_list();
        for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
            machine_id_t machine_id = machine_id_translation_table->get_value(*it).get();
            std::list<clone_ptr_t<local_issue_t> > issues = issues_view.get_value(*it).get();
            for (std::list<clone_ptr_t<local_issue_t> >::iterator jt = issues.begin(); jt != issues.end(); jt++) {
                l.push_back(clone_ptr_t<global_issue_t>(new remote_issue_t(*jt, machine_id)));
            }
        }
    }

private:
    clone_ptr_t<directory_rview_t<std::list<clone_ptr_t<local_issue_t> > > > issues_view;
    clone_ptr_t<directory_rview_t<machine_id_t> > machine_id_translation_table;
};

#endif /* __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TO_GLOBAL_HPP__ */
