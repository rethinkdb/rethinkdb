// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_ISSUES_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_ISSUES_APP_HPP_

#include "clustering/administration/issues/global.hpp"
#include "http/http.hpp"

class issues_http_app_t : public http_json_app_t {
public:
    explicit issues_http_app_t(global_issue_tracker_t *_issue_tracker);
    http_res_t handle(const http_req_t &req);
    void get_root(scoped_cJSON_t *json_out);

private:
    global_issue_tracker_t *issue_tracker;

    DISABLE_COPYING(issues_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_ISSUES_APP_HPP_ */
