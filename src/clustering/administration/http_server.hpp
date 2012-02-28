#ifndef __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__
#define __CLUSTERING_ADMINSTRATION_HTTP_SERVER_HPP__

#include "clustering/adminstration/metadata.hpp"
#include "http/http.hpp"
#include "rpc/semilattice/view.hpp"
#include "mock/dummy_protocol.hpp"

class blueprint_http_server_t : public http_server_t {
    blueprint_http_server_t(int port)
        : http_server_t(port)
    { }

    http_res_t handle(const http_req_t &);

private:
    boost::shared_ptr<semilattice_readwrite_view_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > > namespaces_semilattice_metadata;
};

#endif
