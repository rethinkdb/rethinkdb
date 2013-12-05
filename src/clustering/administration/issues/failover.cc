#include "clustering/administration/issues/failover.hpp"

failover_issue_t::failover_issue_t(uuid_u _namespace_id)
    : namespace_id(_namespace_id) { }

std::string failover_issue_t::get_description() const {
    return "The namespace: " + uuid_to_str(namespace_id) +
        " is in failover mode due the the primary being unreachable.";
}

cJSON *failover_issue_t::get_json_description() {
        issue_json_t json;
        json.critical = false;
        json.time = get_secs();
        json.description = get_description();
        json.type = "FAILOVER";

        cJSON *res = render_as_json(&json);
        cJSON_AddItemToObject(res, "namespace_id", render_as_json(&namespace_id));

        return res;
}

failover_issue_t *failover_issue_t::clone() const {
    return new failover_issue_t(namespace_id);
}

failover_issue_tracker_t::failover_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _cluster_view) 
    : cluster_view(_cluster_view) { }

failover_issue_tracker_t::~failover_issue_tracker_t() { }

template <class protocol_t>
void get_ns_issues(
        const namespaces_semilattice_metadata_t<protocol_t> &metadata,
        std::list<clone_ptr_t<global_issue_t> > *list_to_append_to) {
    for (auto it = metadata.namespaces.begin(); it != metadata.namespaces.end(); ++it) {
        if (it->second.is_deleted() || it->second.get_ref().blueprint.in_conflict()) {
            continue;
        }

        if (!it->second.get_ref().blueprint.get_ref().failover.empty()) {
            list_to_append_to->push_back(
                clone_ptr_t<global_issue_t>(new failover_issue_t(it->first)));
        }
    }
}

std::list<clone_ptr_t<global_issue_t> > failover_issue_tracker_t::get_issues() {
    std::list<clone_ptr_t<global_issue_t> > res;
    cluster_semilattice_metadata_t metadata = cluster_view->get();
    get_ns_issues(*metadata.dummy_namespaces, &res);
    get_ns_issues(*metadata.memcached_namespaces, &res);
    get_ns_issues(*metadata.rdb_namespaces, &res);
    return res;
}
