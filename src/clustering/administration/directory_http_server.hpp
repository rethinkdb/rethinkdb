#ifndef __CLUSTERING_ADMINSTRATION_DIRECTORY_HTTP_SERVER_HPP__
#define __CLUSTERING_ADMINSTRATION_DIRECTORY_HTTP_SERVER_HPP__

#include "clustering/administration/http_server.hpp"

class directory_http_server_t : public http_server_t {
    typedef blueprint_http_server_t::directory_view_t directory_view_t;
public:
    explicit directory_http_server_t(clone_ptr_t<directory_view_t>& _directory_metadata);
    http_res_t handle(const http_req_t &);
private:
    clone_ptr_t<directory_view_t> directory_metadata;
};

#endif
