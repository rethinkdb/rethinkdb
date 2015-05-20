// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "buffer_cache/types.hpp"   // for `write_durability_t`
#include "clustering/administration/servers/server_metadata.hpp"
#include "clustering/administration/tables/database_metadata.hpp"
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"

/* This is the metadata for a single table. */

/* `table_basic_config_t` contains the subset of the table's configuration that the
parser needs to process queries against the table. A copy of this is stored on every
thread of every server for every table. */
class table_basic_config_t {
public:
    name_string_t name;
    database_id_t database;
    std::string primary_key;
};

RDB_DECLARE_SERIALIZABLE(table_basic_config_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_basic_config_t);

enum class write_ack_config_t {
    SINGLE,
    MAJORITY
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    write_ack_config_t,
    int8_t,
    write_ack_config_t::SINGLE,
    write_ack_config_t::MAJORITY);

/* `table_config_t` describes the complete contents of the `rethinkdb.table_config`
artificial table. */

class table_config_t {
public:
    class shard_t {
    public:
        std::set<server_id_t> replicas;
        server_id_t primary_replica;
    };
    table_basic_config_t basic;
    std::vector<shard_t> shards;
    std::map<std::string, sindex_config_t> sindexes;
    write_ack_config_t write_ack_config;
    write_durability_t durability;
};

RDB_DECLARE_SERIALIZABLE(table_config_t::shard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_config_t::shard_t);
RDB_DECLARE_SERIALIZABLE(table_config_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_config_t);

class table_shard_scheme_t {
public:
    std::vector<store_key_t> split_points;

    static table_shard_scheme_t one_shard() {
        return table_shard_scheme_t();
    }

    size_t num_shards() const {
        return split_points.size() + 1;
    }

    key_range_t get_shard_range(size_t i) const {
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

    size_t find_shard_for_key(const store_key_t &key) const {
        size_t ix = 0;
        while (ix < split_points.size() && key >= split_points[ix]) {
            ++ix;
        }
        return ix;
    }
};

RDB_DECLARE_SERIALIZABLE(table_shard_scheme_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_shard_scheme_t);

/* `table_config_and_shards_t` exists because the `table_config_t` needs to be changed in
sync with the `table_shard_scheme_t` and the server name mapping. */

class table_config_and_shards_t {
public:
    table_config_t config;
    table_shard_scheme_t shard_scheme;

    /* This contains an entry for every server mentioned in the config. The `uint64_t`s
    are server config versions. */
    server_name_map_t server_names;
};

RDB_DECLARE_SERIALIZABLE(table_config_and_shards_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_config_and_shards_t);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_ */
