// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_1_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_1_HPP_

#include "clustering/administration/persist/file.hpp"
#include "serializer/types.hpp"

// These functions are used to migrate metadata from v2.1 through v2.2 to the v2.3 format

// This will migrate all metadata from v2_1 to v2_3 (including auth metadata).
void migrate_metadata_v2_1_to_v2_3(cluster_version_t serialization_version,
                                   metadata_file_t::write_txn_t *txn,
                                   signal_t *interruptor);

// This is only exposed because auth metadata used to be in a separate file,
//  and when migrating from that file, we need this function.  This function
//  assumes that the metadata is serialized using the latest version.
void migrate_auth_metadata_v2_1_to_v2_3(metadata_file_t::write_txn_t *txn,
                                        signal_t *interruptor);

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V2_1_HPP_ */
