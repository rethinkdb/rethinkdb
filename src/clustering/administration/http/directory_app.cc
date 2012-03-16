#include "errors.hpp"
#include "http/http.hpp"
#include "clustering/administration/http/directory_app.hpp"

directory_http_app_t::directory_http_app_t(clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> >& _directory_metadata)
    : directory_metadata(_directory_metadata) { }

http_res_t directory_http_app_t::handle(UNUSED const http_req_t &req) {
    return http_res_t(405);
}

