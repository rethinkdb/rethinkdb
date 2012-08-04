#include "clustering/administration/http/issues_app.hpp"
#include "http/json.hpp"

issues_http_app_t::issues_http_app_t(global_issue_tracker_t *_issue_tracker) :
    issue_tracker(_issue_tracker) { }

void issues_http_app_t::get_root(scoped_cJSON_t *json_out) {
    std::list<clone_ptr_t<global_issue_t> > issues = issue_tracker->get_issues();
    json_out->reset(cJSON_CreateArray());
    for (std::list<clone_ptr_t<global_issue_t> >::iterator it = issues.begin(); it != issues.end(); ++it) {
        json_out->AddItemToArray((*it)->get_json_description());
    }
}

http_res_t issues_http_app_t::handle(const http_req_t &req) {
    if (req.method != GET) {
        return http_res_t(HTTP_METHOD_NOT_ALLOWED);
    }

    std::string resource = req.resource.as_string();
    if (resource != "/" && resource != "") {
        return http_res_t(HTTP_NOT_FOUND);
    }

    scoped_cJSON_t json(NULL);
    get_root(&json);

    return http_json_res(json.get());
}
