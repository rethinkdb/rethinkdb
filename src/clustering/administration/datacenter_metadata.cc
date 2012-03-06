#include "clustering/administration/datacenter_metadata.hpp"

/* semilattice concept for dataceneter_semilattice_metadata_t */
bool operator==(const datacenter_semilattice_metadata_t &, const datacenter_semilattice_metadata_t &) {
    return true;
}

void semilattice_join(datacenter_semilattice_metadata_t *, const datacenter_semilattice_metadata_t &) { }
