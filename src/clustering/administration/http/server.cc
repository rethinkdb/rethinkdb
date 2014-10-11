// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/http/server.hpp"

#include "clustering/administration/http/cyanide.hpp"
#include "clustering/administration/http/directory_app.hpp"
#include "clustering/administration/http/distribution_app.hpp"
#include "clustering/administration/http/issues_app.hpp"
#include "clustering/administration/http/last_seen_app.hpp"
#include "clustering/administration/http/log_app.hpp"
#include "clustering/administration/http/progress_app.hpp"
#include "clustering/administration/http/semilattice_app.hpp"
#include "clustering/administration/http/stat_app.hpp"
#include "clustering/administration/http/combining_app.hpp"
#include "http/file_app.hpp"
#include "http/http.hpp"
#include "http/routing_app.hpp"
#include "rpc/semilattice/view/field.hpp"

std::map<peer_id_t, log_server_business_card_t> get_log_mailbox(const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> &md) {
    std::map<peer_id_t, log_server_business_card_t> out;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = md.get_inner().begin(); it != md.get_inner().end(); it++) {
        out.insert(std::make_pair(it->first, it->second.log_mailbox));
    }
    return out;
}

std::map<peer_id_t, machine_id_t> get_machine_id(const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> &md) {
    std::map<peer_id_t, machine_id_t> out;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = md.get_inner().begin(); it != md.get_inner().end(); it++) {
        out.insert(std::make_pair(it->first, it->second.machine_id));
    }
    return out;
}

administrative_http_server_manager_t::administrative_http_server_manager_t(
        const std::set<ip_address_t> &local_addresses,
        int port,
        mailbox_manager_t *mbox_manager,
        metadata_change_handler_t<cluster_semilattice_metadata_t> *_metadata_change_handler,
        metadata_change_handler_t<auth_semilattice_metadata_t> *_auth_change_handler,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > _directory_metadata,
        real_reql_cluster_interface_t *_cluster_interface,
        admin_tracker_t *_admin_tracker,
        http_app_t *reql_app,
        uuid_u _us,
        std::string path)
{
    file_app.init(new file_http_app_t(path));

    cluster_semilattice_app.init(new cluster_semilattice_http_app_t(_metadata_change_handler, _directory_metadata, _us));
    auth_semilattice_app.init(new auth_semilattice_http_app_t(_auth_change_handler, _directory_metadata, _us));
    directory_app.init(new directory_http_app_t(_directory_metadata));
    issues_app.init(new issues_http_app_t(&_admin_tracker->issue_aggregator));
    stat_app.init(new stat_http_app_t(mbox_manager, _directory_metadata, _semilattice_metadata));
    last_seen_app.init(new last_seen_http_app_t(&_admin_tracker->last_seen_tracker));
    log_app.init(new log_http_app_t(mbox_manager,
        _directory_metadata->subview(&get_log_mailbox),
        _directory_metadata->subview(&get_machine_id)));
    progress_app.init(new progress_app_t(_directory_metadata, mbox_manager));
    distribution_app.init(new distribution_app_t(metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, _semilattice_metadata), _cluster_interface));

#ifndef NDEBUG
    cyanide_app.init(new cyanide_http_app_t);
#endif

    std::map<std::string, http_app_t *> ajax_routes;
    ajax_routes["directory"] = directory_app.get();
    ajax_routes["issues"] = issues_app.get();
    ajax_routes["stat"] = stat_app.get();
    ajax_routes["last_seen"] = last_seen_app.get();
    ajax_routes["log"] = log_app.get();
    ajax_routes["progress"] = progress_app.get();
    ajax_routes["distribution"] = distribution_app.get();
    ajax_routes["semilattice"] = cluster_semilattice_app.get();
    ajax_routes["auth"] = auth_semilattice_app.get();
    ajax_routes["reql"] = reql_app;
    DEBUG_ONLY_CODE(ajax_routes["cyanide"] = cyanide_app.get());

    std::map<std::string, http_json_app_t *> default_views;
    default_views["semilattice"] = cluster_semilattice_app.get();
    default_views["auth"] = auth_semilattice_app.get();
    default_views["directory"] = directory_app.get();
    default_views["issues"] = issues_app.get();
    default_views["last_seen"] = last_seen_app.get();

    combining_app.init(new combining_http_app_t(default_views));

    ajax_routing_app.init(new routing_http_app_t(combining_app.get(), ajax_routes));

    std::map<std::string, http_app_t *> root_routes;
    root_routes["ajax"] = ajax_routing_app.get();
    root_routing_app.init(new routing_http_app_t(file_app.get(), root_routes));

    server.init(new http_server_t(local_addresses, port, root_routing_app.get()));
}

administrative_http_server_manager_t::~administrative_http_server_manager_t() {
    /* This must be declared in the `.cc` file because the definitions of the
    destructors for the things in `scoped_ptr_t`s are not available from
    the `.hpp` file. */
}

int administrative_http_server_manager_t::get_port() const {
    return server->get_port();
}
