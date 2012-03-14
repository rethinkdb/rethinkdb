#include "clustering/administration/http_server.hpp"
#include "clustering/administration/semilattice_http_server.hpp"
#include "clustering/administration/directory_http_server.hpp"

blueprint_http_server_t::blueprint_http_server_t(boost::shared_ptr<internal_view_t> _semilattice_metadata,
                            clone_ptr_t<directory_view_t> _directory_metadata,
                            boost::uuids::uuid _us,
                            int port)
    : semilattice_server(new semilattice_http_server_t(_semilattice_metadata, _directory_metadata, _us)),
      directory_server(new directory_http_server_t(_directory_metadata)),
      routing_server(port, semilattice_server.get(), blueprint_http_server_t::build_routes(directory_server.get())) { }

blueprint_http_server_t::blueprint_http_server_t(boost::shared_ptr<internal_view_t> _semilattice_metadata,
                            clone_ptr_t<directory_view_t> _directory_metadata,
                            boost::uuids::uuid _us)
    : semilattice_server(new semilattice_http_server_t(_semilattice_metadata, _directory_metadata, _us)),
      directory_server(new directory_http_server_t(_directory_metadata)),
      routing_server(semilattice_server.get(), build_routes(directory_server.get())) { }

http_res_t blueprint_http_server_t::handle(const http_req_t &req) {
    return routing_server.handle(req);
}

std::map<std::string,http_server_t*> blueprint_http_server_t::build_routes(http_server_t *directory_server) {
    std::map<std::string,http_server_t*> routes;
    routes["directory"] = directory_server;
    return routes;
}

