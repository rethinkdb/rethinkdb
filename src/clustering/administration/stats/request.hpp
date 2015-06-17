// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_STATS_REQUEST_HPP_
#define CLUSTERING_ADMINISTRATION_STATS_REQUEST_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "clustering/administration/metadata.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"

class server_config_client_t;
class table_meta_client_t;

// A `parsed_stats_t` represents all the values that have been gathered from other
// servers in the cluster.  At construction, it parses the given responses to create
// per-server/table stats structures.  These stats can later be used to build multiple
// rows in the `stats` table, without performing more requests.
class parsed_stats_t {
public:
    struct table_stats_t {
        table_stats_t();

        double read_docs_per_sec;
        double read_docs_total;
        double written_docs_per_sec;
        double written_docs_total;
        double in_use_bytes;
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
        server_stats_t();

        bool responsive;
        double queries_per_sec;
        double queries_total;
        double client_connections;
        double clients_active;

        std::map<namespace_id_t, table_stats_t> tables;
    };

    explicit parsed_stats_t(const std::vector<ql::datum_t> &stats);

    // Accumulate a field in all servers
    double accumulate(double server_stats_t::*field) const;

    // Accumulate a field in all tables (across all servers)
    double accumulate(double table_stats_t::*field) const;

    // Accumulate a field in a specific table (across all servers)
    double accumulate_table(const namespace_id_t &table_id,
                            double table_stats_t::*field) const;

    // Accumulate a field in all tables (on a specific server)
    double accumulate_server(const server_id_t &server_id,
                             double table_stats_t::*field) const;

    std::map<server_id_t, server_stats_t> servers;

private:
    // Adds a given stat value to an existing value, useful for summing stats
    // that are spread out across multiple objects.
    void add_perfmon_value(const ql::datum_t &perf,
                           const std::string &key,
                           double *value_out);

    // Stores a given stat value and asserts that the value is the default (0);
    // use `add_perfmon_value` for summing stats from multple sources.
    void store_perfmon_value(const ql::datum_t &perf,
                             const std::string &key,
                             double *value_out);

    void store_shard_values(const ql::datum_t &shard_perf,
                            table_stats_t *stats_out);

    void store_serializer_values(const ql::datum_t &ser_perf,
                                 table_stats_t *);

    void store_query_engine_stats(const ql::datum_t &qe_perf,
                                  server_stats_t *stats_out);

    void store_table_stats(const namespace_id_t &table_id,
                           const ql::datum_t &table_perf,
                           server_stats_t *stats_out);
};

/* A `stats_request_t` encapsulates all the behavior that differentiates between
different types of rows in the `stats` table. First, `get_filter` provides a set of
regular expressions for which stats are needed by the request. Then, `get_peers` will
tell us which peers should be contacted to get the relevant data. Finally, `to_datum`
will take the `parsed_stats_t` obtained from the cross-cluster stats and format it into
the appropriate row format. Each subclass of `stats_request_t` should correspond to a
category within the admin `stats` table. */
class stats_request_t {
public:
    typedef cluster_semilattice_metadata_t metadata_t;

    static std::set<std::vector<std::string> > global_stats_filter();
    static std::vector<peer_id_t> all_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory);

    virtual ~stats_request_t() { }

    // Gets the regex filter to be applied to the perfmons on every server
    virtual std::set<std::vector<std::string> > get_filter() const = 0;

    // Gets the list of servers/peers the request should be sent to
    virtual std::vector<peer_id_t> get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *server_config_client) const = 0;

    // Converts stats from the response into the row format for the stats table
    virtual bool to_datum(const parsed_stats_t &stats,
                          const metadata_t &metadata,
                          server_config_client_t *server_config_client,
                          table_meta_client_t *table_meta_client,
                          admin_identifier_format_t admin_format,
                          ql::datum_t *result_out) const = 0;
};

class cluster_stats_request_t : public stats_request_t {
    static const char *cluster_request_type;
public:
    static const char *get_name() { return cluster_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out);

    cluster_stats_request_t();

    std::set<std::vector<std::string> > get_filter() const;

    std::vector<peer_id_t> get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *server_config_client) const;

    virtual bool to_datum(const parsed_stats_t &stats,
                          const metadata_t &metadata,
                          server_config_client_t *server_config_client,
                          table_meta_client_t *table_meta_client,
                          admin_identifier_format_t admin_format,
                          ql::datum_t *result_out) const;
};

class table_stats_request_t : public stats_request_t {
    static const char *table_request_type;
    const namespace_id_t table_id;
public:

    static const char *get_name() { return table_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out);

    explicit table_stats_request_t(const namespace_id_t &_table_id);

    std::set<std::vector<std::string> > get_filter() const;

    std::vector<peer_id_t> get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *server_config_client) const;

    virtual bool to_datum(const parsed_stats_t &stats,
                          const metadata_t &metadata,
                          server_config_client_t *server_config_client,
                          table_meta_client_t *table_meta_client,
                          admin_identifier_format_t admin_format,
                          ql::datum_t *result_out) const;
};

class server_stats_request_t : public stats_request_t {
    static const char *server_request_type;
    const server_id_t server_id;
public:
    static const char *get_name() { return server_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out);

    explicit server_stats_request_t(const server_id_t &_server_id);

    std::set<std::vector<std::string> > get_filter() const;

    std::vector<peer_id_t> get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *server_config_client) const;

    virtual bool to_datum(const parsed_stats_t &stats,
                          const metadata_t &metadata,
                          server_config_client_t *server_config_client,
                          table_meta_client_t *table_meta_client,
                          admin_identifier_format_t admin_format,
                          ql::datum_t *result_out) const;
};

class table_server_stats_request_t : public stats_request_t {
    static const char *table_server_request_type;
    const namespace_id_t table_id;
    const server_id_t server_id;
public:
    static const char *get_name() { return table_server_request_type; }
    static bool parse(const ql::datum_t &info,
                      scoped_ptr_t<stats_request_t> *request_out);

    table_server_stats_request_t(const namespace_id_t &_table_id,
                                 const server_id_t &_server_id);

    std::set<std::vector<std::string> > get_filter() const;

    std::vector<peer_id_t> get_peers(
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        server_config_client_t *server_config_client) const;

    virtual bool to_datum(const parsed_stats_t &stats,
                          const metadata_t &metadata,
                          server_config_client_t *server_config_client,
                          table_meta_client_t *table_meta_client,
                          admin_identifier_format_t admin_format,
                          ql::datum_t *result_out) const;
};

#endif // CLUSTERING_ADMINISTRATION_STATS_REQUEST_HPP_
