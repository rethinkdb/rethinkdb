// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/stats/request.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

const char *cluster_stats_request_t::cluster_request_type = "cluster";
const char *server_stats_request_t::server_request_type = "server";
const char *table_stats_request_t::table_request_type = "table";
const char *table_server_stats_request_t::table_server_request_type = "table_server";

// Macros to make converting stats easier and more consistent
// The name of the field in the stats struct will be the same in the datum result
#define ADD_STAT(BUILDER, SUB_STATS, NAME) \
    (BUILDER).overwrite(#NAME, ql::datum_t((SUB_STATS).NAME))

#define ADD_CLUSTER_SERVER_STAT(BUILDER, STATS, NAME) \
    (BUILDER).overwrite(#NAME, ql::datum_t( \
        (STATS).accumulate(&parsed_stats_t::server_stats_t::NAME)));

#define ADD_CLUSTER_TABLE_STAT(BUILDER, STATS, NAME) \
    (BUILDER).overwrite(#NAME, ql::datum_t( \
        (STATS).accumulate(&parsed_stats_t::table_stats_t::NAME)));

#define ADD_TABLE_STAT(BUILDER, STATS, TABLE, NAME) \
    (BUILDER).overwrite(#NAME, ql::datum_t( \
        (STATS).accumulate_table(TABLE, &parsed_stats_t::table_stats_t::NAME)));

#define ADD_SERVER_STAT(BUILDER, STATS, SERVER, NAME) \
    (BUILDER).overwrite(#NAME, ql::datum_t( \
        (STATS).accumulate_server(SERVER, &parsed_stats_t::table_stats_t::NAME)));

parsed_stats_t::server_stats_t::server_stats_t() :
    responsive(false),
    queries_per_sec(0), queries_total(0),
    client_connections(0), clients_active(0) { }

parsed_stats_t::table_stats_t::table_stats_t() :
    read_docs_per_sec(0), read_docs_total(0),
    written_docs_per_sec(0), written_docs_total(0),
    in_use_bytes(0), metadata_bytes(0), data_bytes(0),
    garbage_bytes(0), preallocated_bytes(0),
    read_bytes_per_sec(0), read_bytes_total(0),
    written_bytes_per_sec(0), written_bytes_total(0) { }

parsed_stats_t::parsed_stats_t(const std::vector<ql::datum_t> &stats) {
    for (auto const &s : stats) {
        if (!s.has()) {
            continue;
        }

        ql::datum_t server_id_datum = s.get_field("server_id",
                                                  ql::throw_bool_t::NOTHROW);
        server_id_t server_id;
        admin_err_t error;
        bool res = convert_uuid_from_datum(server_id_datum, &server_id, &error);
        guarantee(res, "Stats contained an invalid server id. %s", error.msg.c_str());

        server_stats_t &serv_stats = servers[server_id];

        serv_stats.responsive = true;
        r_sanity_check(s.get_type() == ql::datum_t::R_OBJECT);
        for (size_t i = 0; i < s.obj_size(); ++i) {
            std::pair<datum_string_t, ql::datum_t> perf_pair = s.get_pair(i);
            if (perf_pair.first == "query_engine") {
                store_query_engine_stats(perf_pair.second, &serv_stats);
            } else {
                namespace_id_t table_id;
                res = str_to_uuid(perf_pair.first.to_std(), &table_id);
                if (res) {
                    store_table_stats(table_id, perf_pair.second, &serv_stats);
                }
            }
        }
    }
}

void parsed_stats_t::store_perfmon_value(const ql::datum_t &perf,
                                       const std::string &key,
                                       double *value_out) {
    rassert(*value_out == 0);
    ql::datum_t v = perf.get_field(key.c_str(), ql::throw_bool_t::NOTHROW);
    // If the value is missing, it should mean that the given stat was not specified
    // in the request, and therefore isn't needed - so we ignore any missing values.
    if (v.has()) {
        r_sanity_check(v.get_type() == ql::datum_t::R_NUM);
        *value_out = v.as_num();
    }
}

void parsed_stats_t::add_perfmon_value(const ql::datum_t &perf,
                                       const std::string &key,
                                       double *value_out) {
    ql::datum_t v = perf.get_field(key.c_str(), ql::throw_bool_t::NOTHROW);
    // If the value is missing, it should mean that the given stat was not specified
    // in the request, and therefore isn't needed - so we ignore any missing values.
    if (v.has()) {
        r_sanity_check(v.get_type() == ql::datum_t::R_NUM);
        *value_out += v.as_num();
    }
}

