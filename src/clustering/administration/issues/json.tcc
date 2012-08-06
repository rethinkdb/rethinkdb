#ifndef CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_
#define CLUSTERING_ADMINISTRATION_ISSUES_JSON_TCC_

#include <string>

#include "clustering/administration/issues/json.hpp"

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

//json adapter concept for issue_json_t
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(issue_json_t *target, const ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["critical"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<bool, ctx_t>(&target->critical, ctx));
    res["description"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string, ctx_t>(&target->description, ctx));
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string, ctx_t>(&target->type, ctx));
    res["time"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<ticks_t, ctx_t>(&target->time, ctx));

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
// TODO: Is this ever instantiated?  I suddenly got a compiler warning about no return statement when making json_adapter_if_t lose its template argument.
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(local_issue_json_t *target, const ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res = render_as_json(static_cast<issue_json_t *>(target), ctx);
    res["machine"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<bool, ctx_t>(&target->machine, ctx));
    return res;
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
