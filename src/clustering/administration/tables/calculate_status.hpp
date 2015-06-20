// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_

#include "clustering/table_manager/table_meta_client.hpp"
#include "protocol_api.hpp"

class namespace_repo_t;
class signal_t;

/* Blocks until the given table is available at the given level of readiness. If the
table is deleted, throws `no_such_table_exc_t`. The return value is `true` if the table
was ready immediately, or `false` if it was necessary to wait. */
bool wait_for_table_readiness(
    const namespace_id_t &table_id,
    table_readiness_t readiness,
    namespace_repo_t *namespace_repo,
    table_meta_client_t *table_meta_client,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t);

/* Blocks until all of the tables in the given set are either deleted or ready at the
given level of readiness. Returns the number of tables that were ready (as opposed to
deleted.) */
size_t wait_for_many_tables_readiness(
    const std::set<namespace_id_t> &tables,
    table_readiness_t readiness,
    namespace_repo_t *namespace_repo,
    table_meta_client_t *table_meta_client,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

/* `table_status_t` describes the current status of a table, in a format that can be
converted to a `rethinkdb.table_status` entry by `convert_table_status_to_datum()`. */
class table_status_t {
public:
    table_readiness_t readiness;

    /* If `total_loss` is `true`, that indicates that we were unable to access even one
    server for the table, and the fields below are meaningless. */
    bool total_loss;

    /* No server appears in both `server_shards` and `disconnected`. Every server in
    `config` appears in either `server_shards` and `disconnected`, but some servers may
    appear in `server_shards` that aren't in `config`. Every server in any of the three
    appears in `server_names`. */

    table_config_and_shards_t config;
    std::map<server_id_t, range_map_t<key_range_t::right_bound_t,
        table_shard_status_t> > server_shards;
    std::set<server_id_t> disconnected;
    server_name_map_t server_names;
};

/* Fetches the current `table_status_t` for a table that isn't a total loss. */
void get_table_status(
    const namespace_id_t &table_id,
    const table_config_and_shards_t &config,
    namespace_repo_t *namespace_repo,
    table_meta_client_t *table_meta_client,
    server_config_client_t *server_config_client,
    signal_t *interruptor,
    table_status_t *status_out)
    THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_ */

