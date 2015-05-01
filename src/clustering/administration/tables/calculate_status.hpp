// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_

#include <map>
#include <set>
#include <vector>

#include "clustering/administration/servers/config_client.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "protocol_api.hpp"

enum class server_status_t {
    BACKFILLING,
    DISCONNECTED,
    NOTHING,
    READY,
    TRANSITIONING,
    WAITING_FOR_PRIMARY,
    WAITING_FOR_QUORUM
};

struct shard_status_t {
    table_readiness_t readiness;
    std::set<server_id_t> primary_replicas;
    std::map<server_id_t, std::set<server_status_t> > replicas;
};

bool calculate_status(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &config_and_shards,
        signal_t *interruptor,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        table_readiness_t *readiness_out,
        std::vector<shard_status_t> *shard_statuses_out,
        std::string *error_out);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_ */

