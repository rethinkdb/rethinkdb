// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_metadata.hpp"

#include "clustering/administration/tables/database_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"

RDB_IMPL_SERIALIZABLE_2(write_ack_config_t::req_t, replicas, mode);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const write_ack_config_t::req_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, write_ack_config_t::req_t *);
RDB_IMPL_EQUALITY_COMPARABLE_2(write_ack_config_t::req_t, replicas, mode);

RDB_IMPL_SERIALIZABLE_2(write_ack_config_t, mode, complex_reqs);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const write_ack_config_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, write_ack_config_t *);
RDB_IMPL_EQUALITY_COMPARABLE_2(write_ack_config_t, mode, complex_reqs);

RDB_IMPL_SERIALIZABLE_2(table_config_t::shard_t,
                        replicas, director);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const table_config_t::shard_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, table_config_t::shard_t *);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_config_t::shard_t,
                               replicas, director);

RDB_IMPL_SERIALIZABLE_2(table_config_t, shards, write_ack_config);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const table_config_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, table_config_t *);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_config_t, shards, write_ack_config);

RDB_IMPL_SERIALIZABLE_1(table_shard_scheme_t, split_points);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const table_shard_scheme_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, table_shard_scheme_t *);
RDB_IMPL_EQUALITY_COMPARABLE_1(table_shard_scheme_t, split_points);

RDB_IMPL_SERIALIZABLE_2(table_replication_info_t,
                        config, shard_scheme);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const table_replication_info_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, table_replication_info_t *);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_replication_info_t,
                               config, shard_scheme);

RDB_IMPL_SERIALIZABLE_4(namespace_semilattice_metadata_t,
                        name, database, primary_key, replication_info);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const namespace_semilattice_metadata_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, namespace_semilattice_metadata_t *);

RDB_IMPL_SEMILATTICE_JOINABLE_4(
        namespace_semilattice_metadata_t,
        name, database, primary_key, replication_info);
RDB_IMPL_EQUALITY_COMPARABLE_4(
        namespace_semilattice_metadata_t,
        name, database, primary_key, replication_info);

RDB_IMPL_SERIALIZABLE_1(namespaces_semilattice_metadata_t, namespaces);
template void serialize<cluster_version_t::v1_15_is_latest>(
            write_message_t *, const namespaces_semilattice_metadata_t &);
template archive_result_t deserialize<cluster_version_t::v1_15_is_latest>(
            read_stream_t *, namespaces_semilattice_metadata_t *);
RDB_IMPL_SEMILATTICE_JOINABLE_1(namespaces_semilattice_metadata_t, namespaces);
RDB_IMPL_EQUALITY_COMPARABLE_1(namespaces_semilattice_metadata_t, namespaces);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(database_semilattice_metadata_t, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(databases_semilattice_metadata_t, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);


