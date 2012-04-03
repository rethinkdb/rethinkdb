#ifndef CLUSTERING_ADMINISTRATION_LAST_SEEN_TRACKER_HPP_
#define CLUSTERING_ADMINISTRATION_LAST_SEEN_TRACKER_HPP_

#include "clustering/administration/machine_metadata.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/semilattice/view.hpp"

class last_seen_tracker_t {
public:
    last_seen_tracker_t(
            const boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > &machines_view,
            const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_map);

    /* Return value is a UNIX timestamp. `machine` cannot be currently
    connected and also cannot have been declared dead. You are advised to hold
    a freeze on `machine_id_map` when calling, to avoid race conditions. */
    time_t get_last_seen(machine_id_t machine);

private:
    void update();
    void on_machines_view_change();
    void on_machine_id_map_change();

    boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view;
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_map;
    semilattice_read_view_t<machines_semilattice_metadata_t>::subscription_t machines_view_subs;
    watchable_t<std::map<peer_id_t, machine_id_t> >::subscription_t machine_id_map_subs;

    /* Machines are only present in this map if they are not connected but not
    declared dead. */
    std::map<machine_id_t, time_t> last_seen;
};

#endif /* CLUSTERING_ADMINISTRATION_LAST_SEEN_TRACKER_HPP_ */
