#ifndef __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__
#define __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rpc/semilattice/view.hpp"

class blueprint_http_server_t : public http_server_t {
    typedef semilattice_readwrite_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > internal_view_t;
    blueprint_http_server_t(boost::shared_ptr<internal_view_t> _namespaces_semilattice_metadata, 
                            boost::uuids::uuid _us,
                            int port)
        : http_server_t(port), namespaces_semilattice_metadata(_namespaces_semilattice_metadata), us(_us)
    { }

    http_res_t handle(const http_req_t &);

private:
    boost::shared_ptr<internal_view_t> namespaces_semilattice_metadata;
    boost::uuids::uuid us;
};

#endif
