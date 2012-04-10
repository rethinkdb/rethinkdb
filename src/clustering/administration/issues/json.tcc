#ifndef CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_
#define CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

//json adapter concept for issue_type_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(issue_type_t *, const ctx_t &) {
    return typename json_adapter_if_t<ctx_t>::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(issue_type_t *target, const ctx_t &) {
    switch (target->issue_type) {
        case VCLOCK_CONFLICT:
            return cJSON_CreateString("VCLOCK_CONFLICT");
            break;
        case MACHINE_DOWN:
            return cJSON_CreateString("MACHINE_DOWN");
            break;
        case NAME_CONFLICT_ISSUE:
            return cJSON_CreateString("NAME_CONFLICT_ISSUE");
            break;
        case PERSISTENCE_ISSUE:
            return cJSON_CreateString("PERSISTENCE_ISSUE");
            break;
        case PINNINGS_SHARDS_MISMATCH:
            return cJSON_CreateString("PINNINGS_SHARDS_MISMATCH");
            break;
        default:
            unreachable();
            break;
    }
}

template <class ctx_t>
void apply_json_to(cJSON *, issue_type_t *, const ctx_t &) {
    //In theory this could be allowed... I don't see a reason for it or a place
    //it's going to come up. It's unlikely this function should ever be called
    not_implemented();
}

template <class ctx_t>
void on_subfield_change(issue_type_t *, const ctx_t &) { }

//json adapter concept for issue_json_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(issue_json_t *target, const ctx_t &) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res;
    res["critical"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<bool, ctx_t>(&target->critical));
    res["description"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<std::string, ctx_t>(&target->description));
    res["type"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<issue_type_t, ctx_t>(&target->type));
    res["time"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<ticks_t, ctx_t>(&target->time));

    return res;
}

template <class ctx_t>
cJSON *render_as_json(issue_json_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, issue_json_t *target, const ctx_t &ctx) {
    apply_as_directory(change, target, ctx);
}

template <class ctx_t>
void on_subfield_change(issue_json_t *, const ctx_t &) { }

//json adapter concept for local_issue_json_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(local_issue_json_t *target, const ctx_t &ctx) {
    typename json_adapter_if_t<ctx_t>::json_adapter_map_t res = render_as_json(dynamic_cast<issue_json_t *>(target), ctx);
    res["machine"] = boost::shared_ptr<json_adapter_if_t<ctx_t> >(new json_adapter_t<bool, ctx_t>(&target->machine));
}

template <class ctx_t>
cJSON *render_as_json(local_issue_json_t *target, const ctx_t &ctx) {
    return render_as_directory(target, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *, local_issue_json_t *target, const ctx_t &ctx) {
    apply_as_directory(target, ctx);
}

template <class ctx_t>
void on_subfield_change(local_issue_json_t *, const ctx_t &) { }



#endif  // CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_
