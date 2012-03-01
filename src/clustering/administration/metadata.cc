#include "clustering/administration/metadata.hpp"

cluster_semilattice_metadata_t::cluster_semilattice_metadata_t() { }

cluster_semilattice_metadata_t::cluster_semilattice_metadata_t(const machine_id_t &us) {
    machines.machines.insert(us);
}

/* semilattice concept for cluster_semilattice_metadata_t */
bool operator==(const cluster_semilattice_metadata_t& a, const cluster_semilattice_metadata_t& b) {
    return a.namespaces == b.namespaces && a.machines == b.machines;
}

void semilattice_join(cluster_semilattice_metadata_t *a, const cluster_semilattice_metadata_t &b) {
    semilattice_join(&a->namespaces, b.namespaces);
    semilattice_join(&a->machines, b.machines);
}
