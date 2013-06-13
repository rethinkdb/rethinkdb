// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_

#include <map>
#include <string>

#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"

template <class metadata_t>
class semilattice_http_app_t : public http_json_app_t {
public:
    semilattice_http_app_t(
        metadata_change_handler_t<metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us);
    virtual ~semilattice_http_app_t();

    http_res_t handle(const http_req_t &);
    void get_root(scoped_cJSON_t *json_out);

protected:
    virtual void metadata_change_callback(metadata_t *new_metadata, bool change_context) = 0;

    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
    uuid_u us;

private:
    // Helper method
    bool verify_content_type(const http_req_t &, const std::string &expected_content_type) const;

    metadata_change_handler_t<metadata_t> *metadata_change_handler;

    DISABLE_COPYING(semilattice_http_app_t);
};

class cluster_semilattice_http_app_t : public semilattice_http_app_t<cluster_semilattice_metadata_t> {
public:
    cluster_semilattice_http_app_t(
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us);
    ~cluster_semilattice_http_app_t();

private:
    void metadata_change_callback(cluster_semilattice_metadata_t *new_metadata, bool prefer_distribution);
};

class auth_semilattice_http_app_t : public semilattice_http_app_t<auth_semilattice_metadata_t> {
public:
    auth_semilattice_http_app_t(
        metadata_change_handler_t<auth_semilattice_metadata_t> *_metadata_change_handler,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > &_directory_metadata,
        uuid_u _us);
    ~auth_semilattice_http_app_t();

private:
    void metadata_change_callback(auth_semilattice_metadata_t *new_metadata, bool unused_context);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SEMILATTICE_APP_HPP_ */
