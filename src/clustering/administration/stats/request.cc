#include "clustering/administration/stats/request.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

const char *cluster_stats_request_t::cluster_request_type = "cluster";
const char *server_stats_request_t::server_request_type = "server";
const char *table_stats_request_t::table_request_type = "table";
const char *table_server_stats_request_t::table_server_request_type = "table_server";

parsed_stats_t::parsed_stats_t(const std::map<server_id_t, ql::datum_t> &stats) {
    for (auto const &serv_pair : stats) {
        server_stats_t &serv_stats = servers[serv_pair.first];
        serv_stats = { };

        if (!serv_pair.second.has()) {
            serv_stats.responsive = false;
        } else {
            r_sanity_check(serv_pair.second.get_type() == ql::datum_t::R_OBJECT);
            for (size_t i = 0; i < serv_pair.second.obj_size(); ++i) {
                std::pair<datum_string_t, ql::datum_t> perf_pair =
                    serv_pair.second.get_pair(i);
                if (perf_pair.first == "query_engine") {
                    add_query_engine_stats(perf_pair.second, &serv_stats);
                } else {
                    namespace_id_t table_id;
                    bool res = str_to_uuid(perf_pair.first.to_std(), &table_id);
                    if (res) {
                        add_table_stats(table_id, perf_pair.second, &serv_stats);
                    }
                }
            }
        }
    }
}

void parsed_stats_t::add_perfmon_value(const ql::datum_t &perf,
                                       const std::string &key,
                                       double *value_out) {
    ql::datum_t v = perf.get_field(key.c_str(), ql::throw_bool_t::NOTHROW);
    if (v.has()) {
        r_sanity_check(v.get_type() == ql::datum_t::R_NUM);
        *value_out += v.as_num();   
    }
}

void parsed_stats_t::add_shard_values(const ql::datum_t &shard_perf,
                                      table_stats_t *stats_out) {
    r_sanity_check(shard_perf.get_type() == ql::datum_t::R_OBJECT);
    for (size_t i = 0; i < shard_perf.obj_size(); ++i) {
        std::pair<datum_string_t, ql::datum_t> pair = shard_perf.get_pair(i);
        if (pair.first.to_std().find("shard_") == 0) {
            r_sanity_check(pair.second.get_type() == ql::datum_t::R_OBJECT);
            ql::datum_t pkey_stats = pair.second.get_field("btree-primary", ql::throw_bool_t::NOTHROW);
            if (pkey_stats.has()) {
                r_sanity_check(pkey_stats.get_type() == ql::datum_t::R_OBJECT);
                add_perfmon_value(pkey_stats, "keys_read", &stats_out->read_docs_per_sec);
                add_perfmon_value(pkey_stats, "keys_set", &stats_out->written_docs_per_sec);
                add_perfmon_value(pkey_stats, "total_keys_read", &stats_out->read_docs_total);
                add_perfmon_value(pkey_stats, "total_keys_read", &stats_out->written_docs_total);
            }
        }
    }
}

void parsed_stats_t::add_serializer_values(const ql::datum_t &ser_perf,
                                           table_stats_t *) {
    r_sanity_check(ser_perf.get_type() == ql::datum_t::R_OBJECT);
    // TODO: implement after modifying the perfmons to be closer to the stats we want
}

void parsed_stats_t::add_query_engine_stats(const ql::datum_t &qe_perf,
                                            server_stats_t *stats_out) {
    r_sanity_check(qe_perf.get_type() == ql::datum_t::R_OBJECT);
    add_perfmon_value(qe_perf, "queries_per_sec", &stats_out->queries_per_sec);
    add_perfmon_value(qe_perf, "queries_total", &stats_out->queries_total);
    add_perfmon_value(qe_perf, "client_connections", &stats_out->client_connections);
}

