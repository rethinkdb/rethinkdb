#include "clustering/administration/stats/stats_backend.hpp"

#include "clustering/administration/datum_adapter.hpp"

class parsed_stats_t {
public:
    struct table_stats_t {
        double read_docs_per_sec;
        double read_docs_total;
        double written_docs_per_sec;
        double written_docs_total;
        double cache_size;
        double metadata_bytes;
        double data_bytes;
        double garbage_bytes;
        double preallocated_bytes;
        double read_bytes_per_sec;
        double read_bytes_total;
        double written_bytes_per_sec;
        double written_bytes_total;
    };

    struct server_stats_t {
        bool responsive;
        double queries_per_sec;
        double queries_total;
        double client_connections;

        std::map<namespace_id_t, table_stats_t> tables;
    };

    parsed_stats_t(const std::map<server_id_t, ql::datum_t> &stats) {
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

    void add_perfmon_value(const ql::datum_t &perf,
                           const std::string &key,
                           double *value_out) {
        ql::datum_t v = perf.get_field(key.c_str(), ql::throw_bool_t::NOTHROW);
        if (v.has()) {
            r_sanity_check(v.get_type() == ql::datum_t::R_NUM);
            *value_out += v.as_num();   
        }
    }

    void add_shard_values(const ql::datum_t &shard_perf,
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

    void add_serializer_values(const ql::datum_t &ser_perf,
                               table_stats_t *) {
        r_sanity_check(ser_perf.get_type() == ql::datum_t::R_OBJECT);
        // TODO: implement after modifying the perfmons to be closer to the stats we want
    }

    void add_query_engine_stats(const ql::datum_t &qe_perf,
                                server_stats_t *stats_out) {
        r_sanity_check(qe_perf.get_type() == ql::datum_t::R_OBJECT);
        add_perfmon_value(qe_perf, "queries_per_sec", &stats_out->queries_per_sec);
        add_perfmon_value(qe_perf, "queries_total", &stats_out->queries_total);
        add_perfmon_value(qe_perf, "client_connections", &stats_out->client_connections);
    }

    void add_table_stats(const namespace_id_t &table_id,
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

    // Accumulate a field in all servers
    double accumulate(double server_stats_t::*field) const {
        double res = 0;
        for (auto const &pair : servers) {
            res += pair.second.*field;
        }
        return res;
    }

    // Accumulate a field in all tables (across all servers)
    double accumulate(double table_stats_t::*field) const {
        double res = 0;
        for (auto const &server_pair : servers) {
            for (auto const &table_pair : server_pair.second.tables) {
                res += table_pair.second.*field;
            }
        }
        return res;
    }

    // Accumulate a field in a specific table (across all servers)
    double accumulate_table(const namespace_id_t &table_id,
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

    // Accumulate a field in all tables (on a specific server)
    double accumulate_server(const server_id_t &server_id,
                             double table_stats_t::*field) const {
        double res = 0;
        auto const server_it = servers.find(server_id);
        r_sanity_check(server_it != servers.end());
        for (auto const &table_pair : server_it->second.tables) {
            res += table_pair.second.*field;
        }
        return res;
    }

    std::map<server_id_t, server_stats_t> servers;
    std::set<namespace_id_t> all_table_ids;
};

class stats_request_t {
public:
    typedef cluster_semilattice_metadata_t metadata_t;

    static std::set<std::string> global_stats_filter() {
        return std::set<std::string>({ "query_engine/.*", ".*/serializers/.*" });
    }

    static std::vector<std::pair<server_id_t, peer_id_t> > all_peers(
            server_name_client_t *name_client) {
        std::vector<std::pair<server_id_t, peer_id_t> > res;
        for (auto const &pair : name_client->get_server_id_to_peer_id_map()->get()) {
            res.push_back(pair);
        }
        return res;
    }

    virtual ~stats_request_t() { }

    virtual std::set<std::string> get_filter() const = 0;

    virtual bool get_peers(server_name_client_t *name_client,
                           std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                           std::string *error_out) const = 0;

    virtual ql::datum_t to_datum(const parsed_stats_t &stats,
                                 const metadata_t &metadata) const = 0;
};

class cluster_stats_request_t : public stats_request_t {
    static const char *cluster_request_type = "cluster";
public:
    static const char *get_name() { return cluster_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out,
                      std::string *error_out) {
        r_sanity_check(info.get_type() == ql::datum_t::R_ARRAY);
        if (info.arr_size() != 1 || info.get(0).as_str() != get_name()) {
            *error_out =
                strprintf("A '%s' stats request should have the format ['%s'].",
                          get_name(), get_name());
            return false;
        }
        request_out->reset(new cluster_stats_request_t());
        return true;
    }

    cluster_stats_request_t() { }

    std::set<std::string> get_filter() const {
        return std::set<std::string>({ "query_engine/queries_per_sec",
                                       ".*/serializers/shard[0-9]+/keys_.*" });
    }

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const {
        *peers_out = all_peers(name_client);
        return true;
    }

    ql::datum_t to_datum(const parsed_stats_t &stats,
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
};

class table_stats_request_t : public stats_request_t {
    static const char *table_request_type = "table";
public:
    table_stats_request_t(const namespace_id_t &_table_id) :
        table_id(_table_id) { }

    static const char *get_name() { return table_request_type; }
    static bool parse(const ql::datum_t &info,
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
        request_out->reset(new table_stats_request_t(t));
        return true;
    }

    std::set<std::string> get_filter() const {
        return std::set<std::string>({ 
            uuid_to_str(table_id) + "/serializers/shard[0-9]+/keys_.*" });
    }

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const {
        *peers_out = all_peers(name_client);
        return true;
    }

    ql::datum_t to_datum(const parsed_stats_t &stats,
                         const metadata_t &metadata) const {
        ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
        id_builder.add(ql::datum_t(get_name()));
        id_builder.add(convert_uuid_to_datum(table_id));

        ql::datum_object_builder_t qe_builder;
        qe_builder.overwrite("read_docs_per_sec", ql::datum_t(stats.accumulate_table(table_id,
            &parsed_stats_t::table_stats_t::read_docs_per_sec)));
        qe_builder.overwrite("written_docs_per_sec", ql::datum_t(stats.accumulate_table(table_id,
            &parsed_stats_t::table_stats_t::written_docs_per_sec)));

        ql::datum_object_builder_t builder;
        builder.overwrite("id", std::move(id_builder).to_datum());
        builder.overwrite("db_id", );
        builder.overwrite("db_name", );
        builder.overwrite("table_id", );
        builder.overwrite("table_name", );
        builder.overwrite("query_engine", std::move(qe_builder).to_datum());
        return std::move(builder).to_datum();
    }

private:
    const namespace_id_t table_id;
};

class server_stats_request_t : public stats_request_t {
    static const char *server_request_type = "server";
public:
    server_stats_request_t(const server_id_t &_server_id) :
        server_id(_server_id) { }

    static const char *get_name() { return server_request_type; }
    static bool parse(const ql::datum_t &info,
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
        request_out->reset(new server_stats_request_t(s));
        return true;
    }

    std::set<std::string> get_filter() const {
        return std::set<std::string>({ "query_engine/.*",
                                       ".*/serializers/shard[0-9]+/keys_.*" });
    }

    bool get_peers(server_name_client_t *name_client,
                   std::vector<std::pair<server_id_t, peer_id_t> > *peers_out,
                   std::string *error_out) const {
        boost::optional<peer_id_t> peer = name_client->get_peer_id_for_server_id(server_id);
        if (!static_cast<bool(peer)) {
            // TODO: make this use the same error as a timeout?
            *error_out = "Server not available.";
            return false;
        }

        *peers_out = std::vector<std::pair<server_id_t, peer_id_t> >(1,
            std::make_pair(server_id, peer.get()));
        return true;
    }

    ql::datum_t to_datum(const parsed_stats_t &stats,
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
            qe_builder.overwrite("client_connections", ql::datum_t(server_stats.client_connections));
            qe_builder.overwrite("queries_per_sec", ql::datum_t(server_stats.queries_per_sec));
            qe_builder.overwrite("queries_total", ql::datum_t(server_stats.queries_total));
            qe_builder.overwrite("read_docs_per_sec", ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::read_docs_per_sec)));
            qe_builder.overwrite("read_docs_total", ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::read_docs_total)));
            qe_builder.overwrite("written_docs_per_sec", ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::written_docs_per_sec)));
            qe_builder.overwrite("written_docs_total", ql::datum_t(stats.accumulate_server(server_id,
                &parsed_stats_t::table_stats_t::written_docs_total)));
            row_builder.overwrite("query_engine", std::move(qe_builder).to_datum());
        }

        row_builder.overwrite("id", std::move(id_builder).to_datum());
        row_builder.overwrite("server_id", );
        row_builder.overwrite("server_name", );
        return std::move(row_builder).to_datum();
    }

