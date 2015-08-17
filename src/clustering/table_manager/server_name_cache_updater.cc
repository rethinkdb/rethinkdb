// Copyright 2010-2015 RethinkDB, all rights reserved.

#include "clustering/table_manager/server_name_cache_updater.hpp"

#include "clustering/generic/raft_core.tcc"

server_name_cache_updater_t::server_name_cache_updater_t(
        raft_member_t<table_raft_state_t> *_raft,
        server_config_client_t *server_config_client)
    : raft(_raft),
      update_pumper(
        std::bind(&server_name_cache_updater_t::update_blocking, this, ph::_1)),
      server_config_map(server_config_client->get_server_config_map()),
      server_config_map_subs(
        server_config_map,
        [&](const server_id_t &server_id, const server_config_versioned_t *) {
            dirty_server_ids.insert(server_id);
            update_pumper.notify();
        },
        initial_call_t::YES) {
}

void calculate_new_server_names(
        const server_name_map_t &old_server_names,
        watchable_map_t<server_id_t, server_config_versioned_t> *server_config_map,
        std::set<server_id_t> dirty_server_ids,
        server_name_map_t *new_server_names_out) {
    for (const auto &server_id : dirty_server_ids) {
        server_config_map->read_key(server_id,
        [&](const server_config_versioned_t *server_config) {
            if (server_config == nullptr) {
                /* The server config could not be read, we'll get a new notification
                when it reconnects. */
                return;
            }

            auto old_name_it = old_server_names.names.find(server_id);
            if (old_name_it == old_server_names.names.end() ||
                    old_name_it->second.first >= server_config->version ||
                    old_name_it->second.second == server_config->config.name) {
                /* Either the server is not involved in this table, the server name
                that's already stored has a never version that the server configuration,
                or the server name hasn't changed, thus we skip this server. */
                return;
            }

            new_server_names_out->names.insert(
                std::make_pair(server_id,
                    std::make_pair(server_config->version, server_config->config.name)));
        });
    }

    return;
}

void server_name_cache_updater_t::update_blocking(signal_t *interruptor) {
    std::set<server_id_t> local_dirty_server_ids;
    std::swap(dirty_server_ids, local_dirty_server_ids);
    if (local_dirty_server_ids.empty()) {
        /* Nothing to do. */
        return;
    }

    /* Now we'll apply changes to Raft. We keep trying in a loop in case it
    doesn't work at first. */
    while (true) {
        /* Wait until the Raft member is likely to accept changes */
        raft->get_readiness_for_change()->run_until_satisfied(
            [](bool is_ready) { return is_ready; }, interruptor);

        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);

        /* Calculate the proposed change */
        table_raft_state_t::change_t::new_server_names_t change;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
            calculate_new_server_names(
                state->state.config.server_names,
                server_config_map,
                local_dirty_server_ids,
                &change.config_and_shards);
            calculate_new_server_names(
                state->state.server_names,
                server_config_map,
                local_dirty_server_ids,
                &change.raft_state);
        });

        /* Apply the change, unless it's a no-op */
        bool change_ok;
        if (!change.config_and_shards.names.empty() ||
                !change.raft_state.names.empty()) {
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_change(
                    &change_lock, table_raft_state_t::change_t(change));

            change_ok = change_token.has();

            /* Note that we don't wait on the change token. We know we won't
            start a redundant change because we always compute the change by
            comparing to `get_latest_state()`. */
        } else {
            change_ok = true;
        }

        /* If the change failed, go back to the top of the loop and wait on
        `get_readiness_for_change()` again. */
        if (change_ok) {
            break;
        }
    }
}
