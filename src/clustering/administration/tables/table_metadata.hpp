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
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "containers/name_string.hpp"
#include "containers/optional.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/semilattice/joins/macros.hpp"
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

class flush_interval_default_t {
public:
    bool operator==(flush_interval_default_t) const { return true; }
    RDB_MAKE_ME_SERIALIZABLE_0(flush_interval_default_t);
};
class flush_interval_never_t {
public:
    bool operator==(flush_interval_never_t) const { return true; }
    RDB_MAKE_ME_SERIALIZABLE_0(flush_interval_never_t);
};

class flush_interval_config_t {
public:
    // Use the default value, or never flush, or specify a non-negative double.
    boost::variant<flush_interval_default_t,
                   flush_interval_never_t,
                   double> variant;
};

flush_interval_config_t default_flush_interval_config();

RDB_MAKE_SERIALIZABLE_1(flush_interval_config_t, variant);

class user_data_t {
public:
    ql::datum_t datum;
};

user_data_t default_user_data();

/* `table_config_t` describes the complete contents of the `rethinkdb.table_config`
artificial table. */

class table_config_t {
public:
    class shard_t {
    public:
        std::set<server_id_t> voting_replicas() const;

        /* `nonvoting_replicas` must be a subset of `all_replicas`. `primary_replica`
        must be in `all_replicas` and not in `nonvoting_replicas`. */
        std::set<server_id_t> all_replicas;
        std::set<server_id_t> nonvoting_replicas;
        server_id_t primary_replica;
    };
    table_basic_config_t basic;
    std::vector<shard_t> shards;
    std::map<std::string, sindex_config_t> sindexes;
    optional<write_hook_config_t> write_hook;
    write_ack_config_t write_ack_config;
    write_durability_t durability;
    flush_interval_config_t flush_interval;
    user_data_t user_data;  // has user-exposed name "data"
};

RDB_DECLARE_EQUALITY_COMPARABLE(table_config_t);

RDB_DECLARE_SERIALIZABLE(table_config_t::shard_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_config_t::shard_t);

flush_interval_t get_flush_interval(const table_config_t &);

class table_shard_scheme_t {
public:
    std::vector<store_key_t> split_points;

    static table_shard_scheme_t one_shard() {
        return table_shard_scheme_t();
    }

    size_t num_shards() const {
        return split_points.size() + 1;
    }

    key_range_t get_shard_range(size_t i) const;
    size_t find_shard_for_key(const store_key_t &key) const;
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

class table_config_and_shards_change_t {
public:
    class set_table_config_and_shards_t {
    public:
        table_config_and_shards_t new_config_and_shards;
    };

    class write_hook_create_t {
    public:
        write_hook_config_t config;
    };

    class write_hook_drop_t {
    public:
    };

    class sindex_create_t {
    public:
        std::string name;
        sindex_config_t config;
    };

    class sindex_drop_t {
    public:
        std::string name;
    };

    class sindex_rename_t {
    public:
        std::string name;
        std::string new_name;
        bool overwrite;
    };

    table_config_and_shards_change_t() { }

    explicit table_config_and_shards_change_t(set_table_config_and_shards_t &&_change)
        : change(std::move(_change)) { }
    explicit table_config_and_shards_change_t(sindex_create_t &&_change)
        : change(std::move(_change)) { }
    explicit table_config_and_shards_change_t(sindex_drop_t &&_change)
        : change(std::move(_change)) { }
    explicit table_config_and_shards_change_t(sindex_rename_t &&_change)
        : change(std::move(_change)) { }
    explicit table_config_and_shards_change_t(write_hook_create_t &&_change)
        : change(std::move(_change)) { }
    explicit table_config_and_shards_change_t(write_hook_drop_t &&_change)
        : change(std::move(_change)) { }


    /* Note, it's important that `apply_change` does not change
    `table_config_and_shards` if it returns false. */
    bool apply_change(table_config_and_shards_t *table_config_and_shards) const;

    bool name_and_database_equal(const table_basic_config_t &table_basic_config) const;

    RDB_MAKE_ME_SERIALIZABLE_1(table_config_and_shards_change_t, change);

private:
    boost::variant<
        set_table_config_and_shards_t,
        sindex_create_t,
        sindex_drop_t,
        sindex_rename_t,
        write_hook_create_t,
        write_hook_drop_t> change;

    class apply_change_visitor_t;
};

RDB_DECLARE_SERIALIZABLE(table_config_and_shards_change_t::set_table_config_and_shards_t);
RDB_DECLARE_SERIALIZABLE(table_config_and_shards_change_t::write_hook_create_t);
RDB_DECLARE_SERIALIZABLE(table_config_and_shards_change_t::write_hook_drop_t);
RDB_DECLARE_SERIALIZABLE(table_config_and_shards_change_t::sindex_create_t);
RDB_DECLARE_SERIALIZABLE(table_config_and_shards_change_t::sindex_drop_t);
RDB_DECLARE_SERIALIZABLE(table_config_and_shards_change_t::sindex_rename_t);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_METADATA_HPP_ */
