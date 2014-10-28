// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_

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

class server_semilattice_metadata_t {
public:
    /* `name` and `tags` should only be modified by the server that this metadata is
    describing. */
    versioned_t<name_string_t> name;
    versioned_t<std::set<name_string_t> > tags;
};

RDB_DECLARE_SERIALIZABLE(server_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(server_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(server_semilattice_metadata_t);

class servers_semilattice_metadata_t {
public:
    typedef std::map<server_id_t, deletable_t<server_semilattice_metadata_t> > server_map_t;
    server_map_t servers;
};

RDB_DECLARE_SERIALIZABLE(servers_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(servers_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(servers_semilattice_metadata_t);

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_METADATA_HPP_ */