private:
    const server_id_t server_id;
};

class table_server_stats_request_t : public stats_request_t {
    static const char *table_server_request_type = "table_server";
public:
    table_server_stats_request_t(const namespace_id_t &_table_id,
                                 const server_id_t &_server_id) :
        table_id(_table_id), server_id(_server_id) { }

    static const char *get_name() { return table_server_request_type; }
    static bool parse(const ql::datum_t &info,
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
        request_out->reset(new table_server_stats_request_t(t, s));
        return true;
    }

    std::set<std::string> get_filter() const {
        return std::set<std::string>({ uuid_to_str(table_id) + "/serializers/.*" });
    }

    bool get_peers(server_name_client_t *name_client,
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

    ql::datum_t to_datum(const parsed_stats_t &stats,
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
            auto const table_it = server_pair->second.tables.find(table_id);
            table_stats_t table_stats = { };
            if (table_it != server_pair->second.tables.end()) {
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
        row_builder.overwrite("server_id", );
        row_builder.overwrite("server_name", );
        row_builder.overwrite("db_id", );
        row_builder.overwrite("db_name", );
        row_builder.overwrite("table_id", );
        row_builder.overwrite("table_name", );
        return std::move(row_builder).to_datum();
    }

private:
    const namespace_id_t table_id;
    const server_id_t server_id;
};


stats_artificial_table_backend_t::stats_artificial_table_backend_t(
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > >
                &_directory_view,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        server_name_client_t *_name_client,
        mailbox_manager_t *_mailbox_manager) :
    directory_view(_directory_view),
    cluster_sl_view(_cluster_sl_view),
    name_client(_name_client),
    mailbox_manager(_mailbox_manager) { }

std::string stats_artificial_table_backend_t::get_primary_key_name() {
    return std::string("id");
}

// TODO: auto_drainer, keepalive
void stats_artificial_table_backend_t::get_peer_stats(
        const peer_id_t &peer,
        const std::set<std::string> &filter,
        ql::datum_t *result_out,
        signal_t *interruptor) {
    // Loop up peer in directory - find get stats mailbox
    get_stats_mailbox_address_t request_addr;
    directory_view->apply_read(
        [&](change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *directory) {
            auto const peer_it = directory->get_inner().find(peer);
            if (peer_it != directory->get_inner().end()) {
                request_addr = peer_it->second.get_stats_mailbox_address;
            }
        });

    if (!request_addr.is_nil()) {
        // Create response mailbox
        cond_t done;
        mailbox_t<void(ql::datum_t)> return_mailbox(mailbox_manager,
            [&](ql::datum_t stats) {
                *result_out = stats;
                done.pulse_if_not_already_pulsed();
            });

        // Send request
        send(mailbox_manager, request_addr, return_mailbox.get_address(), filter);

        // Allow 5 seconds - on timeout return without setting result
        signal_timer_t timer_interruptor;
        timer_interruptor.start(5000);
        wait_any_t combined_interruptor(interruptor, &timer_interruptor);
        wait_interruptible(&done, &combined_interruptor);
    }
}

void stats_artificial_table_backend_t::perform_stats_request(
        const std::vector<std::pair<server_id_t, peer_id_t> > &peers,
        const std::set<std::string> &filter,
        std::map<server_id_t, ql::datum_t> *results_out,
        signal_t *interruptor) {
    pmap(peers.size(),
        [&](int index) {
            get_peer_stats(peers[index].second, filter,
                           &(*results_out)[peers[index].first], interruptor);
        });
}

bool stats_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        UNUSED std::string *error_out) {
    assert_thread();
    std::set<std::string> filter = stats_request_t::global_stats_filter();
    std::vector<std::pair<server_id_t, peer_id_t> > peers =
        stats_request_t::all_peers(name_client);

    // Save the metadata from when we sent the requests to avoid race conditions
    // TODO: is name client up-to-date?
    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();

    std::map<server_id_t, ql::datum_t> result_map;
    perform_stats_request(peers, filter, &result_map, interruptor);
    parsed_stats_t parsed_stats(result_map);

    // Start building results
    rows_out->clear();
    rows_out->push_back(cluster_stats_request_t().to_datum(result_map));

    for (auto const &pair : parsed_stats.servers) {
        rows_out->push_back(server_stats_request_t(pair.first).to_datum(parsed_stats, metadata));
    }
    
    for (auto const &table_id : parsed_stats.all_table_ids) {
        rows_out->push_back(table_stats_request_t(table_id).to_datum(parsed_stats, metadata));
    }

    for (auto const &pair : parsed_stats.servers) {
        for (auto const &table_id : parsed_stats.all_table_ids) {
            rows_out->push_back(
                table_server_stats_request_t(table_id, pair.first).to_datum(parsed_stats, metadata));
        }
    }

    return results;
}

template <class T>
bool parse_stats_request(const ql::datum_t &info,
                         scoped_ptr_t<stats_request_t> *request_out,
                         std::string *error_out) {
    if (request_type.as_str() == T::get_name()) {
        if (!T::parse(info, request_out, error_out)) {
            return false;
        }
    }
    return true;
}

bool stats_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    assert_thread();
    // TODO: better error messages
    if (primary_key.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "A stats request needs to be an array.";
        return false;
    }
    if (primary_key.arr_size() == 0) {
        *error_out = "The stats request is empty."
        return false;
    }

