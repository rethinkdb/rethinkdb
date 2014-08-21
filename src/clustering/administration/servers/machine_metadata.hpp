// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_MACHINE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_MACHINE_METADATA_HPP_

#include <map>
#include <set>
#include <string>

#include "containers/name_string.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

class machine_semilattice_metadata_t {
public:
    /* `name` and `tags` should only be modified by the machine that this metadata is
    describing. */
    versioned_t<name_string_t> name;
    versioned_t<std::set<name_string_t> > tags;
};

RDB_DECLARE_SERIALIZABLE(machine_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(machine_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(machine_semilattice_metadata_t);

class machines_semilattice_metadata_t {
public:
    typedef std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> > machine_map_t;
    machine_map_t machines;
};

RDB_DECLARE_SERIALIZABLE(machines_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(machines_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(machines_semilattice_metadata_t);

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_MACHINE_METADATA_HPP_ */
