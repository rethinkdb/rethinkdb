// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_

#include "clustering/administration/namespace_metadata.hpp"

class real_reql_cluster_interface_t;
class server_name_client_t;

class table_reconfigure_params_t {
public:
    int num_shards;
    std::map<name_string_t, int> num_replicas;
    name_string_t director_tag;
};

/* Suggests a `table_config_t` for the table. */
bool table_reconfigure(
        server_name_client_t *name_client,
        /* The UUID of the table being reconfigured. This can be `nil_uuid()`. */
        namespace_id_t table_id,
        /* This is used to run distribution queries. If `table_id` is `nil_uuid()`, this
        can be NULL. */
        real_reql_cluster_interface_t *reql_cluster_interface,
        /* This is used to determine where the table's data is currently stored and
        prioritize keeping it there. If `table_id` is `nil_uuid()`, this can be NULL. */
        clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
            cow_ptr_t<namespaces_directory_metadata_t> > > > directory_view,
        /* Compute this by calling `calculate_server_usage()` on the table configs for
        all of the other tables */
        const std::map<name_string_t, int> &server_usage,

        /* `table_reconfigure()` will validate `params` */
        const table_reconfigure_params_t &params,

        signal_t *interruptor,
        table_config_t *config_out,
        std::string *error_out);

/* `calculate_server_usage()` adds usage statistics for the configuration described in
`config` to the map `usage_inout`. */
void calculate_server_usage(
        const table_config_t &config,
        std::map<name_string_t, int> *server_usage_inout);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_RECONFIGURE_HPP_ */

