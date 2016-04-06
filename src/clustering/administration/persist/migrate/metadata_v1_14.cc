// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate/metadata_v1_14.hpp"

namespace metadata_v1_14 {

RDB_IMPL_SERIALIZABLE_1(persistable_blueprint_t, machines_roles);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(persistable_blueprint_t);
RDB_IMPL_SERIALIZABLE_1(database_semilattice_metadata_t, name);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(database_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_1(databases_semilattice_metadata_t, databases);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(databases_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_1(datacenter_semilattice_metadata_t, name);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(datacenter_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_1(datacenters_semilattice_metadata_t, datacenters);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(datacenters_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_2(machine_semilattice_metadata_t, datacenter, name);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(machine_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_1(machines_semilattice_metadata_t, machines);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(machines_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_2(ack_expectation_t, expectation_, hard_durability_);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(ack_expectation_t);
RDB_IMPL_SERIALIZABLE_10(namespace_semilattice_metadata_t,
                         blueprint, primary_datacenter, replica_affinities,
                         ack_expectations, shards, name, primary_pinnings,
                         secondary_pinnings, primary_key, database);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(namespace_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_1(namespaces_semilattice_metadata_t, namespaces);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(namespaces_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_4(cluster_semilattice_metadata_t,
                        rdb_namespaces, machines, datacenters, databases);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(cluster_semilattice_metadata_t);
RDB_IMPL_SERIALIZABLE_1(auth_key_t, key);
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(auth_key_t);
RDB_IMPL_SERIALIZABLE_1(auth_semilattice_metadata_t, auth_key);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(auth_semilattice_metadata_t);

} // namespace metadata_v1_14
