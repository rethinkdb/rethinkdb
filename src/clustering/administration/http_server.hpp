#ifndef __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__
#define __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rpc/semilattice/view.hpp"

class blueprint_http_server_t : public http_server_t {
    typedef semilattice_readwrite_view_t<cluster_semilattice_metadata_t> internal_view_t;

public:
    blueprint_http_server_t(boost::shared_ptr<internal_view_t> _semilattice_metadata, 
                            boost::uuids::uuid _us,
                            int port)
        : http_server_t(port), semilattice_metadata(_semilattice_metadata), us(_us)
    { }

private:
    http_res_t handle(const http_req_t &);

    boost::shared_ptr<internal_view_t> semilattice_metadata;
    boost::uuids::uuid us;
};

#endif
