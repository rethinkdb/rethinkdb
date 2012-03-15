#ifndef __CLUSTERING_ADMINSTRATION_HTTP_DIRECTORY_APP_HPP__
#define __CLUSTERING_ADMINSTRATION_HTTP_DIRECTORY_APP_HPP__

#include "clustering/administration/metadata.hpp"
#include "http/http.hpp"
#include "rpc/directory/read_view.hpp"

class directory_http_app_t : public http_app_t {
public:
    directory_http_app_t(clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> >& _directory_metadata);
    http_res_t handle(const http_req_t &);
private:
    clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > directory_metadata;
};

#endif
