#ifndef CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_

#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "http/json.hpp"

class semilattice_http_app_t : public http_json_app_t {
public:
    semilattice_http_app_t(
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        boost::uuids::uuid _us);
    http_res_t handle(const http_req_t &);
    void get_root(scoped_cJSON_t *json_out);

private:
    metadata_change_handler_t<cluster_semilattice_metadata_t> *metadata_change_handler;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
    boost::uuids::uuid us;

    DISABLE_COPYING(semilattice_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_ */
