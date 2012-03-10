#include "clustering/administration/datacenter_metadata.hpp"

/* semilattice concept for dataceneter_semilattice_metadata_t */
bool operator==(const datacenter_semilattice_metadata_t &a, const datacenter_semilattice_metadata_t &b) {
    return a.name == b.name;
}

void semilattice_join(datacenter_semilattice_metadata_t *a, const datacenter_semilattice_metadata_t &b) { 
    semilattice_join(&a->name, b.name);
}
