// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_
#define CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "clustering/administration/persistable_blueprint.hpp"
#include "clustering/generic/nonoverlapping_regions.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/reactor_json_adapters.hpp"
#include "clustering/reactor/metadata.hpp"
#include "containers/cow_ptr.hpp"
#include "containers/name_string.hpp"
#include "containers/uuid.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "rpc/semilattice/joins/versioned.hpp"
#include "rpc/serialize_macros.hpp"


/* This is the metadata for a single namespace of a specific protocol. */

class ack_expectation_t {
public:
    ack_expectation_t() : expectation_(0), hard_durability_(true) { }

    explicit ack_expectation_t(uint32_t expectation, bool hard_durability) :
        expectation_(expectation),
        hard_durability_(hard_durability) { }

    uint32_t expectation() const { return expectation_; }
    bool is_hardly_durable() const { return hard_durability_; }

    RDB_DECLARE_ME_SERIALIZABLE;

    bool operator==(ack_expectation_t other) const;

private:
    friend json_adapter_if_t::json_adapter_map_t get_json_subfields(ack_expectation_t *target);

    uint32_t expectation_;
    bool hard_durability_;
};

RDB_SERIALIZE_OUTSIDE(ack_expectation_t);

void debug_print(printf_buffer_t *buf, const ack_expectation_t &x);

// ctx-less json adapter concept for ack_expectation_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(ack_expectation_t *target);
cJSON *render_as_json(ack_expectation_t *target);
void apply_json_to(cJSON *change, ack_expectation_t *target);

/* `table_config_t` describes the contents of the `rethinkdb.table_config` artificial
table. */

class table_config_t {
public:
    class shard_t {
    public:
        std::set<machine_id_t> replicas;
        machine_id_t director;
    };
    std::vector<shard_t> shards;
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
};

RDB_DECLARE_SERIALIZABLE(table_shard_scheme_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_shard_scheme_t);

/* `table_replication_info_t` exists because the `table_config_t` needs to be under the
same vector clock as `chosen_directors`. */

class table_replication_info_t {
public:
    table_config_t config;

    table_shard_scheme_t shard_scheme;
};

RDB_DECLARE_SERIALIZABLE(table_replication_info_t);
RDB_DECLARE_EQUALITY_COMPARABLE(table_replication_info_t);

class namespace_semilattice_metadata_t {
public:
    namespace_semilattice_metadata_t() { }

    versioned_t<name_string_t> name;   // TODO maybe this belongs on table_config_t
    versioned_t<database_id_t> database;   // TODO this should never actually change
    versioned_t<std::string> primary_key;   // TODO: This should never actually change

    versioned_t<table_replication_info_t> replication_info;
};

RDB_DECLARE_SERIALIZABLE(namespace_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(namespace_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(namespace_semilattice_metadata_t);

/* This is the metadata for all of the tables. */
class namespaces_semilattice_metadata_t {
public:
    typedef std::map<namespace_id_t,
        deletable_t<namespace_semilattice_metadata_t> > namespace_map_t;
    namespace_map_t namespaces;
};

RDB_DECLARE_SERIALIZABLE(namespaces_semilattice_metadata_t);
RDB_DECLARE_SEMILATTICE_JOINABLE(namespaces_semilattice_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(namespaces_semilattice_metadata_t);


class namespaces_directory_metadata_t {
public:
    namespaces_directory_metadata_t() { }
    namespaces_directory_metadata_t(const namespaces_directory_metadata_t &other) {
        *this = other;
    }
    namespaces_directory_metadata_t(namespaces_directory_metadata_t &&other) {
        *this = std::move(other);
    }
    namespaces_directory_metadata_t &operator=(const namespaces_directory_metadata_t &other) {
        reactor_bcards = other.reactor_bcards;
        return *this;
    }
    namespaces_directory_metadata_t &operator=(namespaces_directory_metadata_t &&other) {
        reactor_bcards = std::move(other.reactor_bcards);
        return *this;
    }

    /* This used to say `reactor_business_card_t` instead of
    `cow_ptr_t<reactor_business_card_t>`, but that
    was extremely slow because the size of the data structure grew linearly with
    the number of tables and so copying it became a major cost. Using a
    `boost::shared_ptr` instead makes it significantly faster. */
    typedef std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > reactor_bcards_map_t;

    reactor_bcards_map_t reactor_bcards;
};

RDB_DECLARE_SERIALIZABLE(namespaces_directory_metadata_t);
RDB_DECLARE_EQUALITY_COMPARABLE(namespaces_directory_metadata_t);

// ctx-less json adapter concept for namespaces_directory_metadata_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(namespaces_directory_metadata_t *target);

cJSON *render_as_json(namespaces_directory_metadata_t *target);

void apply_json_to(cJSON *change, namespaces_directory_metadata_t *target);

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_METADATA_HPP_ */
