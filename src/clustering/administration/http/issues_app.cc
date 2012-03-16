#include "clustering/administration/http/issues_app.hpp"
#include "http/json.hpp"

issues_http_app_t::issues_http_app_t(global_issue_tracker_t *_issue_tracker) :
    issue_tracker(_issue_tracker) { }

http_res_t issues_http_app_t::handle(const http_req_t &req) {

    if (req.resource != "") {
        return http_res_t(404);
    }

    std::list<clone_ptr_t<global_issue_t> > issues = issue_tracker->get_issues();
    scoped_cJSON_t json(cJSON_CreateArray());
    for (std::list<clone_ptr_t<global_issue_t> >::iterator it = issues.begin(); it != issues.end(); it++) {
        cJSON_AddItemToArray(json.get(), cJSON_CreateString((*it)->get_description().c_str()));
    }

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(json.get()));
    return res;
}
