// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_DIRECTORY_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_DIRECTORY_APP_HPP_

#include <map>

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "http/json/cJSON.hpp"

class directory_http_app_t : public http_json_app_t {
public:
    explicit directory_http_app_t(const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory_metadata);
    http_res_t handle(const http_req_t &);

protected:
    void get_root(scoped_cJSON_t *json_out);

private:
    cJSON *get_metadata_json(cluster_directory_metadata_t *metadata, http_req_t::resource_t::iterator path_begin, http_req_t::resource_t::iterator path_end) THROWS_ONLY(schema_mismatch_exc_t);

    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;

    DISABLE_COPYING(directory_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_DIRECTORY_APP_HPP_ */
