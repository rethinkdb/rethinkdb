// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/servers/last_seen_tracker.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

last_seen_tracker_t::last_seen_tracker_t(
        const boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > &mv,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, server_id_t> > > &mim) :
    servers_view(mv), server_id_map(mim),
    servers_view_subs(boost::bind(&last_seen_tracker_t::on_servers_view_change, this)),
    server_id_map_subs(boost::bind(&last_seen_tracker_t::on_server_id_map_change, this)) {

    /* We would freeze `servers_view` as well here if we could */
    watchable_t<change_tracking_map_t<peer_id_t, server_id_t> >::freeze_t server_id_map_freeze(server_id_map);

    servers_view_subs.reset(servers_view);
    server_id_map_subs.reset(server_id_map, &server_id_map_freeze);

    update();
}

void last_seen_tracker_t::update() {
    std::set<server_id_t> visible;
    std::map<peer_id_t, server_id_t> server_ids = server_id_map->get().get_inner();
    for (std::map<peer_id_t, server_id_t>::iterator it = server_ids.begin();
                                                     it != server_ids.end();
                                                     ++it) {
        visible.insert(it->second);
    }
    servers_semilattice_metadata_t server_metadata = servers_view->get();
    for (auto it = server_metadata.servers.begin();
         it != server_metadata.servers.end(); ++it) {
        if (!it->second.is_deleted() && visible.find(it->first) == visible.end()) {
            /* If it was already present, this will have no effect. */
            disconnected_times.insert(std::make_pair(it->first, current_microtime()));
        } else {
            disconnected_times.erase(it->first);
        }
        if (!it->second.is_deleted() && visible.find(it->first) != visible.end()) {
            /* If it was already present, this will have no effect. */
            connected_times.insert(std::make_pair(it->first, current_microtime()));
        } else {
            connected_times.erase(it->first);
        }
    }
}

void last_seen_tracker_t::on_servers_view_change() {
    watchable_t<change_tracking_map_t<peer_id_t, server_id_t> >::freeze_t freeze(server_id_map);
    update();
}

void last_seen_tracker_t::on_server_id_map_change() {
    /* We would freeze `servers_view` here if we could */
    update();
}
