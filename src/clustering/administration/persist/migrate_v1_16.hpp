// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_V1_16_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_V1_16_HPP_

#include <string>

#include "buffer_cache/types.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist/file.hpp"
#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "serializer/types.hpp"

namespace migrate_v1_16 {

void migrate_cluster_metadata(
    txn_t *txn,
    buf_parent_t buf_parent,
    const void *old_superblock,
    metadata_file_t::write_txn_t *destination);

void migrate_auth_file(
    const serializer_filepath_t &path,
    metadata_file_t::write_txn_t *destination);

} // namespace migrate_v1_16

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_V1_16_HPP_ */
