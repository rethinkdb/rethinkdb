#ifndef CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_

#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "rpc/semilattice/view.hpp"

class http_server_t;
class routing_http_app_t;
class file_http_app_t;
class semilattice_http_app_t;
class directory_http_app_t;
class issues_http_app_t;
class stat_http_app_t;
class last_seen_http_app_t;
class log_http_app_t;
class progress_app_t;
class stat_manager_t;
class distribution_app_t;
class cyanide_http_app_t;
class combining_http_app_t;

class administrative_http_server_manager_t {

public:
    administrative_http_server_manager_t(
        int port,
        mailbox_manager_t *mbox_manager,
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
        clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > _directory_metadata,
        namespace_repo_t<memcached_protocol_t> *_namespace_repo,
        admin_tracker_t *_admin_tracker,
        uuid_t _us,
        std::string _path);
    ~administrative_http_server_manager_t();

private:
    boost::scoped_ptr<file_http_app_t> file_app;
    boost::scoped_ptr<semilattice_http_app_t> semilattice_app;
    boost::scoped_ptr<directory_http_app_t> directory_app;
    boost::scoped_ptr<issues_http_app_t> issues_app;
    boost::scoped_ptr<stat_http_app_t> stat_app;
    boost::scoped_ptr<last_seen_http_app_t> last_seen_app;
    boost::scoped_ptr<log_http_app_t> log_app;
    boost::scoped_ptr<progress_app_t> progress_app;
    boost::scoped_ptr<distribution_app_t> distribution_app;
    boost::scoped_ptr<combining_http_app_t> combining_app;
#ifndef NDEBUG
    boost::scoped_ptr<cyanide_http_app_t> cyanide_app;
#endif
    boost::scoped_ptr<routing_http_app_t> ajax_routing_app;
    boost::scoped_ptr<routing_http_app_t> root_routing_app;
    boost::scoped_ptr<http_server_t> server;

    DISABLE_COPYING(administrative_http_server_manager_t);  // kind of redundant with the scoped_ptrs but too bad.
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_ */
