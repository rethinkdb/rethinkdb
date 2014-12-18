// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_DATABASE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_DATABASE_METADATA_HPP_

#include <map>
#include <string>

#include "containers/archive/stl_types.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "http/json.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"


class database_semilattice_metadata_t {
public:
    versioned_t<name_string_t> name;
};

RDB_DECLARE_SERIALIZABLE(database_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(database_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(database_semilattice_metadata_t);

void debug_print(printf_buffer_t *buf, const database_semilattice_metadata_t &x);


class databases_semilattice_metadata_t {
public:
    typedef std::map<database_id_t, deletable_t<database_semilattice_metadata_t> > database_map_t;
    database_map_t databases;
};

RDB_DECLARE_SERIALIZABLE(databases_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(databases_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(databases_semilattice_metadata_t);


#endif /* CLUSTERING_ADMINISTRATION_TABLES_DATABASE_METADATA_HPP_ */

