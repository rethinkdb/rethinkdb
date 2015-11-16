// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V1_14_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V1_14_HPP_

#include "clustering/administration/persist/migrate/metadata_v1_14.hpp"
#include "clustering/administration/persist/migrate/metadata_v1_16.hpp"

// These functions are used to migrate metadata from v1.14 and v1.15 to the v1.16 format

metadata_v1_16::cluster_semilattice_metadata_t migrate_cluster_metadata_v1_14_to_v1_16(
    const metadata_v1_14::cluster_semilattice_metadata_t &old_metadata);
metadata_v1_16::auth_semilattice_metadata_t migrate_auth_metadata_v1_14_to_v1_16(
    const metadata_v1_14::auth_semilattice_metadata_t &old_metadata);

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V1_14_HPP_ */
