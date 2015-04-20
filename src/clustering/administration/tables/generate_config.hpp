// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_GENERATE_CONFIG_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_GENERATE_CONFIG_HPP_

#include <map>
#include <string>

#include "clustering/administration/tables/table_metadata.hpp"
#include "rdb_protocol/context.hpp"

class real_reql_cluster_interface_t;
class server_config_client_t;
class table_meta_client_t;

/* Suggests a `table_config_t` for the table. This is the brains behind
`table.reconfigure()`. */
bool table_generate_config(
        /* This is used to get the list of servers with each tag, and to look up the
        names for error messages. */
        server_config_client_t *server_config_client,
        /* The UUID of the table being reconfigured. This can be `nil_uuid()`. */
        namespace_id_t table_id,
        /* This is used to determine where the table's data is currently stored and
        prioritize keeping it there. It's also used to determine where the other tables'
        data is currently stored, so we can put this table's data elsewhere if possible.
        */
        table_meta_client_t *table_meta_client,
        /* `table_generate_config()` will validate `params`, so there's no need for the
        caller to do so. */
        const table_generate_config_params_t &params,
        /* What the new sharding scheme for the table will be. If `table_id` is
        `nil_uuid()` this is unused. */
        const table_shard_scheme_t &shard_scheme,
        signal_t *interruptor,
        std::vector<table_config_t::shard_t> *config_shards_out,
        std::string *error_out);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_GENERATE_CONFIG_HPP_ */

