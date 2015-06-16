// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_SERVER_NAMES_MANAGER_HPP_
#define CLUSTERING_TABLE_MANAGER_SERVER_NAMES_MANAGER_HPP_

#include "clustering/administration/servers/config_client.hpp"
#include "concurrency/pump_coro.hpp"

class server_name_cache_updater_t {
public:
    server_name_cache_updater_t(
        raft_member_t<table_raft_state_t> *raft,
        server_config_client_t *server_config_client);

private:
    void on_config_change(
        const server_id_t &server_id, const server_config_versioned_t *config);

    void update_blocking(signal_t *interruptor);

    raft_member_t<table_raft_state_t> *raft;

    std::set<server_id_t> dirty_server_ids;
    pump_coro_t update_pumper;

    watchable_map_t<server_id_t, server_config_versioned_t> *server_config_map;
    watchable_map_t<server_id_t, server_config_versioned_t>::all_subs_t
        server_config_map_subs;
};

#endif /* CLUSTERING_TABLE_MANAGER_SERVER_NAMES_MANAGER_HPP_ */

