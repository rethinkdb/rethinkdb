// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/servers/last_seen_tracker.hpp"

last_seen_tracker_t::last_seen_tracker_t(
        const boost::shared_ptr<semilattice_read_view_t<servers_semilattice_metadata_t> > &sv,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *d):
    servers_view(sv),
    directory(d),
    servers_view_subs(std::bind(
        &last_seen_tracker_t::update, this, false), servers_view),
    directory_subs(directory, std::bind(
        &last_seen_tracker_t::update, this, false), false) {
    update(true);
}

void last_seen_tracker_t::update(bool is_start) {
    ASSERT_FINITE_CORO_WAITING;
    std::set<server_id_t> visible;
    directory->read_all([&](const peer_id_t &, const cluster_directory_metadata_t *d) {
        visible.insert(d->server_id);
        });
    servers_semilattice_metadata_t server_metadata = servers_view->get();
    for (auto it = server_metadata.servers.begin();
         it != server_metadata.servers.end(); ++it) {
        if (!it->second.is_deleted() && visible.find(it->first) == visible.end()) {
            /* If the server was disconnected at startup, return `0` from
            `get_disconnected_time()` instead of our start time */
            microtime_t t = is_start ? 0 : current_microtime();
            /* If it was already present, this will have no effect. */
            disconnected_times.insert(std::make_pair(it->first, t));
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

