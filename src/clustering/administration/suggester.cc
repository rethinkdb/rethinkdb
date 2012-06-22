#include "clustering/administration/suggester.hpp"

void fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata,
                        std::map<peer_id_t, cluster_directory_metadata_t> directory,
                        const boost::uuids::uuid &us) {
    std::map<machine_id_t, datacenter_id_t> machine_assignments;

    for (std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it = cluster_metadata->machines.machines.begin();
            it != cluster_metadata->machines.machines.end();
            it++) {
        if (!it->second.is_deleted()) {
            machine_assignments[it->first] = it->second.get().datacenter.get();
        }
    }

    std::map<peer_id_t, namespaces_directory_metadata_t<memcached_protocol_t> > reactor_directory;
    std::map<peer_id_t, machine_id_t> machine_id_translation_table;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator it = directory.begin(); it != directory.end(); it++) {
        reactor_directory.insert(std::make_pair(it->first, it->second.memcached_namespaces));
        machine_id_translation_table.insert(std::make_pair(it->first, it->second.machine_id));
    }

    fill_in_blueprints_for_protocol<memcached_protocol_t>(&cluster_metadata->memcached_namespaces,
            reactor_directory,
            machine_id_translation_table,
            machine_assignments,
            us);
}
