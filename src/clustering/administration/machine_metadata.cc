#include "clustering/administration/machine_metadata.hpp"

/* semilattice concept for machine_semilattice_metadata_t */
bool operator==(const machine_semilattice_metadata_t& a, const machine_semilattice_metadata_t& b) {
    return a.datacenter == b.datacenter &&
           a.name == b.name;
}

void semilattice_join(machine_semilattice_metadata_t *a, const machine_semilattice_metadata_t &b) {
    semilattice_join(&a->datacenter, b.datacenter);
    semilattice_join(&a->name, b.name);
}

/* semilattice concept for machines_semilattice_metadata_t */
bool operator==(const machines_semilattice_metadata_t& a, const machines_semilattice_metadata_t& b) {
    return a.machines == b.machines;
}

void semilattice_join(machines_semilattice_metadata_t *a, const machines_semilattice_metadata_t &b) {
    semilattice_join(&a->machines, b.machines);
}
