// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_metadata.hpp"

#include "clustering/administration/tables/database_metadata.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/protocol.hpp"

RDB_IMPL_SERIALIZABLE_3_SINCE_v2_1(table_basic_config_t,
    name, database, primary_key);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_basic_config_t,
    name, database, primary_key);

RDB_IMPL_SERIALIZABLE_3_SINCE_v2_1(table_config_t::shard_t,
    all_replicas, nonvoting_replicas, primary_replica);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_config_t::shard_t,
    all_replicas, nonvoting_replicas, primary_replica);

// We start with an empty object, not null -- because a good user would set fields of
// that object.
user_data_t default_user_data() {
    return user_data_t{ql::datum_t::empty_object()};
}

flush_interval_config_t default_flush_interval_config() {
    return flush_interval_config_t{flush_interval_default_t{}};
}

RDB_MAKE_SERIALIZABLE_1(user_data_t, datum);

RDB_IMPL_EQUALITY_COMPARABLE_1(user_data_t, datum);

RDB_IMPL_EQUALITY_COMPARABLE_1(flush_interval_config_t, variant);

RDB_DECLARE_SERIALIZABLE(table_config_t);

template <cluster_version_t W>
archive_result_t deserialize_table_config_pre_v2_4(
    read_stream_t *s, table_config_t *tc) {
    archive_result_t res;

    table_basic_config_t basic;
    res = deserialize<W>(s, &basic);
    if (bad(res)) { return res; }

    std::vector<table_config_t::shard_t> shards;
    res = deserialize<W>(s, &shards);
    if (bad(res)) { return res; }

    std::map<std::string, sindex_config_t> sindexes;
    res = deserialize<W>(s, &sindexes);
    if (bad(res)) { return res; }

    write_ack_config_t write_ack_config;
    res = deserialize<W>(s, &write_ack_config);
    if (bad(res)) { return res; }

    write_durability_t durability;
    res = deserialize<W>(s, &durability);
    if (bad(res)) { return res; }

    tc->basic = std::move(basic);
    tc->shards = std::move(shards);
    tc->sindexes = std::move(sindexes);
    tc->write_ack_config = std::move(write_ack_config);
    tc->durability = std::move(durability);
    tc->flush_interval = default_flush_interval_config();
    tc->user_data = default_user_data();

    return res;
}

archive_result_t deserialize_table_config_v2_4(
    read_stream_t *s, table_config_t *tc) {
    const cluster_version_t W = cluster_version_t::v2_4;
    archive_result_t res;

    table_basic_config_t basic;
    res = deserialize<W>(s, &basic);
    if (bad(res)) { return res; }

    std::vector<table_config_t::shard_t> shards;
    res = deserialize<W>(s, &shards);
    if (bad(res)) { return res; }

    std::map<std::string, sindex_config_t> sindexes;
    res = deserialize<W>(s, &sindexes);
    if (bad(res)) { return res; }

    optional<write_hook_config_t> write_hook;
    res = deserialize<W>(s, &write_hook);
    if (bad(res)) { return res; }

    write_ack_config_t write_ack_config;
    res = deserialize<W>(s, &write_ack_config);
    if (bad(res)) { return res; }

    write_durability_t durability;
    res = deserialize<W>(s, &durability);
    if (bad(res)) { return res; }

    *tc = table_config_t{std::move(basic),
                         std::move(shards),
                         std::move(sindexes),
                         std::move(write_hook),
                         std::move(write_ack_config),
                         std::move(durability),
                         default_flush_interval_config(),
                         default_user_data()};

    return res;
}

template <>
archive_result_t deserialize<cluster_version_t::v2_1>(
    read_stream_t *s, table_config_t *tc) {
    return deserialize_table_config_pre_v2_4<cluster_version_t::v2_1>(s, tc);
}

template <>
archive_result_t deserialize<cluster_version_t::v2_2>(
    read_stream_t *s, table_config_t *tc) {
    return deserialize_table_config_pre_v2_4<cluster_version_t::v2_2>(s, tc);
}

