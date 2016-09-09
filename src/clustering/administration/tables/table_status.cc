// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_status.hpp"

#include <algorithm>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "clustering/table_contract/executor/exec_primary.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

table_status_artificial_table_backend_t::table_status_artificial_table_backend_t(
        rdb_context_t *rdb_context,
        lifetime_t<name_resolver_t const &> name_resolver,
        std::shared_ptr<semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        server_config_client_t *_server_config_client,
        table_meta_client_t *_table_meta_client,
        namespace_repo_t *_namespace_repo,
        admin_identifier_format_t _identifier_format)
    : common_table_artificial_table_backend_t(
        name_string_t::guarantee_valid("table_status"),
        rdb_context,
        name_resolver,
        _semilattice_view,
        _table_meta_client,
        _identifier_format),
      server_config_client(_server_config_client),
      namespace_repo(_namespace_repo) {
}

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
        server_names.get(server_id), server_id.get_uuid(), identifier_format));
    replica_builder.overwrite("state", ql::datum_t(status));
    return std::move(replica_builder).to_datum();
}

ql::datum_t convert_shard_status_to_datum(
        const table_config_t::shard_t &shard,
        const std::map<server_id_t, table_shard_status_t> &states,
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
                server_names.get(pair.first), pair.first.get_uuid(), identifier_format));
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

ql::datum_t convert_raft_leader_to_datum(
        const table_status_t &status,
        admin_identifier_format_t identifier_format) {
    if (static_cast<bool>(status.raft_leader)) {
        return convert_name_or_uuid_to_datum(
            status.server_names.get(*status.raft_leader),
            status.raft_leader->get_uuid(),
            identifier_format);
    }

    return ql::datum_t::null();
}

ql::datum_t convert_table_status_to_datum(
        const table_status_t &status,
        admin_identifier_format_t identifier_format) {
    ql::datum_object_builder_t builder;

    if (status.total_loss) {
        builder.overwrite("shards", ql::datum_t::null());
    } else {
        ql::datum_array_builder_t shards_builder(ql::configured_limits_t::unlimited);
        for (size_t i = 0; i < status.config.config.shards.size(); ++i) {
            key_range_t range = status.config.shard_scheme.get_shard_range(i);
            std::map<server_id_t, table_shard_status_t> states;
            for (const auto &pair : status.server_shards) {
                pair.second.visit(
                    key_range_t::right_bound_t(range.left),
                    range.right,
                    [&](const key_range_t::right_bound_t &,
                            const key_range_t::right_bound_t &,
                            const table_shard_status_t &range_states) {
                        states[pair.first].merge(range_states);
                    });
            }
            shards_builder.add(convert_shard_status_to_datum(
                status.config.config.shards[i], states, status.disconnected,
                identifier_format, status.server_names));
        }
        builder.overwrite("shards", std::move(shards_builder).to_datum());
    }

    // add raft leader information
    builder.overwrite("raft_leader",
      convert_raft_leader_to_datum(status, identifier_format));

    ql::datum_object_builder_t status_builder;
    status_builder.overwrite("ready_for_outdated_reads", ql::datum_t::boolean(
        status.readiness >= table_readiness_t::outdated_reads));
    status_builder.overwrite("ready_for_reads", ql::datum_t::boolean(
        status.readiness >= table_readiness_t::reads));
    status_builder.overwrite("ready_for_writes", ql::datum_t::boolean(
        status.readiness >= table_readiness_t::writes));
    status_builder.overwrite("all_replicas_ready", ql::datum_t::boolean(
        status.readiness == table_readiness_t::finished));
    builder.overwrite("status", std::move(status_builder).to_datum());

    return std::move(builder).to_datum();
}

void table_status_artificial_table_backend_t::format_row(
        UNUSED auth::user_context_t const &user_context,
        const namespace_id_t &table_id,
        const table_config_and_shards_t &config,
        const ql::datum_t &db_name_or_uuid,
        signal_t *interruptor_on_home,
        ql::datum_t *row_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    assert_thread();
    table_status_t status;
    get_table_status(table_id, config, namespace_repo, table_meta_client,
        server_config_client, interruptor_on_home, &status);
    ql::datum_t status_datum = convert_table_status_to_datum(status, identifier_format);
    ql::datum_object_builder_t builder(status_datum);
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(status.config.config.basic.name));
    *row_out = std::move(builder).to_datum();
}

void table_status_artificial_table_backend_t::format_error_row(
        UNUSED auth::user_context_t const &user_context,
        const namespace_id_t &table_id,
        const ql::datum_t &db_name_or_uuid,
        const name_string_t &table_name,
        ql::datum_t *row_out) {
    assert_thread();
    table_status_t status;
    status.readiness = table_readiness_t::unavailable;
    status.total_loss = true;
    ql::datum_t status_datum = convert_table_status_to_datum(status, identifier_format);
    ql::datum_object_builder_t builder(status_datum);
    builder.overwrite("id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("name", convert_name_to_datum(table_name));
    *row_out = std::move(builder).to_datum();
}

bool table_status_artificial_table_backend_t::write_row(
        UNUSED auth::user_context_t const &user_context,
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor_on_caller,
        admin_err_t *error_out) {
    *error_out = admin_err_t{
        "It's illegal to write to the `rethinkdb.table_status` table.",
        query_state_t::FAILED};
    return false;
}

