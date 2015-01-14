// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "clustering/administration/servers/server_metadata.hpp"
#include "clustering/administration/tables/database_metadata.hpp"
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"

/* This is the metadata for a single table. */

class write_ack_config_t {
public:
    enum class mode_t { single, majority, complex };
    class req_t {
    public:
        std::set<server_id_t> replicas;
        mode_t mode;   /* must not be `complex` */
    };
    mode_t mode;
    /* `complex_reqs` must be empty unless `mode` is `complex`. */
    std::vector<req_t> complex_reqs;
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    write_ack_config_t::mode_t,
    int8_t,
    write_ack_config_t::mode_t::single,
    write_ack_config_t::mode_t::complex);
RDB_DECLARE_SERIALIZABLE(write_ack_config_t::req_t);
RDB_DECLARE_EQUALITY_COMPARABLE(write_ack_config_t::req_t);
RDB_DECLARE_SERIALIZABLE(write_ack_config_t);
RDB_DECLARE_EQUALITY_COMPARABLE(write_ack_config_t);

/* `table_config_t` describes the contents of the `rethinkdb.table_config` artificial
table. */

class table_config_t {
public:
    class shard_t {
    public:
        std::set<server_id_t> replicas;
        server_id_t primary_replica;
    };
    database_id_t database;
    name_string_t name;
    std::string primary_key;
    std::vector<shard_t> shards;
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
sync with the `table_shard_scheme_t`. */

class table_config_and_shards_t {
public:
    table_config_t config;
    table_shard_scheme_t shard_scheme;
};

RDB_DECLARE_SERIALIZABLE(table_config_and_shards_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_config_and_shards_t);

/* `write_ack_config_checker_t` is used for checking if a set of acks satisfies the
requirements in the given `table_config_t`. The reason it needs the `table_config_t` and
the `servers_semilattice_metadata_t` is because the meaning of the `write_ack_config_t`
may depend on how many replicas the shards have and on which servers have been
permanently removed. The reason it's an object instead of a function is that it caches
intermediate results for best performance. */
class write_ack_config_checker_t {
public:
    /* The default constructor results in a checker with undefined content. In the
    current implementation, calling `check_acks()` on a default-constructed checker will
    always return `true`; but don't rely on this behavior. */
    write_ack_config_checker_t() { }
    write_ack_config_checker_t(const table_config_t &config,
                               const servers_semilattice_metadata_t &servers);
    bool check_acks(const std::set<server_id_t> &acks) const;
private:
    std::vector<std::pair<std::set<server_id_t>, size_t> > reqs;
};

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_ */
