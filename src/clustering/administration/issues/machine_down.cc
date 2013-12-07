// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/machine_id_to_peer_id.hpp"

std::string machine_down_issue_t::get_description() const {
    return "Machine " + uuid_to_str(machine_id) + " is inaccessible.";
}

cJSON *machine_down_issue_t::get_json_description() {
    issue_json_t json;
    json.critical = true;
    json.description = "Machine " + uuid_to_str(machine_id) + " is inaccessible.";
    json.type = "MACHINE_DOWN";
    json.time = get_secs();

    cJSON *res = render_as_json(&json);
    cJSON_AddItemToObject(res, "victim", render_as_json(&machine_id));

    return res;
}

machine_down_issue_t *machine_down_issue_t::clone() const {
    return new machine_down_issue_t(machine_id);
}

std::string machine_ghost_issue_t::get_description() const {
    return "Machine " + uuid_to_str(machine_id) + " was declared dead, but it's connected to the cluster.";
}


cJSON *machine_ghost_issue_t::get_json_description() {
    issue_json_t json;
    json.critical = false;
    json.description = get_description();
    json.type = "MACHINE_GHOST";
    json.time = get_secs();

    cJSON *res = render_as_json(&json);
    cJSON_AddItemToObject(res, "ghost", render_as_json(&machine_id));

    return res;
}

machine_ghost_issue_t *machine_ghost_issue_t::clone() const {
    return new machine_ghost_issue_t(machine_id);
}



machine_down_issue_tracker_t::machine_down_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > &_machine_id_translation_table) :
    semilattice_view(_semilattice_view),
    machine_id_translation_table(_machine_id_translation_table) { }

machine_down_issue_tracker_t::~machine_down_issue_tracker_t() { }

std::list<clone_ptr_t<global_issue_t> > machine_down_issue_tracker_t::get_issues() {
    std::list<clone_ptr_t<global_issue_t> > issues;
    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    for (std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it = metadata.machines.machines.begin();
            it != metadata.machines.machines.end(); it++) {
        peer_id_t peer = machine_id_to_peer_id(it->first, machine_id_translation_table->get().get_inner());
        if (!it->second.is_deleted() && peer.is_nil()) {
            issues.push_back(clone_ptr_t<global_issue_t>(new machine_down_issue_t(it->first)));
        } else if (it->second.is_deleted() && !peer.is_nil()) {
            issues.push_back(clone_ptr_t<global_issue_t>(new machine_ghost_issue_t(it->first)));
        }
    }
    return issues;
}
