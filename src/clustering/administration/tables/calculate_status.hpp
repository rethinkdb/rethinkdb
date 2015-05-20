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
   down the list.

   `backfilling` has a performance impact so we should display it whenever it's
   happening. `waiting_for_quorum` can cause another machine to be `waiting_for_primary`,
   so we should prioritize displaying `waiting_for_quorum` to make it clear why the
   primary is stuck. `transitioning` is the least informative error condition, so we
   shouldn't display it at all unless it's the only thing keeping us from being ready.
   A replica in the `nothing` state is hidden, and last of all, the `disconnected` state
   can never occur in conjunction with any other state, so it doesn't have a meaningful
   position in the list. */
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
        std::vector<shard_status_t> *shard_statuses_out,
        std::map<server_id_t, std::pair<uint64_t, name_string_t> > *server_names_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_CALCULATE_STATUS_HPP_ */

