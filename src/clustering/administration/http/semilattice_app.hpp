#ifndef __CLUSTERING_ADMINSTRATION_HTTP_SEMILATTICE_APP_HPP__
#define __CLUSTERING_ADMINSTRATION_HTTP_SEMILATTICE_APP_HPP__

#include "clustering/administration/metadata.hpp"
#include "rpc/directory/read_view.hpp"
#include "rpc/semilattice/view.hpp"

class semilattice_http_app_t : public http_app_t {
public:
    semilattice_http_app_t(
        const boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > &_semilattice_metadata,
        const clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > &_directory_metadata,
        boost::uuids::uuid _us);
    http_res_t handle(const http_req_t &);
private:
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > directory_metadata;
    boost::uuids::uuid us;
};

#endif /* __CLUSTERING_ADMINSTRATION_HTTP_SEMILATTICE_APP_HPP__ */
