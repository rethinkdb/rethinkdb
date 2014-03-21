// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/issue_subscription.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "http/http.hpp"
#include "rpc/semilattice/view.hpp"

class http_server_t;
class routing_http_app_t;
class file_http_app_t;
class cluster_semilattice_http_app_t;
class auth_semilattice_http_app_t;
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
        const std::set<ip_address_t> &local_addresses,
        int port,
        mailbox_manager_t *mbox_manager,
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        metadata_change_handler_t<auth_semilattice_metadata_t> *_auth_change_handler,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > _directory_metadata,
        namespace_repo_t<memcached_protocol_t> *_namespace_repo,
        namespace_repo_t<rdb_protocol_t> *_rdb_namespace_repo,
        admin_tracker_t *_admin_tracker,
        http_app_t *reql_app,
        uuid_u _us,
        std::string _path);
    ~administrative_http_server_manager_t();

    int get_port() const;
private:

    scoped_ptr_t<file_http_app_t> file_app;
    scoped_ptr_t<cluster_semilattice_http_app_t> cluster_semilattice_app;
    scoped_ptr_t<auth_semilattice_http_app_t> auth_semilattice_app;
    scoped_ptr_t<directory_http_app_t> directory_app;
    scoped_ptr_t<issues_http_app_t> issues_app;
    scoped_ptr_t<stat_http_app_t> stat_app;
    scoped_ptr_t<last_seen_http_app_t> last_seen_app;
    scoped_ptr_t<log_http_app_t> log_app;
    scoped_ptr_t<progress_app_t> progress_app;
    scoped_ptr_t<distribution_app_t> distribution_app;
    scoped_ptr_t<combining_http_app_t> combining_app;
#ifndef NDEBUG
    scoped_ptr_t<cyanide_http_app_t> cyanide_app;
#endif
    scoped_ptr_t<routing_http_app_t> ajax_routing_app;
    scoped_ptr_t<routing_http_app_t> root_routing_app;
    scoped_ptr_t<http_server_t> server;

    DISABLE_COPYING(administrative_http_server_manager_t);  // kind of redundant with the scoped_ptrs but too bad.
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_ */
