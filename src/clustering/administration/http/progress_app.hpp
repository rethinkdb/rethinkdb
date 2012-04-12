#ifndef __CLUSTERING_ADMINISTRATION_PROGRESS_APP_HPP__
#define __CLUSTERING_ADMINISTRATION_PROGRESS_APP_HPP__

#include "clustering/administration/metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "http/http.hpp"
#include "rpc/directory/read_view.hpp"

class progress_app_t : public http_app_t {
public:
    progress_app_t(clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> >& _directory_metadata, mailbox_manager_t *_mbox_manager);
    http_res_t handle(const http_req_t &);

private:
    clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > directory_metadata;
    mailbox_manager_t *mbox_manager;
};

#endif
