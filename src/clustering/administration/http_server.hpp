#ifndef __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__
#define __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "http/routing_server.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/directory/manager.hpp"

class semilattice_http_server_t;
class directory_http_server_t;

class blueprint_http_server_t : public http_server_t {
public:
    typedef semilattice_readwrite_view_t<cluster_semilattice_metadata_t> internal_view_t;
    typedef directory_rwview_t<cluster_directory_metadata_t> directory_view_t;

    // TODO (ivan): move uuid to be the first arg?
    blueprint_http_server_t(boost::shared_ptr<internal_view_t> _semilattice_metadata, 
                            clone_ptr_t<directory_view_t> _directory_metadata,
                            boost::uuids::uuid _us,
                            int port);
    blueprint_http_server_t(boost::shared_ptr<internal_view_t> _semilattice_metadata, 
                            clone_ptr_t<directory_view_t> _directory_metadata,
                            boost::uuids::uuid _us);

    http_res_t handle(const http_req_t &req);

private:
    static std::map<std::string, http_server_t *> build_routes(http_server_t *directory_server);

    boost::scoped_ptr<http_server_t> semilattice_server;
    boost::scoped_ptr<http_server_t> directory_server;
    routing_server_t routing_server;
};

#endif
