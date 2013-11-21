// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/network_logger.hpp"

network_logger_t::network_logger_t(
        peer_id_t our_peer_id,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> > > &dv,
        const boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > &sv) :
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
    machines_semilattice_metadata_t semilattice = semilattice_view->get();

    std::set<machine_id_t> servers_seen;
    std::set<peer_id_t> proxies_seen;

    /* C++11: auto op = [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *directory) -> void { ... }
    Because we cannot use C++11 lambdas yet due to missing support in
    GCC 4.4, this is the messy work-around: */
    struct op_closure_t {
        void operator()(const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *directory) {
            for (auto it = directory->get_inner().begin(); it != directory->get_inner().end(); it++) {
                if (it->first == parent->us) {
                    continue;
                }
                switch (it->second.peer_type) {
                    case ADMIN_PEER: {
                        break;
                    }
                    case SERVER_PEER: {
                        servers_seen.insert(it->second.machine_id);
                        machines_semilattice_metadata_t::machine_map_t::iterator jt = semilattice.machines.find(it->second.machine_id);
                        /* If they don't appear in the machines map, assume that they're
                        a new machine and that an entry for them will appear as soon as
                        the semilattice finishes syncing. */
                        if (jt != semilattice.machines.end()) {
                            if (parent->connected_servers.count(it->second.machine_id) == 0) {
                                parent->connected_servers.insert(it->second.machine_id);
                                logINF("Connected to server %s", parent->pretty_print_machine(it->second.machine_id).c_str());
                            }
                        }
                        break;
                    }
                    case PROXY_PEER: {
                        proxies_seen.insert(it->first);
                        if (parent->connected_proxies.count(it->first)) {
                            logINF("Connected to proxy %s", uuid_to_str(it->first.get_uuid()).c_str());
                        }
                        break;
                    }
                    default: unreachable();
                }
            }
        }
        op_closure_t(machines_semilattice_metadata_t &c1,
                     std::set<machine_id_t> &c2,
                     std::set<peer_id_t> &c3,
                     network_logger_t *c4) :
            semilattice(c1),
            servers_seen(c2),
            proxies_seen(c3),
            parent(c4) { }
        machines_semilattice_metadata_t &semilattice;
        std::set<machine_id_t> &servers_seen;
        std::set<peer_id_t> &proxies_seen;
        network_logger_t *parent; // Parent would go away in a C++11 lambda
    };
    op_closure_t op(semilattice, servers_seen, proxies_seen, this);

    directory_view->apply_read(std::bind(&op_closure_t::operator(), &op, std::placeholders::_1));

    std::set<machine_id_t> connected_servers_copy = connected_servers;
    for (std::set<machine_id_t>::iterator it = connected_servers_copy.begin(); it != connected_servers_copy.end(); it++) {
        if (servers_seen.count(*it) == 0) {
            connected_servers.erase(*it);
            logINF("Disconnected from server %s", pretty_print_machine(*it).c_str());
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

std::string network_logger_t::pretty_print_machine(machine_id_t id) {
    machines_semilattice_metadata_t semilattice = semilattice_view->get();
    machines_semilattice_metadata_t::machine_map_t::iterator jt = semilattice.machines.find(id);
    guarantee(jt != semilattice.machines.end());
    std::string name;
    if (jt->second.is_deleted()) {
        name = "<ghost machine>";
    } else if (jt->second.get_ref().name.in_conflict()) {
        name = "<name in conflict>";
    } else {
        name = "\"" + jt->second.get_ref().name.get().str() + "\"";
    }
    return name + " " + uuid_to_str(id);
}
