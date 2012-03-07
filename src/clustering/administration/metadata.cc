#include <utility>

#include "clustering/administration/metadata.hpp"

cluster_semilattice_metadata_t::cluster_semilattice_metadata_t() { }

cluster_semilattice_metadata_t::cluster_semilattice_metadata_t(const machine_id_t &us) {
    machines.machines.insert(std::make_pair(us, machine_semilattice_metadata_t()));
}

/* semilattice concept for cluster_semilattice_metadata_t */
bool operator==(const cluster_semilattice_metadata_t& a, const cluster_semilattice_metadata_t& b) {
    return a.dummy_namespaces == b.dummy_namespaces && 
           a.memcached_namespaces == b.memcached_namespaces && 
           a.machines == b.machines &&
           a.datacenters == b.datacenters;
}

void semilattice_join(cluster_semilattice_metadata_t *a, const cluster_semilattice_metadata_t &b) {
    semilattice_join(&a->dummy_namespaces, b.dummy_namespaces);
    semilattice_join(&a->memcached_namespaces, b.memcached_namespaces);
    semilattice_join(&a->machines, b.machines);
    semilattice_join(&a->datacenters, b.datacenters);
}

cluster_directory_metadata_t::cluster_directory_metadata_t() { }

cluster_directory_metadata_t::cluster_directory_metadata_t(machine_id_t mid) : machine_id(mid) { }
