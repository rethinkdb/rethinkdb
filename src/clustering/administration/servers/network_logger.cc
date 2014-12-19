// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/servers/network_logger.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

network_logger_t::network_logger_t(
        peer_id_t our_peer_id,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &dv,
        const boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > &sv) :
    us(our_peer_id),
    directory_view(dv), semilattice_view(sv),
    directory_subscription(boost::bind(&network_logger_t::on_change, this)),
    semilattice_subscription(boost::bind(&network_logger_t::on_change, this))
{
    watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> >::freeze_t directory_freeze(directory_view);
    guarantee(directory_view->get().get_inner().empty());
    directory_subscription.reset(directory_view, &directory_freeze);
    semilattice_subscription.reset(semilattice_view);
}

void network_logger_t::on_change() {
    servers_semilattice_metadata_t semilattice = semilattice_view->get();

    std::set<server_id_t> servers_seen;
    std::set<peer_id_t> proxies_seen;

    struct op_closure_t {
        static void apply(const servers_semilattice_metadata_t &_semilattice,
                          std::set<server_id_t> *_servers_seen,
                          std::set<peer_id_t> *_proxies_seen,
                          network_logger_t *parent,
                          const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *directory) {
            for (auto it = directory->get_inner().begin(); it != directory->get_inner().end(); it++) {
                if (it->first == parent->us) {
                    continue;
                }
                switch (it->second.peer_type) {
                    case SERVER_PEER: {
                        _servers_seen->insert(it->second.server_id);
                        servers_semilattice_metadata_t::server_map_t::const_iterator jt
                            = _semilattice.servers.find(it->second.server_id);
                        /* If they don't appear in the servers map, assume that they're
                        a new server and that an entry for them will appear as soon as
                        the semilattice finishes syncing. */
                        if (jt != _semilattice.servers.end()) {
                            if (parent->connected_servers.count(it->second.server_id) == 0) {
                                parent->connected_servers.insert(it->second.server_id);
                                logNTC("Connected to server %s", parent->pretty_print_server(it->second.server_id).c_str());
                            }
                        }
                        break;
                    }
                    case PROXY_PEER: {
                        _proxies_seen->insert(it->first);
                        if (parent->connected_proxies.count(it->first)) {
                            logNTC("Connected to proxy %s", uuid_to_str(it->first.get_uuid()).c_str());
                        }
                        break;
                    }
                    default: unreachable();
                }
            }
        }
    };

    directory_view->apply_read(std::bind(&op_closure_t::apply,
                                         std::ref(semilattice),
                                         &servers_seen,
                                         &proxies_seen,
                                         this,
                                         ph::_1));

    std::set<server_id_t> connected_servers_copy = connected_servers;
    for (std::set<server_id_t>::iterator it = connected_servers_copy.begin(); it != connected_servers_copy.end(); it++) {
        if (servers_seen.count(*it) == 0) {
            connected_servers.erase(*it);
            logINF("Disconnected from server %s", pretty_print_server(*it).c_str());
        }
    }

    std::set<peer_id_t> connected_proxies_copy = connected_proxies;
    for (std::set<peer_id_t>::iterator it = connected_proxies_copy.begin(); it != connected_proxies_copy.end(); it++) {
        if (proxies_seen.count(*it) == 0) {
            connected_proxies.erase(*it);
            logINF("Disconnected from proxy %s", uuid_to_str(it->get_uuid()).c_str());
        }
    }
}

std::string network_logger_t::pretty_print_server(server_id_t id) {
    servers_semilattice_metadata_t semilattice = semilattice_view->get();
    servers_semilattice_metadata_t::server_map_t::iterator jt = semilattice.servers.find(id);
    guarantee(jt != semilattice.servers.end());
    std::string name;
    if (jt->second.is_deleted()) {
        name = "__deleted_server__";
    } else {
        name = "\"" + jt->second.get_ref().name.get_ref().str() + "\"";
    }
    return name + " " + uuid_to_str(id);
}