void parsed_stats_t::add_table_stats(const namespace_id_t &table_id,
                                     const ql::datum_t &table_perf,
                                     server_stats_t *stats_out) {
    r_sanity_check(table_perf.get_type() == ql::datum_t::R_OBJECT);
    ql::datum_t sers_perf = table_perf.get_field("serializers", ql::throw_bool_t::NOTHROW);
    if (sers_perf.has()) {
        r_sanity_check(sers_perf.get_type() == ql::datum_t::R_OBJECT);
        table_stats_t &table_stats_out = stats_out->tables[table_id];
        table_stats_out = { };
        all_table_ids.insert(table_id);

        add_shard_values(sers_perf, &table_stats_out);

        ql::datum_t sub_sers_perf = sers_perf.get_field("serializer", ql::throw_bool_t::NOTHROW);
        if (sub_sers_perf.has()) {
            add_serializer_values(sub_sers_perf, &table_stats_out);
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

void add_table_fields(const namespace_id_t &table_id,
                      const cluster_semilattice_metadata_t &metadata,
                      ql::datum_object_builder_t *builder) {
    // TODO: maybe drop the stats if the table wasn't found
    std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t> >
        ::const_iterator table_it;
    if (!search_const_metadata_by_uuid(&metadata.rdb_namespaces->namespaces,
                                      table_id, &table_it)) {
        // TODO: leave out db_id and db_name?
        builder->overwrite("table_name", ql::datum_t("__deleted_table__"));
    } else {
        const database_id_t &db_id = table_it->second.get_ref().database.get_ref();
        auto db_it = metadata.databases.databases.find(db_id);
        if (db_it == metadata.databases.databases.end() ||
            db_it->second.is_deleted()) {
            builder->overwrite("db_name", ql::datum_t("__deleted_database__"));
        } else {
            builder->overwrite("db_name",
                convert_name_to_datum(db_it->second.get_ref().name.get_ref()));
        }

        builder->overwrite("table_name",
            convert_name_to_datum(table_it->second.get_ref().name.get_ref()));
        builder->overwrite("db_id", convert_uuid_to_datum(db_id));
    }

    builder->overwrite("table_id", convert_uuid_to_datum(table_id));
}

void add_server_fields(const server_id_t &server_id,
                       const cluster_semilattice_metadata_t &metadata,
                       ql::datum_object_builder_t *builder) {
    auto server_it = metadata.servers.servers.find(server_id);
    if (server_it == metadata.servers.servers.end() ||
        server_it->second.is_deleted()) {
        builder->overwrite("server_name", ql::datum_t("__deleted_server__"));
    } else {
        builder->overwrite("server_name",
            convert_name_to_datum(server_it->second.get_ref().name.get_ref()));
    }

    builder->overwrite("server_id", convert_uuid_to_datum(server_id));
}


std::set<std::string> stats_request_t::global_stats_filter() {
    return std::set<std::string>({ "query_engine/.*", ".*/serializers/.*" });
}

std::vector<std::pair<server_id_t, peer_id_t> > stats_request_t::all_peers(
        server_name_client_t *name_client) {
    std::vector<std::pair<server_id_t, peer_id_t> > res;
    for (auto const &pair : name_client->get_server_id_to_peer_id_map()->get()) {
        res.push_back(pair);
    }
    return res;
}

bool cluster_stats_request_t::parse(const ql::datum_t &info,
                                    scoped_ptr_t<stats_request_t> *request_out,
                                    std::string *error_out) {
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 1 || info.get(0).as_str() != get_name()) {
        *error_out =
            strprintf("A '%s' stats request should have the format ['%s'].",
                      get_name(), get_name());
        return false;
    }
    request_out->init(new cluster_stats_request_t());
    return true;
}

cluster_stats_request_t::cluster_stats_request_t() { }

std::set<std::string> cluster_stats_request_t::get_filter() const {
    return std::set<std::string>({ "query_engine/queries_per_sec",
                                   ".*/serializers/shard[0-9]+/keys_.*" });
}

bool cluster_stats_request_t::get_peers(
        server_name_client_t *name_client,
        std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
        UNUSED std::string *error_out) const {
    *peers_out = all_peers(name_client);
    return true;
}

ql::datum_t cluster_stats_request_t::to_datum(const parsed_stats_t &stats,
                                              UNUSED const metadata_t &metadata) const {
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));

    ql::datum_object_builder_t qe_builder;
    // TODO: macro magic?
    qe_builder.overwrite("queries_per_sec", ql::datum_t(
        stats.accumulate(&parsed_stats_t::server_stats_t::queries_per_sec)));
    qe_builder.overwrite("read_docs_per_sec", ql::datum_t(
        stats.accumulate(&parsed_stats_t::table_stats_t::read_docs_per_sec)));
    qe_builder.overwrite("written_docs_per_sec", ql::datum_t(
        stats.accumulate(&parsed_stats_t::table_stats_t::written_docs_per_sec)));

    ql::datum_object_builder_t builder;
    builder.overwrite("id", std::move(id_builder).to_datum());
    builder.overwrite("query_engine", std::move(qe_builder).to_datum());
    return std::move(builder).to_datum();
}

bool table_stats_request_t::parse(const ql::datum_t &info,
                                  scoped_ptr_t<stats_request_t> *request_out,
                                  std::string *error_out) {
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 1 || info.get(0).as_str() != get_name()) {
        *error_out = strprintf("A '%s' stats request should have the format "
                               "['%s', <TABLE_ID>].", get_name(), get_name());
        return false;
    }

    namespace_id_t t;
    if (!convert_uuid_from_datum(info.get(1), &t, error_out)) {
        return false;
    }
    request_out->init(new table_stats_request_t(t));
    return true;
}

