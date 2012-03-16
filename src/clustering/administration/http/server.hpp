#ifndef CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/directory/read_view.hpp"
#include "rpc/semilattice/view.hpp"

class http_server_t;
class routing_http_app_t;
class file_http_app_t;
class semilattice_http_app_t;
class directory_http_app_t;
class issues_http_app_t;

class administrative_http_server_manager_t {

public:
    administrative_http_server_manager_t(
        int port,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata, 
        clone_ptr_t<directory_rview_t<cluster_directory_metadata_t> > _directory_metadata,
        global_issue_tracker_t *_issue_tracker,
        boost::uuids::uuid _us);
    ~administrative_http_server_manager_t();

private:
    boost::scoped_ptr<file_http_app_t> file_app;
    boost::scoped_ptr<semilattice_http_app_t> semilattice_app;
    boost::scoped_ptr<directory_http_app_t> directory_app;
    boost::scoped_ptr<issues_http_app_t> issues_app;
    boost::scoped_ptr<routing_http_app_t> ajax_routing_app;
    boost::scoped_ptr<routing_http_app_t> root_routing_app;
    boost::scoped_ptr<http_server_t> server;
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_ */
