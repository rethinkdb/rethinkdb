// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/tables/calculate_status.hpp"

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "concurrency/exponential_backoff.hpp"

bool wait_for_table_readiness(
        const namespace_id_t &table_id,
        table_readiness_t readiness,
        namespace_repo_t *namespace_repo,
        table_meta_client_t *table_meta_client,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t) {
    namespace_interface_access_t ns_if =
        namespace_repo->get_namespace_interface(table_id, interruptor);
    exponential_backoff_t backoff(100, 5000);
    bool immediate = true;
    while (true) {
        if (!table_meta_client->exists(table_id)) {
            throw no_such_table_exc_t();
        }
        bool ok = ns_if.get()->check_readiness(
            std::min(readiness, table_readiness_t::writes),
            interruptor);
        if (ok && readiness == table_readiness_t::finished) {
            try {
                table_meta_client->get_shard_status(
                    table_id, all_replicas_ready_mode_t::INCLUDE_RAFT_TEST, interruptor,
                    nullptr, &ok);
            } catch (const failed_table_op_exc_t &) {
                ok = false;
            }
        }
        if (ok) {
            return immediate;
        }
        immediate = false;
        backoff.failure(interruptor);
    }
}

size_t wait_for_many_tables_readiness(
        const std::set<namespace_id_t> &original_tables,
        table_readiness_t readiness,
        namespace_repo_t *namespace_repo,
        table_meta_client_t *table_meta_client,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    std::set<namespace_id_t> tables = original_tables;
    while (true) {
        bool all_immediate = true;
        for (auto it = tables.begin(); it != tables.end();) {
            try {
                all_immediate &= wait_for_table_readiness(
                    *it, readiness, namespace_repo, table_meta_client, interruptor);
            } catch (const no_such_table_exc_t &) {
                tables.erase(it++);
                continue;
            }
            ++it;
        }
        if (all_immediate) {
            break;
        }
    }
    return tables.size();
}

void get_table_status(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &config,
        namespace_repo_t *namespace_repo,
        table_meta_client_t *table_meta_client,
        server_config_client_t *server_config_client,
        signal_t *interruptor,
        table_status_t *status_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t) {
    status_out->total_loss = false;
    status_out->config = config;

    /* Get the Raft leader for this table. */
    table_meta_client->get_raft_leader(table_id, interruptor, &status_out->raft_leader);

    /* Send the status query to every server for the table. */
    bool all_replicas_ready;
    try {
        table_meta_client->get_shard_status(
            table_id, all_replicas_ready_mode_t::EXCLUDE_RAFT_TEST, interruptor,
            &status_out->server_shards, &all_replicas_ready);
    } catch (const failed_table_op_exc_t &) {
        all_replicas_ready = false;
        status_out->server_shards.clear();
    }

    /* We need to pay special attention to servers that appear in the config but not in
    the status response. There are two possible reasons: they might be disconnected, or
    they might not be hosting the server currently. In the former case we insert them
    into `disconnected`, and in the latter case we set their state to `transitioning` in
    `server_shards`. */
    for (const table_config_t::shard_t &shard : config.config.shards) {
        for (const server_id_t &server : shard.all_replicas) {
            if (status_out->server_shards.count(server) == 0 &&
                    status_out->disconnected.count(server) == 0) {
                if (static_cast<bool>(server_config_client->
                        get_server_to_peer_map()->get_key(server))) {
                    table_shard_status_t status;
                    status.transitioning = true;
                    status_out->server_shards.insert(std::make_pair(server,
                        range_map_t<key_range_t::right_bound_t,
                                    table_shard_status_t>(
                            key_range_t::right_bound_t(store_key_t()),
                            key_range_t::right_bound_t::make_unbounded(),
                            std::move(status))));

                } else {
                    status_out->disconnected.insert(server);
                }
            }
        }
    }

    /* Collect server names. We need the name of every server in the config, every
    server in `server_shards`, and the Raft leader. */
    status_out->server_names = config.server_names;

    std::set<server_id_t> server_ids;
    for (const auto &server_shard : status_out->server_shards) {
        server_ids.insert(server_shard.first);
    }
    if (static_cast<bool>(status_out->raft_leader)) {
        server_ids.insert(status_out->raft_leader.get());
    }

    for (const auto &server_id : server_ids) {
        if (status_out->server_names.names.count(server_id) == 0) {
            server_config_client->get_server_config_map()->read_key(
                server_id,
                [&](const server_config_versioned_t *server_config) {
                    if (server_config != nullptr) {
                        status_out->server_names.names.insert(std::make_pair(
                            server_id,
                            std::make_pair(
                                server_config->version, server_config->config.name)));
                    } else {
                        /* If we can't find the name for one of the servers in the
                        response, then act as though it was disconnected. */
                        status_out->server_names.names.insert(std::make_pair(
                            server_id,
                            std::make_pair(
                                0,
                                name_string_t::guarantee_valid(
                                    "__disconnected_server__"))));
                        status_out->disconnected.insert(server_id);
                        status_out->server_shards.erase(server_id);
                    }
                });
        }
    }

    /* Compute the overall level of table readiness by sending probe queries. */
    if (all_replicas_ready) {
        status_out->readiness = table_readiness_t::finished;
    } else {
        namespace_interface_access_t ns_if =
            namespace_repo->get_namespace_interface(table_id, interruptor);
        if (ns_if.get()->check_readiness(table_readiness_t::writes, interruptor)) {
            status_out->readiness = table_readiness_t::writes;
        } else if (ns_if.get()->check_readiness(table_readiness_t::reads, interruptor)) {
            status_out->readiness = table_readiness_t::reads;
        } else if (ns_if.get()->check_readiness(
                table_readiness_t::outdated_reads, interruptor)) {
            status_out->readiness = table_readiness_t::outdated_reads;
        } else {
            status_out->readiness = table_readiness_t::unavailable;
        }
    }
}

