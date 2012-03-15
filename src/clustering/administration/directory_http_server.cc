#include "errors.hpp"
#include "http/http.hpp"
#include "clustering/administration/directory_http_server.hpp"

directory_http_server_t::directory_http_server_t(clone_ptr_t<directory_view_t>& _directory_metadata)
    : directory_metadata(_directory_metadata) { }

http_res_t directory_http_server_t::handle(UNUSED const http_req_t &req) {
    return http_res_t(405);
}

