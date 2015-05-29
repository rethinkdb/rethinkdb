// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_metadata.hpp"

#include "clustering/administration/tables/database_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"

// RSI(raft): Some of these should be `SINCE_v1_N`, where `N` is the version number at
// which Raft is released.

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(table_basic_config_t,
    name, database, primary_key);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_basic_config_t,
    name, database, primary_key);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(table_config_t::shard_t,
                                    replicas, primary_replica);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_config_t::shard_t,
                               replicas, primary_replica);

RDB_IMPL_SERIALIZABLE_5_SINCE_v1_16(table_config_t,
    basic, shards, sindexes, write_ack_config, durability);
RDB_IMPL_EQUALITY_COMPARABLE_5(table_config_t,
    basic, shards, sindexes, write_ack_config, durability);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(table_shard_scheme_t, split_points);
RDB_IMPL_EQUALITY_COMPARABLE_1(table_shard_scheme_t, split_points);

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(table_config_and_shards_t,
                                    config, shard_scheme, server_names);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_config_and_shards_t,
                               config, shard_scheme, server_names);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(database_semilattice_metadata_t, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(databases_semilattice_metadata_t, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

