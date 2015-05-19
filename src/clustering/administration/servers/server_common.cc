// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "concurrency/cross_thread_signal.hpp"

common_server_artificial_table_backend_t::common_server_artificial_table_backend_t(
        server_config_client_t *_server_config_client,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory) :
    directory(_directory),
    server_config_client(_server_config_client),
    directory_subs(directory,
        [this](const peer_id_t &, const cluster_directory_metadata_t *md) {
            if (md == nullptr) {
                notify_all();
            } else {
                /* This is one of the rare cases where we can tell exactly which row
                needs to be recomputed */
                notify_row(convert_uuid_to_datum(md->server_id));
            }
        }, false)
{
    directory->assert_thread();
    server_config_client->assert_thread();
}

std::string common_server_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool common_server_artificial_table_backend_t::read_all_rows_as_vector(
        UNUSED signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    rows_out->clear();
    bool ok = true;
    directory->read_all(
    [&](const peer_id_t &peer_id, const cluster_directory_metadata_t *md) {
        if (md->peer_type != peer_type_t::SERVER_PEER) {
            return;
        }
        ql::datum_t row;
        if (!format_row(md->server_id, peer_id, *md, &row, error_out)) {
            ok = false;
            return;
        }
        rows_out->push_back(row);
    });
    return ok;
}

bool common_server_artificial_table_backend_t::read_row(
        const ql::datum_t &primary_key,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    server_id_t server_id;
    peer_id_t peer_id;
    cluster_directory_metadata_t metadata;
    if (!lookup(primary_key, &server_id, &peer_id, &metadata)) {
        *row_out = ql::datum_t();
        return true;
    } else {
        return format_row(server_id, peer_id, metadata, row_out, error_out);
    }
}

bool common_server_artificial_table_backend_t::lookup(
        const ql::datum_t &primary_key,
        server_id_t *server_id_out,
        peer_id_t *peer_id_out,
        cluster_directory_metadata_t *metadata_out) {
    assert_thread();
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, server_id_out, &dummy_error)) {
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
            guarantee(metadata->peer_type == peer_type_t::SERVER_PEER);
            *metadata_out = metadata;
            ok = true;
        }
    });
    return ok;
}

