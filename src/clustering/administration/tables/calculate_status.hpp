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

/* Sometimes different parts of a shard can be in different states. To avoid confusing
   the user, we only ever present one state. The ordering of the states in this enum are
   used to decide which state to present; we'll always present the state that's furthest
   down the list. This is such that the state can be default-constructed to
   `DISCONNECTED`, updating the state to the maximum of the current and new state. */
enum class server_status_t {
    DISCONNECTED = 0,
    NOTHING,
    READY,
    TRANSITIONING,
    WAITING_FOR_PRIMARY,
    WAITING_FOR_QUORUM,
    BACKFILLING
};

struct shard_status_t {
    table_readiness_t readiness;
    std::set<server_id_t> primary_replicas;
    std::map<server_id_t, server_status_t> replicas;
};

/* Note: `calculate_status()` may leave `shard_statuses_out` empty. This means that not
only is the table unavailable, but we couldn't even figure out which servers were
supposed to be hosting it or how many shards it has. */
void calculate_status(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        table_readiness_t *readiness_out,
        std::vector<shard_status_t> *shard_statuses_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_ */

