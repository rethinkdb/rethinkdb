#ifndef CLUSTERING_ADMINSTRATION_HTTP_SEMILATTICE_APP_HPP_
#define CLUSTERING_ADMINSTRATION_HTTP_SEMILATTICE_APP_HPP_

#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

class semilattice_http_app_t : public http_app_t {
public:
    semilattice_http_app_t(
        const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        boost::uuids::uuid _us);
    http_res_t handle(const http_req_t &);
private:
    void fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata);

    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
    boost::uuids::uuid us;

    DISABLE_COPYING(semilattice_http_app_t);
};

#endif /* CLUSTERING_ADMINSTRATION_HTTP_SEMILATTICE_APP_HPP_ */
