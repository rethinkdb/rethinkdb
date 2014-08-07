// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_LOOKUP_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_LOOKUP_HPP_

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "containers/name_string.hpp"

namespace_id_t lookup_table_with_database(
        const name_string_t &database,
        const name_string_t &table,
        const databases_semilattice_metadata_t &database_md,
        const cow_ptr_t<namespaces_semilattice_metadata_t> &namespace_md);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_LOOKUP_HPP_ */

