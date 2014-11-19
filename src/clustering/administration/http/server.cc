// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/http/server.hpp"

#include "clustering/administration/http/cyanide.hpp"
#include "clustering/administration/http/directory_app.hpp"
#include "clustering/administration/http/log_app.hpp"
#include "clustering/administration/http/me_app.hpp"
#include "clustering/administration/http/combining_app.hpp"
#include "http/file_app.hpp"
#include "http/http.hpp"
#include "http/routing_app.hpp"
#include "rpc/semilattice/view/field.hpp"

/* RSI(reql_admin): Most of `/ajax` will go away when the ReQL admin API is mature enough
to replace it. */

std::map<peer_id_t, log_server_business_card_t> get_log_mailbox(const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> &md) {
    std::map<peer_id_t, log_server_business_card_t> out;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = md.get_inner().begin(); it != md.get_inner().end(); it++) {
        out.insert(std::make_pair(it->first, it->second.log_mailbox));
    }
    return out;
}

std::map<peer_id_t, server_id_t> get_server_id(const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> &md) {
    std::map<peer_id_t, server_id_t> out;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = md.get_inner().begin(); it != md.get_inner().end(); it++) {
        out.insert(std::make_pair(it->first, it->second.server_id));
    }
    return out;
}

administrative_http_server_manager_t::administrative_http_server_manager_t(
        const std::set<ip_address_t> &local_addresses,
        int port,
        const server_id_t &my_server_id,
        mailbox_manager_t *mbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > _directory_metadata,
        http_app_t *reql_app,
        std::string path)
{

    file_app.init(new file_http_app_t(path));

    me_app.init(new me_http_app_t(my_server_id));
    directory_app.init(new directory_http_app_t(_directory_metadata));
    log_app.init(new log_http_app_t(mbox_manager,
        _directory_metadata->subview(&get_log_mailbox),
        _directory_metadata->subview(&get_server_id)));

#ifndef NDEBUG
    cyanide_app.init(new cyanide_http_app_t);
#endif

    std::map<std::string, http_app_t *> ajax_routes;
    ajax_routes["me"] = me_app.get();
    ajax_routes["directory"] = directory_app.get();
    ajax_routes["log"] = log_app.get();
    ajax_routes["reql"] = reql_app;
    DEBUG_ONLY_CODE(ajax_routes["cyanide"] = cyanide_app.get());

    std::map<std::string, http_json_app_t *> default_views;
    default_views["directory"] = directory_app.get();

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
