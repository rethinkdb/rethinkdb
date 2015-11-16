// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_REWRITE_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_REWRITE_HPP_

#include "clustering/administration/persist/file.hpp"

// This function is used to update the serialized metadata to the
// latest serialization format, so it doesn't get out-of-date
void rewrite_cluster_metadata(metadata_file_t::write_txn_t *out,
                              signal_t *interruptor);

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_REWRITE_HPP_ */
