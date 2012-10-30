// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_LAST_SEEN_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_LAST_SEEN_APP_HPP_

#include "clustering/administration/last_seen_tracker.hpp"
#include "http/http.hpp"
#include "http/json/cJSON.hpp"

class last_seen_http_app_t : public http_json_app_t {
public:
    explicit last_seen_http_app_t(last_seen_tracker_t *last_seen_tracker);
    http_res_t handle(const http_req_t &);
    void get_root(scoped_cJSON_t *json_out);

private:
    last_seen_tracker_t *last_seen_tracker;

    DISABLE_COPYING(last_seen_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_LAST_SEEN_APP_HPP_ */