table_stats_request_t::table_stats_request_t(const namespace_id_t &_table_id) :
    table_id(_table_id) { }

std::set<std::string> table_stats_request_t::get_filter() const {
    return std::set<std::string>({ 
        uuid_to_str(table_id) + "/serializers/shard[0-9]+/keys_.*" });
}

bool table_stats_request_t::get_peers(
        server_name_client_t *name_client,
        std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
        UNUSED std::string *error_out) const {
    *peers_out = all_peers(name_client);
    return true;
}

ql::datum_t table_stats_request_t::to_datum(const parsed_stats_t &stats,
                                            const metadata_t &metadata) const {
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    id_builder.add(convert_uuid_to_datum(table_id));

    ql::datum_object_builder_t qe_builder;
    qe_builder.overwrite("read_docs_per_sec", ql::datum_t(stats.accumulate_table(table_id,
        &parsed_stats_t::table_stats_t::read_docs_per_sec)));
    qe_builder.overwrite("written_docs_per_sec", ql::datum_t(stats.accumulate_table(table_id,
        &parsed_stats_t::table_stats_t::written_docs_per_sec)));

    ql::datum_object_builder_t row_builder;
    row_builder.overwrite("id", std::move(id_builder).to_datum());
    row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());
    add_table_fields(table_id, metadata, &row_builder);
    return std::move(row_builder).to_datum();
}

bool server_stats_request_t::parse(const ql::datum_t &info,
                                   scoped_ptr_t<stats_request_t> *request_out,
                                   std::string *error_out) {
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 1 || info.get(0).as_str() != get_name()) {
        *error_out = strprintf("A '%s' stats request should have the format "
                               "['%s', <SERVER_ID>].", get_name(), get_name());
        return false;
    }

    server_id_t s;
    if (!convert_uuid_from_datum(info.get(2), &s, error_out)) {
        return false;
    }
    request_out->init(new server_stats_request_t(s));
    return true;
}

server_stats_request_t::server_stats_request_t(const server_id_t &_server_id) :
    server_id(_server_id) { }

    std::set<std::string> server_stats_request_t::get_filter() const {
        return std::set<std::string>({ "query_engine/.*",
                                     ".*/serializers/shard[0-9]+/keys_.*" });
    }

bool server_stats_request_t::get_peers(
        server_name_client_t *name_client,
        std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
        std::string *error_out) const {
    boost::optional<peer_id_t> peer = name_client->get_peer_id_for_server_id(server_id);
    if (!static_cast<bool>(peer)) {
        // TODO: make this use the same error as a timeout?
        *error_out = "Server not available.";
        return false;
    }

    *peers_out = std::vector<std::pair<server_id_t, peer_id_t> >(1,
        std::make_pair(server_id, peer.get()));
    return true;
}

ql::datum_t server_stats_request_t::to_datum(const parsed_stats_t &stats,
                                             const metadata_t &metadata) const {
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    id_builder.add(convert_uuid_to_datum(server_id));

    auto const &server_it = stats.servers.find(server_id);
    r_sanity_check(server_it != stats.servers.end());
    const parsed_stats_t::server_stats_t &server_stats = server_it->second;

    ql::datum_object_builder_t row_builder;
    if (!server_stats.responsive) {
        row_builder.overwrite("error", ql::datum_t("Timed out. Unable to retrieve stats."));
    } else {
        ql::datum_object_builder_t qe_builder;
        qe_builder.overwrite("client_connections",
                             ql::datum_t(server_stats.client_connections));
        qe_builder.overwrite("queries_per_sec",
                             ql::datum_t(server_stats.queries_per_sec));
        qe_builder.overwrite("queries_total",
                             ql::datum_t(server_stats.queries_total));
        qe_builder.overwrite("read_docs_per_sec",
            ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::read_docs_per_sec)));
        qe_builder.overwrite("read_docs_total",
            ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::read_docs_total)));
        qe_builder.overwrite("written_docs_per_sec",
            ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::written_docs_per_sec)));
        qe_builder.overwrite("written_docs_total",
            ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::written_docs_total)));
        row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());
    }

    row_builder.overwrite("id", std::move(id_builder).to_datum());
    add_server_fields(server_id, metadata, &row_builder);
    return std::move(row_builder).to_datum();
}


