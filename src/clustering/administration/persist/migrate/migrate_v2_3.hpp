// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_3_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_3_HPP_

#include "clustering/administration/persist/file.hpp"
#include "serializer/types.hpp"

// These functions are used to migrate metadata from v2.3 to the v2.4 format

// This will migrate all metadata from v2_3 to v2_4
void migrate_metadata_v2_3_to_v2_4(cluster_version_t serialization_version,
                                   metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor);

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_3_HPP_ */
