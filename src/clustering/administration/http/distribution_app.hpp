#ifndef CLUSTERING_ADMINISTRATION_HTTP_DISTRIBUTION_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_DISTRIBUTION_APP_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "http/http.hpp"
#include "memcached/protocol.hpp"
#include "rpc/semilattice/view.hpp"

class distribution_app_t : public http_app_t {
public:
    distribution_app_t(boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > >, namespace_repo_t<memcached_protocol_t> *);
    http_res_t handle(const http_req_t &);

private:
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > > namespaces_sl_metadata;
    namespace_repo_t<memcached_protocol_t> *ns_repo;

    DISABLE_COPYING(distribution_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_DISTRIBUTION_APP_HPP_ */