bool table_server_stats_request_t::parse(const ql::datum_t &info,
                                         scoped_ptr_t<stats_request_t> *request_out,
                                         std::string *error_out) {
    r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
    if (info.arr_size() != 3 || info.get(0).as_str() != get_name()) {
        *error_out = strprintf("A '%s' stats request should have the format "
                               "['%s', <TABLE_ID>, <SERVER_ID>].",
                               get_name(), get_name());
        return false;
    }

    namespace_id_t t;
    server_id_t s;
    if (!convert_uuid_from_datum(info.get(1), &t, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(info.get(2), &s, error_out)) {
        return false;
    }
    request_out->init(new table_server_stats_request_t(t, s));
    return true;
}

table_server_stats_request_t::table_server_stats_request_t(const namespace_id_t &_table_id,
                                                           const server_id_t &_server_id) :
    table_id(_table_id), server_id(_server_id) { }

std::set<std::string> table_server_stats_request_t::get_filter() const {
    return std::set<std::string>({ uuid_to_str(table_id) + "/serializers/.*" });
}

bool table_server_stats_request_t::get_peers(server_name_client_t *name_client,
               std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
               std::string *error_out) const {
    boost::optional<peer_id_t> peer = name_client->get_peer_id_for_server_id(server_id);
    if (!static_cast<bool>(peer)) {
        // TODO: make this use the same error as a timeout?
        *error_out = "Server not available.";
        return false;
    }

    *peers_out = std::vector<std::pair<server_id_t, peer_id_t> >(1,
        std::make_pair(server_id, peer.get()));
    return true;
}

ql::datum_t table_server_stats_request_t::to_datum(const parsed_stats_t &stats,
                                                   const metadata_t &metadata) const {
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(get_name()));
    id_builder.add(convert_uuid_to_datum(table_id));
    id_builder.add(convert_uuid_to_datum(server_id));

    ql::datum_object_builder_t row_builder;
    auto const server_it = stats.servers.find(server_id);
    r_sanity_check(server_it != stats.servers.end());
    if (!server_it->second.responsive) {
        row_builder.overwrite("error", ql::datum_t("Timed out. Unable to retrieve stats."));
    } else {
        auto const table_it = server_it->second.tables.find(table_id);
        parsed_stats_t::table_stats_t table_stats = { };
        if (table_it != server_it->second.tables.end()) {
            table_stats = table_it->second;
        }

        ql::datum_object_builder_t qe_builder;
        qe_builder.overwrite("read_docs_per_sec", ql::datum_t(table_stats.read_docs_per_sec));
        qe_builder.overwrite("read_docs_total", ql::datum_t(table_stats.read_docs_total));
        qe_builder.overwrite("written_docs_per_sec", ql::datum_t(table_stats.written_docs_per_sec));
        qe_builder.overwrite("written_docs_total", ql::datum_t(table_stats.written_docs_total));

        ql::datum_object_builder_t se_cache_builder;
        se_cache_builder.overwrite("in_use_bytes", ql::datum_t(table_stats.in_use_bytes));

        ql::datum_object_builder_t se_disk_space_builder;
        se_disk_space_builder.overwrite("metadata_bytes", ql::datum_t(table_stats.metadata_bytes));
        se_disk_space_builder.overwrite("data_bytes", ql::datum_t(table_stats.data_bytes));
        se_disk_space_builder.overwrite("garbage_bytes", ql::datum_t(table_stats.garbage_bytes));
        se_disk_space_builder.overwrite("preallocated_bytes", ql::datum_t(table_stats.preallocated_bytes));

        ql::datum_object_builder_t se_disk_builder;
        se_disk_builder.overwrite("read_bytes_per_sec", ql::datum_t(table_stats.read_bytes_per_sec));
        se_disk_builder.overwrite("read_bytes_total", ql::datum_t(table_stats.read_bytes_total));
        se_disk_builder.overwrite("written_bytes_per_sec", ql::datum_t(table_stats.written_bytes_per_sec));
        se_disk_builder.overwrite("written_bytes_total", ql::datum_t(table_stats.written_bytes_total));
        se_disk_builder.overwrite("space_usage", std::move(se_disk_space_builder).to_datum());

        ql::datum_object_builder_t se_builder;
        se_builder.overwrite("cache", std::move(se_cache_builder).to_datum());
        se_builder.overwrite("disk", std::move(se_disk_builder).to_datum());

        row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());
        row_builder.overwrite("storage_engine", std::move(se_builder).to_datum());
    }

    row_builder.overwrite("id", std::move(id_builder).to_datum());
    add_server_fields(server_id, metadata, &row_builder);
    add_table_fields(table_id, metadata, &row_builder);
    return std::move(row_builder).to_datum();
}
