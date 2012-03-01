#include "clustering/administration/machine_metadata.hpp"

/* semilattice concept for machines_semilattice_metadata_t */
bool operator==(const machines_semilattice_metadata_t& a, const machines_semilattice_metadata_t& b) {
    return a.machines == b.machines;
}

void semilattice_join(machines_semilattice_metadata_t *a, const machines_semilattice_metadata_t &b) {
    semilattice_join(&a->machines, b.machines);
}
