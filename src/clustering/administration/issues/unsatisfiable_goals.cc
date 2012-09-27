#include "clustering/administration/issues/unsatisfiable_goals.hpp"

unsatisfiable_goals_issue_t::unsatisfiable_goals_issue_t(
        const namespace_id_t &ni,
        const datacenter_id_t &pd,
        const std::map<datacenter_id_t, int> &ra,
        const std::map<datacenter_id_t, int> &am) :
    namespace_id(ni), primary_datacenter(pd), replica_affinities(ra), actual_machines_in_datacenters(am) { }

std::string unsatisfiable_goals_issue_t::get_description() const {
    return "The settings you have given are impossible.";
}

cJSON *unsatisfiable_goals_issue_t::get_json_description() {
    issue_json_t json;
    json.critical = true;
    json.description = get_description();
    json.type = "UNSATISFIABLE_GOALS";
    json.time = get_secs();

    cJSON *res = render_as_json(&json);

    cJSON_AddItemToObject(res, "namespace_id", render_as_json(&namespace_id));
    cJSON_AddItemToObject(res, "primary_datacenter", render_as_json(&primary_datacenter));
    cJSON_AddItemToObject(res, "replica_affinities", render_as_json(&replica_affinities));
    cJSON_AddItemToObject(res, "actual_machines_in_datacenters", render_as_json(&actual_machines_in_datacenters));

    return res;
}

unsatisfiable_goals_issue_t *unsatisfiable_goals_issue_t::clone() const {
    return new unsatisfiable_goals_issue_t(namespace_id, primary_datacenter, replica_affinities, actual_machines_in_datacenters);
}

static bool is_satisfiable(
        const datacenter_id_t &primary_datacenter,
        const std::map<datacenter_id_t, int> &replica_affinities,
        const std::map<datacenter_id_t, int> &actual_machines_in_datacenters) {
    int extra_machines = 0;
    for (std::map<datacenter_id_t, int>::const_iterator it = replica_affinities.begin(); it != replica_affinities.end(); ++it) {
        int need = it->second;
        if (it->first == primary_datacenter) {
            need++;
        }
        std::map<datacenter_id_t, int>::const_iterator jt = actual_machines_in_datacenters.find(it->first);
        if (jt == actual_machines_in_datacenters.end() || jt->second < need) {
            debugf("Not enough secondaries for %s\n", uuid_to_str(it->first).c_str());
            return false;
        } else {
            extra_machines += need - jt->second;
        }
    }

    for (std::map<datacenter_id_t, int>::const_iterator it = actual_machines_in_datacenters.begin(); it != actual_machines_in_datacenters.end(); ++it) {
        if (!std_contains(replica_affinities, it->first)) {
            extra_machines += it->second;
        }
    }

    if (primary_datacenter.is_nil()) {
        if (extra_machines == 0) {
            debugf("No extra machines\n");
            return false;
        }
    } else if (replica_affinities.find(primary_datacenter) == replica_affinities.end()) {
        std::map<datacenter_id_t, int>::const_iterator jt = actual_machines_in_datacenters.find(primary_datacenter);
        if (jt == actual_machines_in_datacenters.end() || jt->second < 1) {
            debugf("No space in primary datacenter\n");
            return false;
        }
    }
    return true;
}

template<class protocol_t>
static void make_issues(const cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > &namespaces,
        const std::map<datacenter_id_t, int> &actual_machines_in_datacenters,
        std::list<clone_ptr_t<global_issue_t> > *issues_out) {
    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it = namespaces->namespaces.begin();
            it != namespaces->namespaces.end(); it++) {
        if (it->second.is_deleted()) {
            continue;
        }
        namespace_semilattice_metadata_t<protocol_t> ns = it->second.get();
        if (ns.primary_datacenter.in_conflict() || ns.replica_affinities.in_conflict()) {
            continue;
        }
        if (!is_satisfiable(ns.primary_datacenter.get(), ns.replica_affinities.get(), actual_machines_in_datacenters)) {
            issues_out->push_back(clone_ptr_t<global_issue_t>(
                new unsatisfiable_goals_issue_t(it->first, ns.primary_datacenter.get(), ns.replica_affinities.get(), actual_machines_in_datacenters)));
        }
    }
}

std::list<clone_ptr_t<global_issue_t> > unsatisfiable_goals_issue_tracker_t::get_issues() {
    cluster_semilattice_metadata_t metadata = semilattice_view->get();

    std::map<datacenter_id_t, int> actual_machines_in_datacenters;
    for (machines_semilattice_metadata_t::machine_map_t::iterator it = metadata.machines.machines.begin();
            it != metadata.machines.machines.end(); it++) {
        if (!it->second.is_deleted() && !it->second.get().datacenter.in_conflict()) {
            get_with_default(actual_machines_in_datacenters, it->second.get().datacenter.get(), 0)++;
        }
    }

    std::list<clone_ptr_t<global_issue_t> > issues;
    make_issues(metadata.rdb_namespaces, actual_machines_in_datacenters, &issues);
    make_issues(metadata.dummy_namespaces, actual_machines_in_datacenters, &issues);
    make_issues(metadata.memcached_namespaces, actual_machines_in_datacenters, &issues);
    return issues;
}
