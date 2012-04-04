#include "clustering/administration/last_seen_tracker.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

last_seen_tracker_t::last_seen_tracker_t(
        const boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > &mv,
        const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &mim) :
    machines_view(mv), machine_id_map(mim),
    machines_view_subs(boost::bind(&last_seen_tracker_t::on_machines_view_change, this)),
    machine_id_map_subs(boost::bind(&last_seen_tracker_t::on_machine_id_map_change, this)) {

    /* We would freeze `machines_view` as well here if we could */
    watchable_t<std::map<peer_id_t, machine_id_t> >::freeze_t machine_id_map_freeze(machine_id_map);

    machines_view_subs.reset(machines_view);
    machine_id_map_subs.reset(machine_id_map, &machine_id_map_freeze);

    update();
}

void last_seen_tracker_t::update() {
    std::set<machine_id_t> visible;
    std::map<peer_id_t, machine_id_t> machine_ids = machine_id_map->get();
    for (std::map<peer_id_t, machine_id_t>::iterator it = machine_ids.begin(); it != machine_ids.end(); it++) {
        visible.insert(it->second);
    }
    machines_semilattice_metadata_t machine_metadata = machines_view->get();
    for (machines_semilattice_metadata_t::machine_map_t::iterator it = machine_metadata.machines.begin(); it != machine_metadata.machines.end(); it++) {
        if (!it->second.is_deleted() && visible.find(it->first) == visible.end()) {
            /* If it was already present, this will have no effect. */
            last_seen.insert(std::make_pair(it->first, time(NULL)));
        } else {
            last_seen.erase(it->first);
        }
    }
}

void last_seen_tracker_t::on_machines_view_change() {
    watchable_t<std::map<peer_id_t, machine_id_t> >::freeze_t freeze(machine_id_map);
    update();
}

void last_seen_tracker_t::on_machine_id_map_change() {
    /* We would freeze `machines_view` here if we could */
    update();
}