    ql::datum_t request_type = primary_key.get(0);
    if (request_type.get_type() != ql::datum_t::R_STR) {
        *error_out = "The first element of a stats request must be a string.";
        return false;
    }

    scoped_ptr_t<stats_request_t> request;
    if (!parse_stats_request<cluster_stats_request_t>(info, &request, error_out) ||
        !parse_stats_request<table_stats_request_t>(info, &request, error_out) ||
        !parse_stats_request<server_stats_request_t>(info, &request, error_out) ||
        !parse_stats_request<table__serverstats_request_t>(info, &request, error_out)) {
        return false;
    }

    if (!request.has()) {
        *error_out = "Unknown stats request type.";
        return false;
    }

    std::vector<std::pair<server_id_t, peer_id_t> > peers;
    std::map<server_id_t, ql::datum_t> results_map;
    if (!request->get_peers(name_client, &peers, error_out)) {
        return false;
    }

    // Save the metadata from when we sent the requests to avoid race conditions
    // TODO: is name client up-to-date?
    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();

    perform_stats_request(peers, request->get_filter(), &results_map, interruptor);
    parsed_stats_t parsed_stats(results_map);
    *row_out = request->to_datum(parsed_stats, metadata);
    return true;
}

bool stats_artificial_table_backend_t::write_row(
        ql::datum_t primary_key,
        bool pkey_was_autogenerated,
        ql::datum_t *new_value_inout,
        signal_t *interruptor,
        std::string *error_out) {
    assert_thread();
    // TODO: error message
    *error_out = "Cannot write to the stats table."
    return false;
}
