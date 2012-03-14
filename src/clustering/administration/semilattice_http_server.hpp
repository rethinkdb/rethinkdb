#ifndef __CLUSTERING_ADMINSTRATION_SEMILATTICE_HTTP_SERVER_HPP__
#define __CLUSTERING_ADMINSTRATION_SEMILATTICE_HTTP_SERVER_HPP__

#include "clustering/administration/http_server.hpp"

class semilattice_http_server_t : public http_server_t {
    typedef blueprint_http_server_t::internal_view_t internal_view_t;
    typedef blueprint_http_server_t::directory_view_t directory_view_t;
public:
    semilattice_http_server_t(boost::shared_ptr<internal_view_t> _semilattice_metadata, clone_ptr_t<directory_view_t> _directory_metadata, boost::uuids::uuid _us);
    http_res_t handle(const http_req_t &);
private:
    boost::shared_ptr<internal_view_t> semilattice_metadata;
    clone_ptr_t<directory_view_t> directory_metadata;
    boost::uuids::uuid us;
};

#endif
