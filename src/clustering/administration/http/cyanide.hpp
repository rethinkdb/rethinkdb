// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_CYANIDE_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_CYANIDE_HPP_

#include "http/http.hpp"

/* This is an `http_app_t` whose jobs is to give us a way to kill a server over
 * http. */
class cyanide_http_app_t : public http_app_t {
    http_res_t handle(const http_req_t &) {
        crash("Goodbye sweet world\n");
        return http_res_t(HTTP_OK);
    }
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_CYANIDE_HPP_ */
