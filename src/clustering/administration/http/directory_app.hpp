#ifndef CLUSTERING_ADMINSTRATION_HTTP_DIRECTORY_APP_HPP_
#define CLUSTERING_ADMINSTRATION_HTTP_DIRECTORY_APP_HPP_

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "http/json/cJSON.hpp"

class directory_http_app_t : public http_app_t {
public:
    explicit directory_http_app_t(const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory_metadata);
    http_res_t handle(const http_req_t &);
private:
    cJSON *get_metadata_json(cluster_directory_metadata_t& metadata, http_req_t::resource_t::iterator path_begin, http_req_t::resource_t::iterator path_end) THROWS_ONLY(schema_mismatch_exc_t);

    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
};

#endif
