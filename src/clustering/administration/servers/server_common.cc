// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "concurrency/cross_thread_signal.hpp"

common_server_artificial_table_backend_t::common_server_artificial_table_backend_t(
        name_string_t const &table_name,
        rdb_context_t *rdb_context,
        lifetime_t<name_resolver_t const &> name_resolver,
        server_config_client_t *_server_config_client,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory)
    : caching_cfeed_artificial_table_backend_t(table_name, rdb_context, name_resolver),
      directory(_directory),
      server_config_client(_server_config_client),
      directory_subs(directory,
        [this](const peer_id_t &, const cluster_directory_metadata_t *md) {
            if (md == nullptr) {
                notify_all();
            } else {
                /* This is one of the rare cases where we can tell exactly which row
                needs to be recomputed */
                notify_row(convert_server_id_to_datum(md->server_id));
            }
        }, initial_call_t::NO) {
    directory->assert_thread();
    server_config_client->assert_thread();
}

std::string common_server_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool common_server_artificial_table_backend_t::read_all_rows_as_vector(
        auth::user_context_t const &user_context,
        signal_t *interruptor_on_caller,
        std::vector<ql::datum_t> *rows_out,
        admin_err_t *error_out) {
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    rows_out->clear();
    std::map<peer_id_t, cluster_directory_metadata_t> metadata = directory->get_all();
    for (const auto &pair : metadata) {
        if (pair.second.peer_type != SERVER_PEER) {
            continue;
        }
        ql::datum_t row;
        if (!format_row(
                user_context,
                pair.second.server_id,
                pair.first,
                pair.second,
                &interruptor_on_home,
                &row,
                error_out)) {
            return false;
        }
        rows_out->push_back(row);
    }
    return true;
}

bool common_server_artificial_table_backend_t::read_row(
        auth::user_context_t const &user_context,
        ql::datum_t primary_key,
        signal_t *interruptor_on_caller,
        ql::datum_t *row_out,
        admin_err_t *error_out) {
    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    server_id_t server_id;
    peer_id_t peer_id;
    cluster_directory_metadata_t metadata;
    if (!lookup(primary_key, &server_id, &peer_id, &metadata)) {
        *row_out = ql::datum_t();
        return true;
    } else {
        return format_row(
            user_context,
            server_id,
            peer_id,
            metadata,
            &interruptor_on_home,
            row_out,
            error_out);
    }
}

bool common_server_artificial_table_backend_t::lookup(
        const ql::datum_t &primary_key,
        server_id_t *server_id_out,
        peer_id_t *peer_id_out,
        cluster_directory_metadata_t *metadata_out) {
    assert_thread();
    admin_err_t dummy_error;
    if (!convert_server_id_from_datum(primary_key, server_id_out, &dummy_error)) {
        return false;
    }
    boost::optional<peer_id_t> peer_id =
        server_config_client->get_server_to_peer_map()->get_key(*server_id_out);
    if (!static_cast<bool>(peer_id)) {
        return false;
    }
    *peer_id_out = *peer_id;
    bool ok;
    directory->read_key(*peer_id, [&](const cluster_directory_metadata_t *metadata) {
        if (metadata == nullptr) {
            ok = false;
        } else {
            guarantee(metadata->peer_type == SERVER_PEER);
            *metadata_out = *metadata;
            ok = true;
        }
    });
    return ok;
}

