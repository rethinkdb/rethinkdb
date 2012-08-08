#ifndef CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_
#define CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_

#include <string>

#include "clustering/administration/issues/json.hpp"

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

//json adapter concept for issue_json_t
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(issue_json_t *target, UNUSED const ctx_t &ctx) {
    return get_json_subfields(target);
}

template <class ctx_t>
cJSON *render_as_json(issue_json_t *target, UNUSED const ctx_t &ctx) {
    return render_as_json(target);
}

template <class ctx_t>
void apply_json_to(cJSON *change, issue_json_t *target, UNUSED const ctx_t &ctx) {
    apply_json_to(change, target);
}

template <class ctx_t>
void on_subfield_change(issue_json_t *target, const ctx_t &) {
    on_subfield_change(target);
}


// ctx-less json adapter concept for issue_json_t
// TODO: deinline these?
inline json_adapter_if_t::json_adapter_map_t get_json_subfields(issue_json_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["critical"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<bool>(&target->critical));
    res["description"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string>(&target->description));
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string>(&target->type));
    res["time"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<ticks_t>(&target->time));

    return res;
}

inline cJSON *render_as_json(issue_json_t *target) {
    return render_as_directory(target);
}

inline void apply_json_to(cJSON *change, issue_json_t *target) {
    apply_as_directory(change, target);
}

inline void on_subfield_change(issue_json_t *) { }




#endif  // CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_
