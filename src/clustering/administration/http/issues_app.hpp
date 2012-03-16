#ifndef CLUSTERING_ADMINISTRATION_HTTP_ISSUES_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_ISSUES_APP_HPP_

#include "clustering/administration/issues/global.hpp"
#include "http/http.hpp"

class issues_http_app_t : public http_app_t {
public:
    issues_http_app_t(global_issue_tracker_t *_issue_tracker);

    http_res_t handle(const http_req_t &req);

private:
    global_issue_tracker_t *issue_tracker;
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_ISSUES_APP_HPP_ */
