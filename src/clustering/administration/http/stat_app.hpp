#ifndef CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_

#include "http/http.hpp"

class stat_http_app_t : public http_app_t {
public:
    stat_http_app_t();
    http_res_t handle(const http_req_t &req);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_ */

