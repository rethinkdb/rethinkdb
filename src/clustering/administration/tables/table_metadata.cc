// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_metadata.hpp"

#include "clustering/administration/tables/database_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(write_ack_config_t::req_t, replicas, mode);
RDB_IMPL_EQUALITY_COMPARABLE_2(write_ack_config_t::req_t, replicas, mode);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(write_ack_config_t, mode, complex_reqs);
RDB_IMPL_EQUALITY_COMPARABLE_2(write_ack_config_t, mode, complex_reqs);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(table_config_t::shard_t,
                                    replicas, primary_replica);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_config_t::shard_t,
                               replicas, primary_replica);

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(table_config_t,
                                    shards, write_ack_config, durability);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_config_t, shards, write_ack_config, durability);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(table_shard_scheme_t, split_points);
RDB_IMPL_EQUALITY_COMPARABLE_1(table_shard_scheme_t, split_points);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_16(table_replication_info_t,
                                    config, shard_scheme);
RDB_IMPL_EQUALITY_COMPARABLE_2(table_replication_info_t,
                               config, shard_scheme);

RDB_IMPL_SERIALIZABLE_4_SINCE_v1_16(namespace_semilattice_metadata_t,
                                    name, database, primary_key, replication_info);

RDB_IMPL_SEMILATTICE_JOINABLE_4(
        namespace_semilattice_metadata_t,
        name, database, primary_key, replication_info);
RDB_IMPL_EQUALITY_COMPARABLE_4(
        namespace_semilattice_metadata_t,
        name, database, primary_key, replication_info);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(namespaces_semilattice_metadata_t, namespaces);
RDB_IMPL_SEMILATTICE_JOINABLE_1(namespaces_semilattice_metadata_t, namespaces);
RDB_IMPL_EQUALITY_COMPARABLE_1(namespaces_semilattice_metadata_t, namespaces);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(database_semilattice_metadata_t, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(databases_semilattice_metadata_t, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

write_ack_config_checker_t::write_ack_config_checker_t(
        const table_config_t &config,
        const servers_semilattice_metadata_t &servers) {
    if (config.write_ack_config.mode != write_ack_config_t::mode_t::complex) {
        std::set<server_id_t> all_replicas;
        for (const table_config_t::shard_t &shard : config.shards) {
            for (const server_id_t &replica : shard.replicas) {
                if (!servers.servers.at(replica).is_deleted()) {
                    all_replicas.insert(replica);
                }
            }
        }
        size_t acks;
        if (config.write_ack_config.mode == write_ack_config_t::mode_t::single) {
            acks = 1;
        } else {
            size_t largest = 0;
            for (const table_config_t::shard_t &shard : config.shards) {
                size_t replicas = 0;
                for (const server_id_t &server_id : shard.replicas) {
                    if (!servers.servers.at(server_id).is_deleted()) {
                        ++replicas;
                    }
                }
                largest = std::max(largest, replicas);
            }
            acks = (largest + 2) / 2;
        }
        reqs.push_back(std::make_pair(all_replicas, acks));
    } else {
        for (const write_ack_config_t::req_t &req :
                config.write_ack_config.complex_reqs) {
            size_t acks;
            if (req.mode == write_ack_config_t::mode_t::single) {
                acks = 1;
            } else {
                size_t largest = 0;
                for (const table_config_t::shard_t &shard : config.shards) {
                    size_t overlap = 0;
                    for (const server_id_t &replica : req.replicas) {
                        if (!servers.servers.at(replica).is_deleted()) {
                            overlap += shard.replicas.count(replica);
                        }
                    }
                    largest = std::max(largest, overlap);
                }
                acks = (largest + 2) / 2;
            }
            reqs.push_back(std::make_pair(req.replicas, acks));
        }
    }
}

bool write_ack_config_checker_t::check_acks(const std::set<server_id_t> &acks) const {
    for (const std::pair<std::set<server_id_t>, size_t> &pair : reqs) {
        size_t count = 0;
        for (const server_id_t &server : acks) {
            count += pair.first.count(server);
        }
        if (count < pair.second) {
            return false;
        }
    }
    return true;
}

