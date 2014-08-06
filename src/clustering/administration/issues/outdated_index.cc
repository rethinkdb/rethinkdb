#include "clustering/administration/issues/outdated_index.hpp"

typedef std::map<namespace_id_t, std::set<std::string> > index_set_map_t;

outdated_index_issue_t::outdated_index_issue_t(index_set_map_t &&indexes) :
    outdated_indexes(indexes) { }

std::string outdated_index_issue_t::get_description() const {
    return "";
}

cJSON *outdated_index_issue_t::get_json_description() {
    issue_json_t json;
    json.critical = false;
    json.description = "The cluster contains indexes that were created with a "
        "previous version of the query language which contained some bugs.  These "
        "should be remade to avoid relying on broken behavior.  See <url> for "
        "details.";
    json.type = "OUTDATED_INDEX_ISSUE";
    json.time = get_secs();

    // Put all of the outdated indexes in a map by their uuid, like so:
    // indexes: { '1234-5678': ['sindex_a', 'sindex_b'], '9284-af82': ['sindex_c'] }
    cJSON *res = render_as_json(&json);
    cJSON *obj = cJSON_CreateObject();
    for (auto it = outdated_indexes.begin(); it != outdated_indexes.end(); ++it) {
        cJSON *arr = cJSON_CreateArray();
        for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
            cJSON_AddItemToArray(arr, cJSON_CreateString(jt->c_str()));
        }
        cJSON_AddItemToObject(obj, uuid_to_str(it->first).c_str(), arr);
    }
    cJSON_AddItemToObject(res, "indexes", obj);

    return res;
}

outdated_index_issue_t *outdated_index_issue_t::clone() const {
    return new outdated_index_issue_t(index_set_map_t(outdated_indexes));
}

outdated_index_issue_tracker_t::outdated_index_issue_tracker_t() { }

outdated_index_issue_tracker_t::~outdated_index_issue_tracker_t() { }

std::set<std::string> *
outdated_index_issue_tracker_t::get_index_set(const namespace_id_t &ns_id) { 
    auto thread_map = outdated_indexes.get();
    auto ns_it = thread_map->find(ns_id);

    if (ns_it == thread_map->end()) {
        ns_it = thread_map->insert(std::make_pair(ns_id, std::set<std::string>())).first;
        guarantee(ns_it != thread_map->end());
    }

    return &ns_it->second;
}

void copy_thread_map(
        one_per_thread_t<index_set_map_t> *maps_in,
        std::vector<index_set_map_t> *maps_out, int thread_id) {
    on_thread_t thread_switcher((threadnum_t(thread_id)));
    (*maps_out)[thread_id] = *maps_in->get();
}

index_set_map_t merge_maps(
        const std::vector<index_set_map_t> &maps) {
    index_set_map_t res;
    for (auto it = maps.begin(); it != maps.end(); ++it) {
        for (auto jt = it->begin(); jt != it->end(); ++jt) {
            if (!jt->second.empty()) {
                res[jt->first].insert(jt->second.begin(), jt->second.end());
            }
        }
    }
    return std::move(res);
}

index_set_map_t
outdated_index_issue_tracker_t::collect_and_merge_maps() {
    std::vector<index_set_map_t> all_maps(get_num_threads());
    pmap(get_num_threads(), std::bind(&copy_thread_map,
                                      &outdated_indexes,
                                      &all_maps,
                                      ph::_1));
    
    // TODO: collect outdated indexes from other nodes

    return merge_maps(all_maps);
}

// We only create one issue (at most) to avoid spamming issues
std::list<clone_ptr_t<global_issue_t> > outdated_index_issue_tracker_t::get_issues() {
    std::list<clone_ptr_t<global_issue_t> > issues;
    index_set_map_t all_outdated_indexes = collect_and_merge_maps();
    if (!all_outdated_indexes.empty()) {
        issues.push_back(clone_ptr_t<global_issue_t>(
            new outdated_index_issue_t(std::move(all_outdated_indexes))));
    }
    return std::move(issues);
}