template <>
archive_result_t deserialize<cluster_version_t::v2_3>(
    read_stream_t *s, table_config_t *tc) {
    return deserialize_table_config_pre_v2_4<cluster_version_t::v2_3>(s, tc);
}

template <>
archive_result_t deserialize<cluster_version_t::v2_4>(
    read_stream_t *s, table_config_t *tc) {
    return deserialize_table_config_v2_4(s, tc);
}

RDB_IMPL_SERIALIZABLE_8_SINCE_v2_5(table_config_t,
    basic, shards, write_hook, sindexes, write_ack_config, durability,
    flush_interval, user_data);

RDB_IMPL_EQUALITY_COMPARABLE_8(table_config_t,
    basic, shards, write_hook, sindexes, write_ack_config, durability,
    flush_interval, user_data);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_16(table_shard_scheme_t, split_points);
RDB_IMPL_EQUALITY_COMPARABLE_1(table_shard_scheme_t, split_points);

RDB_IMPL_SERIALIZABLE_3_SINCE_v2_1(table_config_and_shards_t,
                                    config, shard_scheme, server_names);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_config_and_shards_t,
                               config, shard_scheme, server_names);

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(
    table_config_and_shards_change_t::set_table_config_and_shards_t,
    new_config_and_shards);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(table_config_and_shards_change_t::sindex_create_t,
    name, config);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(table_config_and_shards_change_t::sindex_drop_t,
    name);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(table_config_and_shards_change_t::sindex_rename_t,
    name, new_name, overwrite);

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(table_config_and_shards_change_t::write_hook_create_t, config);
RDB_IMPL_SERIALIZABLE_0_FOR_CLUSTER(table_config_and_shards_change_t::write_hook_drop_t);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(database_semilattice_metadata_t, name);
RDB_IMPL_SEMILATTICE_JOINABLE_1(database_semilattice_metadata_t, name);
RDB_IMPL_EQUALITY_COMPARABLE_1(database_semilattice_metadata_t, name);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(databases_semilattice_metadata_t, databases);
RDB_IMPL_SEMILATTICE_JOINABLE_1(databases_semilattice_metadata_t, databases);
RDB_IMPL_EQUALITY_COMPARABLE_1(databases_semilattice_metadata_t, databases);

struct get_flush_interval_visitor_t : public boost::static_visitor<flush_interval_t> {
    flush_interval_t operator()(flush_interval_default_t) const {
        return flush_interval_t{DEFAULT_FLUSH_INTERVAL};
    }
    flush_interval_t operator()(flush_interval_never_t) const {
        return flush_interval_t{NEVER_FLUSH_INTERVAL};
    }
    flush_interval_t operator()(double x) const {
        rassert(x >= 0);
        double value_ms = x * 1000;
        if (value_ms <= 0) {
            // The behavior of 0 is unspecified "reasonable" behavior.  For example, it
            // could be treated the same as default (5 seconds), or whatnot.  We go with
            // 100 ms.
            return flush_interval_t{100};
        }

        // We don't want any flush interval bigger than NEVER_FLUSH_INTERVAL.  (We also
        // don't want a value in milliseconds that would overflow when converted to
        // nanoseconds.  Hence this logic here.)
        static_assert(NEVER_FLUSH_INTERVAL == (0x100000000ll * 1000ll),
                      "NEVER_FLUSH_INTERVAL value changed");
        if (value_ms >= static_cast<double>(NEVER_FLUSH_INTERVAL)) {
            return flush_interval_t{NEVER_FLUSH_INTERVAL};
        }

        int64_t value_int = ceil(value_ms);
        return flush_interval_t{value_int};
    }
};

flush_interval_t get_flush_interval(const table_config_t &config) {
    return boost::apply_visitor(
        get_flush_interval_visitor_t{},
        config.flush_interval.variant);
}
