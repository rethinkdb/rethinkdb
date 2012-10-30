// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_PROGRESS_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_PROGRESS_APP_HPP_

#include <map>

#include "clustering/administration/metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "http/http.hpp"

// TODO: Name this progress_http_app_t like the rest of the
// http_app_t's in this directory.
class progress_app_t : public http_app_t {
public:
    progress_app_t(clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > _directory_metadata, mailbox_manager_t *_mbox_manager);
    http_res_t handle(const http_req_t &);

private:
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
    mailbox_manager_t *mbox_manager;

    DISABLE_COPYING(progress_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_PROGRESS_APP_HPP_ */
