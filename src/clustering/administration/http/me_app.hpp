// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_ME_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_ME_APP_HPP_

#include "http/http.hpp"

/* This is an `http_app_t` that returns the server ID of the server processing the
request. */
class me_http_app_t : public http_app_t {
public:
    explicit me_http_app_t(const server_id_t &_me) : me(_me) { }
private:
    void handle(const http_req_t &req, http_res_t *result, signal_t *) {
        if (req.method != http_method_t::GET) {
            *result = http_res_t(http_status_code_t::METHOD_NOT_ALLOWED);
            return;
        }
        if (!req.query_params.empty()) {
            *result = http_error_res("Unexpected query param");
            return;
        }
        *result = http_res_t(http_status_code_t::OK,
                             "application/json", '"' + me.print() + '"');
    }
    server_id_t me;
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_ME_APP_HPP_ */
