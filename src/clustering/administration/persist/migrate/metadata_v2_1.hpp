// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_METADATA_V2_1_HPP_
#define CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_METADATA_V2_1_HPP_

#include <string>

#include "clustering/administration/persist/file.hpp"
#include "clustering/administration/persist/migrate/metadata_v1_16.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/semilattice/joins/macros.hpp"

namespace metadata_v2_1 {

metadata_file_t::key_t<metadata_v1_16::auth_semilattice_metadata_t> mdkey_auth_semilattices();

} // namespace metadata_v2_1

#endif /* CLUSTERING_ADMINISTRATION_PERSIST_MIGRATE_METADATA_V2_1_HPP_ */
