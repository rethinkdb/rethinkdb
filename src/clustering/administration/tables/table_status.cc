// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_status.hpp"

#include <algorithm>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/administration/tables/calculate_status.hpp"
#include "clustering/table_contract/executor/exec_primary.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

table_status_artificial_table_backend_t::table_status_artificial_table_backend_t(
        boost::shared_ptr<semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        server_config_client_t *_server_config_client,
        namespace_repo_t *_namespace_repo,
        table_meta_client_t *_table_meta_client,
        admin_identifier_format_t _identifier_format) :
    common_table_artificial_table_backend_t(
        _semilattice_view, _table_meta_client, _identifier_format),
    server_config_client(_server_config_client),
    namespace_repo(_namespace_repo) { }

table_status_artificial_table_backend_t::~table_status_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

ql::datum_t convert_replica_status_to_datum(
        const server_id_t &server_id,
        const char *status,
        admin_identifier_format_t identifier_format,
        const server_name_map_t &server_names) {
    ql::datum_object_builder_t replica_builder;
    replica_builder.overwrite("server", convert_name_or_uuid_to_datum(
        server_names.get(server_id), server_id, identifier_format));
    replica_builder.overwrite("state", ql::datum_t(status));
    return std::move(replica_builder).to_datum();
}

ql::datum_t convert_shard_status_to_datum(
        const table_config_t::shard_t &shard,
        const std::map<server_id_t, table_status_response_t::shard_status_t> &states,
        const std::set<server_id_t> &disconnected,
        admin_identifier_format_t identifier_format,
        const server_name_map_t &server_names) {
    ql::datum_array_builder_t primary_replicas_builder(
        ql::configured_limits_t::unlimited);
    ql::datum_array_builder_t replicas_builder(
        ql::configured_limits_t::unlimited);

    for (const auto &pair : states) {
        if (!pair.second.primary && !pair.second.secondary &&
                !pair.second.transitioning) {
            continue;
        }
        if (pair.second.primary) {
            primary_replicas_builder.add(convert_name_or_uuid_to_datum(
                server_names.get(pair.first), pair.first, identifier_format));
        }
        const char *status;
        if (pair.second.backfilling) {
            status = "backfilling";
        } else if (pair.second.need_quorum) {
            status = "waiting_for_quorum";
        } else if (pair.second.need_primary) {
            status = "waiting_for_primary";
        } else if (pair.second.transitioning) {
            status = "transitioning";
        } else {
            status = "ready";
        }
        replicas_builder.add(convert_replica_status_to_datum(
            pair.first, status, identifier_format, server_names));
    }

    for (const server_id_t &server : shard.all_replicas) {
        guarantee(states.count(server) != 0 || disconnected.count(server) != 0);
        if (disconnected.count(server) != 0) {
            replicas_builder.add(convert_replica_status_to_datum(
                server, "disconnected", identifier_format, server_names));
        }
    }

    ql::datum_object_builder_t shard_builder;
    shard_builder.overwrite("primary_replicas",
        std::move(primary_replicas_builder).to_datum());
    shard_builder.overwrite("replicas",
        std::move(replicas_builder).to_datum());
    return std::move(shard_builder).to_datum();
}

ql::datum_t convert_table_status_to_datum(
        const namespace_id_t &table_id,
        const ql::datum_t &db_name_or_uuid,
        const table_config_and_shards_t &config,
        const std::map<server_id_t, range_map_t<key_range_t::right_bound_t,
            table_status_response_t::shard_status_t> > &server_shards,
        const std::set<server_id_t> &disconnected,
        table_readiness_t readiness,
        admin_identifier_format_t identifier_format,
        const server_name_map_t &server_names) {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(table_name));

    ql::datum_array_builder_t shards_builder(ql::configured_limits_t::unlimited);
    for (size_t i = 0; i < config.config.shards.size(); ++i) {
        key_range_t range = config.shard_scheme.get_shard_range(i);
        std::map<server_id_t, table_status_response_t::shard_status_t> states;
        for (const auto &pair : server_shards) {
            pair.second.visit(
                key_range_t::right_bound_t(range.left),
                range.right,
                [&](const key_range_t::right_bound_t &,
                        const key_range_t::right_bound_t &,
                        const table_status_response_t::shard_status_t &range_states) {
                    states[pair.first].merge(range_states);
                });
        }
        shards_builder.add(convert_shard_status_to_datum(
            config.config.shards[i], states, disconnected, identifier_format,
            server_names));
    }
    builder.overwrite("shards", std::move(shards_builder).to_datum());

    ql::datum_object_builder_t status_builder;
    status_builder.overwrite("ready_for_outdated_reads", ql::datum_t::boolean(
        readiness >= table_readiness_t::outdated_reads));
    status_builder.overwrite("ready_for_reads", ql::datum_t::boolean(
        readiness >= table_readiness_t::reads));
    status_builder.overwrite("ready_for_writes", ql::datum_t::boolean(
        readiness >= table_readiness_t::writes));
    status_builder.overwrite("all_replicas_ready", ql::datum_t::boolean(
        readiness == table_readiness_t::finished));
    builder.overwrite("status", std::move(status_builder).to_datum());

    return std::move(builder).to_datum();
}