void parsed_stats_t::store_shard_values(const ql::datum_t &shard_perf,
                                        table_stats_t *stats_out) {
    r_sanity_check(shard_perf.get_type() == ql::datum_t::R_OBJECT);
    for (size_t i = 0; i < shard_perf.obj_size(); ++i) {
        std::pair<datum_string_t, ql::datum_t> pair = shard_perf.get_pair(i);
        if (pair.first.to_std().find("shard_") == 0) {
            r_sanity_check(pair.second.get_type() == ql::datum_t::R_OBJECT);
            for (size_t j = 0; j < pair.second.obj_size(); ++j) {
                std::pair<datum_string_t, ql::datum_t> sub_pair = pair.second.get_pair(j);
                std::string key = sub_pair.first.to_std();

                if (key.find("btree-") == 0) {
                    r_sanity_check(sub_pair.second.get_type() == ql::datum_t::R_OBJECT);
                    add_perfmon_value(sub_pair.second, "keys_read",
                                      &stats_out->read_docs_per_sec);
                    add_perfmon_value(sub_pair.second, "keys_set",
                                      &stats_out->written_docs_per_sec);
                    add_perfmon_value(sub_pair.second, "total_keys_read",
                                      &stats_out->read_docs_total);
                    add_perfmon_value(sub_pair.second, "total_keys_set",
                                      &stats_out->written_docs_total);
                } else if (key == "cache") {
                    add_perfmon_value(sub_pair.second, "in_use_bytes",
                                      &stats_out->in_use_bytes);
                }
            }
        }
    }
}

void parsed_stats_t::store_serializer_values(const ql::datum_t &ser_perf,
                                             table_stats_t *stats_out) {
    r_sanity_check(ser_perf.get_type() == ql::datum_t::R_OBJECT);
    store_perfmon_value(ser_perf, "serializer_read_bytes_per_sec",
                        &stats_out->read_bytes_per_sec);
    store_perfmon_value(ser_perf, "serializer_read_bytes_total",
                        &stats_out->read_bytes_total);
    store_perfmon_value(ser_perf, "serializer_written_bytes_per_sec",
                        &stats_out->written_bytes_per_sec);
    store_perfmon_value(ser_perf, "serializer_written_bytes_total",
                        &stats_out->written_bytes_total);

    // TODO: these are not entirely accurate, but the underlying stats would need
    // a good overhaul
    store_perfmon_value(ser_perf, "serializer_data_extents",
                        &stats_out->data_bytes);
    store_perfmon_value(ser_perf, "serializer_lba_extents",
                        &stats_out->metadata_bytes);
    store_perfmon_value(ser_perf, "serializer_old_garbage_block_bytes",
                        &stats_out->garbage_bytes);
    store_perfmon_value(ser_perf, "serializer_bytes_in_use",
                        &stats_out->preallocated_bytes);
    stats_out->data_bytes = stats_out->data_bytes * DEFAULT_EXTENT_SIZE - stats_out->garbage_bytes;
    stats_out->metadata_bytes *= DEFAULT_EXTENT_SIZE;
    stats_out->preallocated_bytes -= stats_out->data_bytes +
        stats_out->garbage_bytes + stats_out->metadata_bytes;
}

void parsed_stats_t::store_query_engine_stats(const ql::datum_t &qe_perf,
                                              server_stats_t *stats_out) {
    r_sanity_check(qe_perf.get_type() == ql::datum_t::R_OBJECT);
    store_perfmon_value(qe_perf, "queries_per_sec", &stats_out->queries_per_sec);
    store_perfmon_value(qe_perf, "queries_total", &stats_out->queries_total);
    store_perfmon_value(qe_perf, "client_connections", &stats_out->client_connections);
    store_perfmon_value(qe_perf, "clients_active", &stats_out->clients_active);
}

void parsed_stats_t::store_table_stats(const namespace_id_t &table_id,
                                       const ql::datum_t &table_perf,
                                       server_stats_t *stats_out) {
    r_sanity_check(table_perf.get_type() == ql::datum_t::R_OBJECT);
    ql::datum_t sers_perf = table_perf.get_field("serializers",
                                                 ql::throw_bool_t::NOTHROW);
    if (sers_perf.has()) {
        r_sanity_check(sers_perf.get_type() == ql::datum_t::R_OBJECT);
        table_stats_t &table_stats_out = stats_out->tables[table_id];

        store_shard_values(sers_perf, &table_stats_out);

        ql::datum_t sub_sers_perf = sers_perf.get_field("serializer",
                                                        ql::throw_bool_t::NOTHROW);
        if (sub_sers_perf.has()) {
            store_serializer_values(sub_sers_perf, &table_stats_out);
        }
    }
}

