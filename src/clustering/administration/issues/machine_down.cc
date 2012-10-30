// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/machine_id_to_peer_id.hpp"

machine_down_issue_tracker_t::machine_down_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &_machine_id_translation_table) :
    semilattice_view(_semilattice_view),
    machine_id_translation_table(_machine_id_translation_table) { }

machine_down_issue_tracker_t::~machine_down_issue_tracker_t() { }

std::list<clone_ptr_t<global_issue_t> > machine_down_issue_tracker_t::get_issues() {
    std::list<clone_ptr_t<global_issue_t> > issues;
    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    for (std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it = metadata.machines.machines.begin();
            it != metadata.machines.machines.end(); it++) {
        peer_id_t peer = machine_id_to_peer_id(it->first, machine_id_translation_table->get());
        if (!it->second.is_deleted() && peer.is_nil()) {
            issues.push_back(clone_ptr_t<global_issue_t>(new machine_down_issue_t(it->first)));
        } else if (it->second.is_deleted() && !peer.is_nil()) {
            issues.push_back(clone_ptr_t<global_issue_t>(new machine_ghost_issue_t(it->first)));
        }
    }
    return issues;
}