void table_status_artificial_table_backend_t::format_row(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &config,
        const ql::datum_t &db_name_or_uuid,
        signal_t *interruptor_on_home,
        ql::datum_t *row_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    assert_thread();

    /* Fetch the statuses from every server */
    std::map<server_id_t, range_map_t<key_range_t::right_bound_t,
        table_status_response_t::shard_status_t> > server_shards;
    bool all_replicas_ready;
    table_meta_client->get_shard_status(
        table_id, interruptor_on_home,
        &server_shards, &all_replicas_ready);

    /* We need to pay special attention to servers that appear in the config but not in
    the status response. There are two possible reasons: they might be disconnected, or
    they might not be hosting the server currently. In the former case we insert them
    into `disconnected`, and in the latter case we set their state to `transitioning` in
    `server_shards`. */
    std::set<server_id_t> disconnected;
    for (const table_config_t::shard_t &shard : config.config.shards) {
        for (const server_id_t &server : shard.replicas) {
            if (server_shards.count(server) == 0 && disconnected.count(server) == 0) {
                if (static_cast<bool>(server_config_client->
                        get_server_to_peer_map()->get_key(server))) {
                    table_status_response_t::shard_status_t status;
                    status.transitioning = true;
                    server_shards.insert(std::make_pair(server,
                        range_map_t<key_range_t::right_bound_t,
                                    table_status_response_t::shard_status_t>(
                            key_range_t::right_bound_t(store_key_t()),
                            key_range_t::right_bound_t::make_unbounded(),
                            status)));
                            
                } else {
                    disconnected.insert(server);
                }
            }
        }
    }

    /* Collect server names. We need the name of every server in the config and every
    server in `server_shards`. */
    server_name_map_t server_names = config.server_names;
    for (auto it = server_shards.begin(); it != server_shards.end();) {
        if (server_names.names.find(it->first) != server_names.names.end()) {
            ++it;
        } else {
            bool found = false;
            server_config_client->get_server_config_map()->read_key(it->first,
            [&](const server_config_versioned_t *config) {
                if (config != nullptr) {
                    found = true;
                    server_names.names.insert(std::make_pair(it->first,
                        std::make_pair(config->version, config->config.name)));
                }
            }):
            if (found) {
                ++it;
            } else {
                /* If we can't find the name for one of the servers in the response, then
                act as though it was disconnected */
                server_shards.erase(it++);
            }
        }
    }

    /* Compute the overall level of table readiness by sending probe queries */
    table_readiness_t readiness;
    if (all_replicas_ready) {
        readiness = table_readiness_t::finished;
    } else {
        namespace_interface_access_t ns_if =
            namespace_repo->get_namespace_interface(table_id, interruptor_on_home);
        if (ns_if.get()->check_readiness(
                table_readiness_t::writes, interruptor_on_home)) {
            readiness = table_readiness_t::writes;
        } else if (ns_if.get()->check_readiness(
                table_readiness_t::reads, interruptor_on_home)) {
            readiness = table_readiness_t::reads;
        } else if (ns_if.get()->check_readiness(
                table_readiness_t::outdated_reads, interruptor_on_home)) {
            readiness = table_readiness_t::outdated_reads;
        } else {
            readiness = table_readiness_t::nothing;
        }
    }

    *row_out = convert_table_status_to_datum(
        table_id,
        config.config.basic.name,
        db_name_or_uuid,
        server_shards,
        disconnected,
        readiness,
        identifier_format,
        server_names);
}

bool table_status_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor_on_caller,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb.table_status` table.";
    return false;
}