double parsed_stats_t::accumulate(double server_stats_t::*field) const {
    double res = 0;
    for (auto const &pair : servers) {
        res += pair.second.*field;
    }
    return res;
}

double parsed_stats_t::accumulate(double table_stats_t::*field) const {
    double res = 0;
    for (auto const &server_pair : servers) {
        for (auto const &table_pair : server_pair.second.tables) {
            res += table_pair.second.*field;
        }
    }
    return res;
}

double parsed_stats_t::accumulate_table(const namespace_id_t &table_id,
                                        double table_stats_t::*field) const {
    double res = 0;
    for (auto const &server_pair : servers) {
        auto const &table_it = server_pair.second.tables.find(table_id);
        if (table_it != server_pair.second.tables.end()) {
            res += table_it->second.*field;
        }
    }
    return res;
}

double parsed_stats_t::accumulate_server(const server_id_t &server_id,
                                         double table_stats_t::*field) const {
    double res = 0;
    auto const server_it = servers.find(server_id);
    r_sanity_check(server_it != servers.end());
    for (auto const &table_pair : server_it->second.tables) {
        res += table_pair.second.*field;
    }
    return res;
}

bool add_table_fields(const namespace_id_t &table_id,
                      const cluster_semilattice_metadata_t &metadata,
                      table_meta_client_t *table_meta_client,
                      admin_identifier_format_t admin_format,
                      ql::datum_object_builder_t *builder) {
    ql::datum_t db_identifier;
    ql::datum_t table_identifier;
    if (!convert_table_id_to_datums(table_id, admin_format, metadata, table_meta_client,
            &table_identifier, nullptr, &db_identifier, nullptr)) {
        return false; // The table was deleted or does not exist
    }
    builder->overwrite("db", db_identifier);
    builder->overwrite("table", table_identifier);
    return true;
}

// ------------------------------------
// stats_request_t
// ------------------------------------

std::set<std::vector<std::string> > stats_request_t::global_stats_filter() {
    return std::set<std::vector<std::string> >(
        { {"query_engine"},
          {"[0-9A-Fa-f-]+", "serializers" } });
}

std::vector<peer_id_t> stats_request_t::all_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory) {
    std::vector<peer_id_t> res;
    res.reserve(directory.size());
    for (auto const &pair : directory) {
        res.push_back(pair.first);
    }
    return res;
}

// ------------------------------------
// cluster_stats_request_t
// ------------------------------------

bool cluster_stats_request_t::parse(const ql::datum_t &info,
                                    scoped_ptr_t<stats_request_t> *request_out) {
    rassert(info.get(0).as_str() == get_name());
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 1) {
        return false;
    }
    request_out->init(new cluster_stats_request_t());
    return true;
}

cluster_stats_request_t::cluster_stats_request_t() { }

std::set<std::vector<std::string> > cluster_stats_request_t::get_filter() const {
    return std::set<std::vector<std::string> >(
        { {"query_engine" },
          {".*", "serializers", "shard_[0-9]+", "btree-.*", "keys_.*" } });
}

std::vector<peer_id_t> cluster_stats_request_t::get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *) const {
    return all_peers(directory);
}

bool cluster_stats_request_t::to_datum(const parsed_stats_t &stats,
                                       const metadata_t &,
                                       server_config_client_t *,
                                       table_meta_client_t *,
                                       admin_identifier_format_t,
                                       ql::datum_t *result_out) const {
    ql::datum_object_builder_t row_builder;
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    row_builder.overwrite("id", std::move(id_builder).to_datum());

    ql::datum_object_builder_t qe_builder;
    ADD_CLUSTER_SERVER_STAT(qe_builder, stats, queries_per_sec);
    ADD_CLUSTER_SERVER_STAT(qe_builder, stats, client_connections);
    ADD_CLUSTER_SERVER_STAT(qe_builder, stats, clients_active);
    ADD_CLUSTER_TABLE_STAT(qe_builder, stats, read_docs_per_sec);
    ADD_CLUSTER_TABLE_STAT(qe_builder, stats, written_docs_per_sec);
    row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());

    *result_out = std::move(row_builder).to_datum();
    return true;
}

// ------------------------------------
// table_stats_request_t
// ------------------------------------

bool table_stats_request_t::parse(const ql::datum_t &info,
                                  scoped_ptr_t<stats_request_t> *request_out) {
    rassert(info.get(0).as_str() == get_name());
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 2) {
        return false;
    }

    admin_err_t dummy_error;
    namespace_id_t t;
    if (!convert_uuid_from_datum(info.get(1), &t, &dummy_error)) {
        return false;
    }
    request_out->init(new table_stats_request_t(t));
    return true;
}

