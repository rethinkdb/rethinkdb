// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/migrate/metadata_v1_16.hpp"

namespace metadata_v1_16 {

size_t table_shard_scheme_t::num_shards() const {
    return split_points.size() + 1;
}

key_range_t table_shard_scheme_t::get_shard_range(size_t i) const {
    guarantee(i < num_shards());
    store_key_t left = (i == 0) ? store_key_t::min() : split_points[i-1];
    if (i != num_shards() - 1) {
        return key_range_t(
                           key_range_t::closed, left,
                           key_range_t::open, split_points[i]);
    } else {
        return key_range_t(
                           key_range_t::closed, left,
                           key_range_t::none, store_key_t());
    }
}

RDB_IMPL_SERIALIZABLE_2(version_t, branch, timestamp);
RDB_IMPL_EQUALITY_COMPARABLE_2(version_t, branch, timestamp);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(version_t);

RDB_IMPL_SERIALIZABLE_2(version_range_t, earliest, latest);
RDB_IMPL_EQUALITY_COMPARABLE_2(version_range_t, earliest, latest);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(version_range_t);

RDB_IMPL_SERIALIZABLE_3(branch_birth_certificate_t, region, initial_timestamp, origin);
RDB_IMPL_EQUALITY_COMPARABLE_3(branch_birth_certificate_t, region, initial_timestamp, origin);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(branch_birth_certificate_t);

RDB_IMPL_SERIALIZABLE_1(branch_history_t, branches);
INSTANTIATE_DESERIALIZE_SINCE_v1_13(branch_history_t);

RDB_IMPL_SERIALIZABLE_3(server_semilattice_metadata_t, name, tags, cache_size_bytes);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(server_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_1(servers_semilattice_metadata_t, servers);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(servers_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_1(database_semilattice_metadata_t, name);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(database_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_1(databases_semilattice_metadata_t, databases);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(databases_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_2(write_ack_config_t::req_t, replicas, mode);
RDB_IMPL_EQUALITY_COMPARABLE_2(write_ack_config_t::req_t, replicas, mode);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(write_ack_config_t::req_t);

RDB_IMPL_SERIALIZABLE_2(write_ack_config_t, mode, complex_reqs);
RDB_IMPL_EQUALITY_COMPARABLE_2(write_ack_config_t, mode, complex_reqs);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(write_ack_config_t);

RDB_IMPL_SERIALIZABLE_2(table_config_t::shard_t, replicas, primary_replica);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_config_t::shard_t, replicas, primary_replica);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(table_config_t::shard_t);

RDB_IMPL_SERIALIZABLE_3(table_config_t, shards, write_ack_config, durability);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_config_t, shards, write_ack_config, durability);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(table_config_t);

RDB_IMPL_SERIALIZABLE_1(table_shard_scheme_t, split_points);
RDB_IMPL_EQUALITY_COMPARABLE_1(table_shard_scheme_t, split_points);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(table_shard_scheme_t);

RDB_IMPL_SERIALIZABLE_2(table_replication_info_t, config, shard_scheme);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_replication_info_t, config, shard_scheme);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(table_replication_info_t);

RDB_IMPL_SERIALIZABLE_4(namespace_semilattice_metadata_t, name, database, primary_key, replication_info);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(namespace_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_1(namespaces_semilattice_metadata_t, namespaces);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(namespaces_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_3(cluster_semilattice_metadata_t, rdb_namespaces, servers, databases);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(cluster_semilattice_metadata_t);

RDB_IMPL_SERIALIZABLE_1(auth_semilattice_metadata_t, auth_key);
INSTANTIATE_DESERIALIZE_SINCE_v1_16(auth_semilattice_metadata_t);

} // namespace metadata_v1_16
