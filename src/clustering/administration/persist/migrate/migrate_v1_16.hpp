// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V1_16_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V1_16_HPP_

#include "clustering/administration/persist/file.hpp"
#include "serializer/types.hpp"

// These functions are used to migrate metadata from v1.14 through v2.0 to the v2.1 format

void migrate_cluster_metadata_to_v2_1(io_backender_t *io_backender,
                                      const base_path_t &base_path,
                                      bool migrate_inconsistent_data,
                                      buf_parent_t buf_parent,
                                      const void *old_superblock,
                                      metadata_file_t::write_txn_t *out,
                                      signal_t *interruptor);

void migrate_auth_metadata_to_v2_1(io_backender_t *io_backender,
                                   const serializer_filepath_t &path,
                                   metadata_file_t::write_txn_t *out,
                                   signal_t *interruptor);

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_MIGRATE_V1_16_HPP_ */
