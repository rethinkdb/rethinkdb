// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PRE_V1_16_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_PRE_V1_16_METADATA_HPP_

#include <map>

#include "clustering/administration/metadata.hpp"
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "containers/archive/versioned.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "region/region.hpp"
#include "region/region_map.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/serialize_macros.hpp"

/* The transition from v1.15 to v1.16 changed the metadata format in some major ways.
This file preserves the pre-v1.16 metadata format, described by the types in the
`pre_v1_16` namespace. It also includes a function to convert from a pre-v1.16
`cluster_semilattice_metadata_t` to a v1.16 `cluster_semilattice_metadata_t`, which is a
non-trivial change. */

namespace pre_v1_16 {

typedef uuid_u datacenter_id_t;

typedef std::map<uuid_u, int> version_map_t;

template <class T>
class vclock_t {
public:
    typedef std::pair<version_map_t, T> stamped_value_t;

    typedef std::map<version_map_t, T> value_map_t;
    value_map_t values;

    RDB_MAKE_ME_SERIALIZABLE_1(values);
};

class persistable_blueprint_t {
public:
    typedef std::map<region_t, blueprint_role_t> region_to_role_map_t;
    typedef std::map<server_id_t, region_to_role_map_t> role_map_t;

    role_map_t machines_roles;
};
RDB_DECLARE_SERIALIZABLE(persistable_blueprint_t);

class database_semilattice_metadata_t {
public:
    vclock_t<name_string_t> name;
};
RDB_DECLARE_SERIALIZABLE(database_semilattice_metadata_t);

class databases_semilattice_metadata_t {
public:
    typedef std::map<database_id_t,
        deletable_t<pre_v1_16::database_semilattice_metadata_t> > database_map_t;
    database_map_t databases;
};
RDB_DECLARE_SERIALIZABLE(databases_semilattice_metadata_t);

class datacenter_semilattice_metadata_t {
public:
    vclock_t<name_string_t> name;
};
RDB_DECLARE_SERIALIZABLE(datacenter_semilattice_metadata_t);

class datacenters_semilattice_metadata_t {
public:
    typedef std::map<datacenter_id_t,
        deletable_t<pre_v1_16::datacenter_semilattice_metadata_t> > datacenter_map_t;
    datacenter_map_t datacenters;
};
RDB_DECLARE_SERIALIZABLE(datacenters_semilattice_metadata_t);

class machine_semilattice_metadata_t {
public:
    vclock_t<datacenter_id_t> datacenter;
    vclock_t<name_string_t> name;
};
RDB_DECLARE_SERIALIZABLE(machine_semilattice_metadata_t);

class machines_semilattice_metadata_t {
public:
    typedef std::map<server_id_t,
        deletable_t<pre_v1_16::machine_semilattice_metadata_t> > machine_map_t;
    machine_map_t machines;
};
RDB_DECLARE_SERIALIZABLE(machines_semilattice_metadata_t);

class ack_expectation_t {
public:
    uint32_t expectation_;
    bool hard_durability_;

    RDB_DECLARE_ME_SERIALIZABLE;
};

class namespace_semilattice_metadata_t {
public:
    namespace_semilattice_metadata_t() { }

    vclock_t<persistable_blueprint_t> blueprint;
    vclock_t<pre_v1_16::datacenter_id_t> primary_datacenter;
    vclock_t<std::map<pre_v1_16::datacenter_id_t, int32_t> > replica_affinities;
    vclock_t<std::map<pre_v1_16::datacenter_id_t, pre_v1_16::ack_expectation_t> >
        ack_expectations;
    vclock_t<nonoverlapping_regions_t> shards;
    vclock_t<name_string_t> name;
    vclock_t<region_map_t<server_id_t> > primary_pinnings;
    vclock_t<region_map_t<std::set<server_id_t> > > secondary_pinnings;
    vclock_t<std::string> primary_key;
    vclock_t<database_id_t> database;
};
RDB_DECLARE_SERIALIZABLE(namespace_semilattice_metadata_t);

class namespaces_semilattice_metadata_t {
public:
    typedef std::map<namespace_id_t,
        deletable_t<pre_v1_16::namespace_semilattice_metadata_t> > namespace_map_t;
    namespace_map_t namespaces;
};
RDB_DECLARE_SERIALIZABLE(namespaces_semilattice_metadata_t);

class cluster_semilattice_metadata_t {
public:
    cluster_semilattice_metadata_t() { }

    cow_ptr_t<pre_v1_16::namespaces_semilattice_metadata_t> rdb_namespaces;

    pre_v1_16::machines_semilattice_metadata_t machines;
    pre_v1_16::datacenters_semilattice_metadata_t datacenters;
    pre_v1_16::databases_semilattice_metadata_t databases;
};
RDB_DECLARE_SERIALIZABLE(cluster_semilattice_metadata_t);

class auth_semilattice_metadata_t {
public:
    auth_semilattice_metadata_t() { }

    vclock_t<auth_key_t> auth_key;
};
RDB_DECLARE_SERIALIZABLE(auth_semilattice_metadata_t);

} // namespace pre_v1_16

cluster_semilattice_metadata_t migrate_cluster_metadata_to_v1_16(
    const pre_v1_16::cluster_semilattice_metadata_t &old_metadata);
auth_semilattice_metadata_t migrate_auth_metadata_to_v1_16(
    const pre_v1_16::auth_semilattice_metadata_t &old_metadata);

#endif /* CLUSTERING_ADMINISTRATION_PRE_V1_16_METADATA_HPP_ */

