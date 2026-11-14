// Copyright 2010-2026 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_4_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_4_HPP_

#include "clustering/administration/persist/file.hpp"
#include "serializer/types.hpp"

// These functions are used to migrate metadata from v2.4 to the v2.5 format

// This will migrate all metadata from v2_4 to v2_5
void migrate_metadata_v2_4_to_v2_5(cluster_version_t serialization_version,
                                   metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor);

#endif // CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_4_HPP_
