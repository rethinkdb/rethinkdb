// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/servers/network_logger.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

network_logger_t::network_logger_t(
        peer_id_t our_peer_id,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *dv,
        const boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > &sv) :
    us(our_peer_id),
    directory_view(dv), semilattice_view(sv),
    directory_subscription(directory_view,
        std::bind(&network_logger_t::on_change, this), false),
    semilattice_subscription(
        std::bind(&network_logger_t::on_change, this), semilattice_view)
{
    on_change();
}

void network_logger_t::on_change() {
    servers_semilattice_metadata_t semilattice = semilattice_view->get();

    std::set<server_id_t> servers_seen;
    std::set<peer_id_t> proxies_seen;

    directory_view->read_all(
    [&](const peer_id_t &peer_id, const cluster_directory_metadata_t *value) {
        switch (value->peer_type) {
            case SERVER_PEER: {
                servers_seen.insert(value->server_id);
                servers_semilattice_metadata_t::server_map_t::const_iterator jt
                    = semilattice.servers.find(value->server_id);
                /* If they don't appear in the servers map, assume that they're a new
                server and that an entry for them will appear as soon as the semilattice
                finishes syncing. */
                if (jt != semilattice.servers.end()) {
                    if (!static_cast<bool>(connected_servers.get_key(
                            value->server_id))) {
                        connected_servers.set_key(value->server_id, empty_value_t());
                        if (peer_id != us) {
                            logNTC("Connected to server %s",
                                pretty_print_server(value->server_id).c_str());
                        }
                    }
                }
                break;
            }
            case PROXY_PEER: {
                proxies_seen.insert(peer_id);
                if (connected_proxies.count(peer_id) == 0) {
                    connected_proxies.insert(peer_id);
                    logNTC("Connected to proxy %s",
                        uuid_to_str(peer_id.get_uuid()).c_str());
                }
                break;
            }
            default: unreachable();
        }
    });

    std::map<server_id_t, empty_value_t> connected_servers_copy =
        connected_servers.get_all();
    for (const auto &pair : connected_servers_copy) {
        if (servers_seen.count(pair.first) == 0) {
            connected_servers.delete_key(pair.first);
            logINF("Disconnected from server %s",
                pretty_print_server(pair.first).c_str());
        }
    }

    std::set<peer_id_t> connected_proxies_copy = connected_proxies;
    for (const auto &proxy : connected_proxies_copy) {
        if (proxies_seen.count(proxy) == 0) {
            connected_proxies.erase(proxy);
            logINF("Disconnected from proxy %s", uuid_to_str(proxy.get_uuid()).c_str());
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