table_stats_request_t::table_stats_request_t(const namespace_id_t &_table_id) :
    table_id(_table_id) { }

std::set<std::vector<std::string> > table_stats_request_t::get_filter() const {
    return std::set<std::vector<std::string> >({
        { uuid_to_str(table_id), "serializers", "shard_[0-9]+", "btree-.*", "keys_.*" }
        });
}

std::vector<peer_id_t> table_stats_request_t::get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *) const {
    return all_peers(directory);
}

bool table_stats_request_t::to_datum(const parsed_stats_t &stats,
                                     const metadata_t &metadata,
                                     server_config_client_t *,
                                     table_meta_client_t *table_meta_client,
                                     admin_identifier_format_t admin_format,
                                     ql::datum_t *result_out) const {
    ql::datum_object_builder_t row_builder;
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    id_builder.add(convert_uuid_to_datum(table_id));
    row_builder.overwrite("id", std::move(id_builder).to_datum());

    if (!add_table_fields(table_id, metadata, table_meta_client, admin_format,
            &row_builder)) {
        return false;
    }

    ql::datum_object_builder_t qe_builder;
    ADD_TABLE_STAT(qe_builder, stats, table_id, read_docs_per_sec);
    ADD_TABLE_STAT(qe_builder, stats, table_id, written_docs_per_sec);
    row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());

    *result_out = std::move(row_builder).to_datum();
    return true;
}

// ------------------------------------
// server_stats_request_t
// ------------------------------------

bool server_stats_request_t::parse(const ql::datum_t &info,
                                   scoped_ptr_t<stats_request_t> *request_out) {
    rassert(info.get(0).as_str() == get_name());
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 2) {
        return false;
    }

    admin_err_t dummy_error;
    server_id_t s;
    if (!convert_uuid_from_datum(info.get(1), &s, &dummy_error)) {
        return false;
    }
    request_out->init(new server_stats_request_t(s));
    return true;
}

server_stats_request_t::server_stats_request_t(const server_id_t &_server_id) :
    server_id(_server_id) { }

std::set<std::vector<std::string> > server_stats_request_t::get_filter() const {
    return std::set<std::vector<std::string> >(
        { {"query_engine"},
          {".*", "serializers", "shard_[0-9]+", "btree-.*" } });
}

std::vector<peer_id_t> server_stats_request_t::get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &,
        server_config_client_t *server_config_client) const {
    boost::optional<peer_id_t> peer =
        server_config_client->get_server_to_peer_map()->get_key(server_id);
    if (!static_cast<bool>(peer)) {
        return std::vector<peer_id_t>();
    }
    return std::vector<peer_id_t>(1, peer.get());
}

bool server_stats_request_t::to_datum(const parsed_stats_t &stats,
                                      const metadata_t &,
                                      server_config_client_t *server_config_client,
                                      UNUSED table_meta_client_t *table_meta_client,
                                      admin_identifier_format_t admin_format,
                                      ql::datum_t *result_out) const {
    ql::datum_object_builder_t row_builder;
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    id_builder.add(convert_uuid_to_datum(server_id));
    row_builder.overwrite("id", std::move(id_builder).to_datum());

    ql::datum_t server_identifier;
    if (!convert_connected_server_id_to_datum(server_id, admin_format,
            server_config_client, &server_identifier, nullptr)) {
        return false;
    }
    row_builder.overwrite("server", server_identifier);

    auto const &server_it = stats.servers.find(server_id);
    if (server_it == stats.servers.end() ||
        !server_it->second.responsive) {
        row_builder.overwrite("error",
                              ql::datum_t("Timed out. Unable to retrieve stats."));
    } else {
        const parsed_stats_t::server_stats_t &server_stats = server_it->second;
        ql::datum_object_builder_t qe_builder;
        ADD_STAT(qe_builder, server_stats, client_connections);
        ADD_STAT(qe_builder, server_stats, clients_active);
        ADD_STAT(qe_builder, server_stats, queries_per_sec);
        ADD_STAT(qe_builder, server_stats, queries_total);
        ADD_SERVER_STAT(qe_builder, stats, server_id, read_docs_per_sec);
        ADD_SERVER_STAT(qe_builder, stats, server_id, read_docs_total);
        ADD_SERVER_STAT(qe_builder, stats, server_id, written_docs_per_sec);
        ADD_SERVER_STAT(qe_builder, stats, server_id, written_docs_total);
        row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());
    }
    *result_out = std::move(row_builder).to_datum();
    return true;
}

