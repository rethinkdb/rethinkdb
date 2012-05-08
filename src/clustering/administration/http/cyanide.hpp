#ifndef __CLUSTERING_ADMINISTRATION_HTTP_CYANIDE_HPP__
#define __CLUSTERING_ADMINISTRATION_HTTP_CYANIDE_HPP__

#include "http/http.hpp"

/* This is an `http_app_t` whose jobs is to give us a way to kill a server over
 * http. */
class cyanide_http_app_t : public http_app_t {
    http_res_t handle(const http_req_t &) {
        crash("Goodbye sweet world\n");
        return http_res_t(200);
    }
};

#endif
