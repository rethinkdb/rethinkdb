// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_

#include <map>
#include <string>

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "http/json.hpp"

class semilattice_http_app_t : public http_json_app_t {
public:
    semilattice_http_app_t(
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us);
    http_res_t handle(const http_req_t &);
    void get_root(scoped_cJSON_t *json_out);

private:
    // Helper method
    bool verify_content_type(const http_req_t &, const std::string &expected_content_type) const;

    metadata_change_handler_t<cluster_semilattice_metadata_t> *metadata_change_handler;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
    uuid_u us;

    DISABLE_COPYING(semilattice_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_ */