// ------------------------------------
// table_server_stats_request_t
// ------------------------------------

bool table_server_stats_request_t::parse(const ql::datum_t &info,
                                         scoped_ptr_t<stats_request_t> *request_out) {
    rassert(info.get(0).as_str() == get_name());
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 3) {
        return false;
    }

    admin_err_t dummy_error;
    namespace_id_t t;
    server_id_t s;
    if (!convert_uuid_from_datum(info.get(1), &t, &dummy_error)) {
        return false;
    }
    if (!convert_uuid_from_datum(info.get(2), &s, &dummy_error)) {
        return false;
    }
    request_out->init(new table_server_stats_request_t(t, s));
    return true;
}

table_server_stats_request_t::table_server_stats_request_t(
        const namespace_id_t &_table_id,
        const server_id_t &_server_id) :
    table_id(_table_id), server_id(_server_id) { }

std::set<std::vector<std::string> > table_server_stats_request_t::get_filter() const {
    return std::set<std::vector<std::string> >({
        { uuid_to_str(table_id), "serializers" } });
}

std::vector<peer_id_t> table_server_stats_request_t::get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &,
        server_config_client_t *server_config_client) const {
    boost::optional<peer_id_t> peer =
        server_config_client->get_server_to_peer_map()->get_key(server_id);
    if (!static_cast<bool>(peer)) {
        return std::vector<peer_id_t>();
    }
    return std::vector<peer_id_t>(1, peer.get());
}

bool table_server_stats_request_t::to_datum(const parsed_stats_t &stats,
                                            const metadata_t &metadata,
                                            server_config_client_t *server_config_client,
                                            table_meta_client_t *table_meta_client,
                                            admin_identifier_format_t admin_format,
                                            ql::datum_t *result_out) const {
    ql::datum_object_builder_t row_builder;
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    id_builder.add(convert_uuid_to_datum(table_id));
    id_builder.add(convert_uuid_to_datum(server_id));
    row_builder.overwrite("id", std::move(id_builder).to_datum());

    ql::datum_t server_identifier;
    if (!convert_connected_server_id_to_datum(server_id, admin_format,
            server_config_client, &server_identifier, nullptr)) {
        return false;
    }
    row_builder.overwrite("server", server_identifier);

    if (!add_table_fields(table_id, metadata, table_meta_client, admin_format,
            &row_builder)) {
        return false;
    }

    auto const &server_it = stats.servers.find(server_id);
    if (server_it == stats.servers.end() ||
        !server_it->second.responsive) {
        row_builder.overwrite("error", ql::datum_t("Timed out. Unable to retrieve stats."));
    } else {
        auto const table_it = server_it->second.tables.find(table_id);
        parsed_stats_t::table_stats_t table_stats;
        if (table_it != server_it->second.tables.end()) {
            table_stats = table_it->second;
        }

        ql::datum_object_builder_t qe_builder;
        ADD_STAT(qe_builder, table_stats, read_docs_per_sec);
        ADD_STAT(qe_builder, table_stats, read_docs_total);
        ADD_STAT(qe_builder, table_stats, written_docs_per_sec);
        ADD_STAT(qe_builder, table_stats, written_docs_total);

        ql::datum_object_builder_t se_cache_builder;
        ADD_STAT(se_cache_builder, table_stats, in_use_bytes);

        ql::datum_object_builder_t se_disk_space_builder;
        ADD_STAT(se_disk_space_builder, table_stats, metadata_bytes);
        ADD_STAT(se_disk_space_builder, table_stats, data_bytes);
        ADD_STAT(se_disk_space_builder, table_stats, garbage_bytes);
        ADD_STAT(se_disk_space_builder, table_stats, preallocated_bytes);

        ql::datum_object_builder_t se_disk_builder;
        ADD_STAT(se_disk_builder, table_stats, read_bytes_per_sec);
        ADD_STAT(se_disk_builder, table_stats, read_bytes_total);
        ADD_STAT(se_disk_builder, table_stats, written_bytes_per_sec);
        ADD_STAT(se_disk_builder, table_stats, written_bytes_total);
        se_disk_builder.overwrite("space_usage", std::move(se_disk_space_builder).to_datum());

        ql::datum_object_builder_t se_builder;
        se_builder.overwrite("cache", std::move(se_cache_builder).to_datum());
        se_builder.overwrite("disk", std::move(se_disk_builder).to_datum());

        row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());
        row_builder.overwrite("storage_engine", std::move(se_builder).to_datum());
    }
    *result_out = std::move(row_builder).to_datum();
    return true;
}
