// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

std::string common_server_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool common_server_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    rows_out->clear();
    servers_semilattice_metadata_t servers_sl = servers_sl_view->get();
    bool result = true;
    name_client->get_server_id_to_name_map()->apply_read(
        [&](const std::map<server_id_t, name_string_t> *map) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                ql::datum_t row;
                std::map<
                        server_id_t, deletable_t<server_semilattice_metadata_t>
                    >::iterator sl_it;
                if (!search_metadata_by_uuid(&servers_sl.servers, it->first, &sl_it)) {
                    continue;
                }
                if (!format_row(it->second, it->first, sl_it->second.get_ref(),
                                interruptor, &row, error_out)) {
                    result = false;
                    return;
                }
                rows_out->push_back(row);
            }
        });
    return result;
}

bool common_server_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    servers_semilattice_metadata_t servers_sl = servers_sl_view->get();
    name_string_t server_name;
    server_id_t server_id;
    server_semilattice_metadata_t *server_sl;
    if (!lookup(primary_key, &servers_sl, &server_name, &server_id, &server_sl)) {
        *row_out = ql::datum_t();
        return true;
    } else {
        return format_row(server_name, server_id, *server_sl,
            interruptor, row_out, error_out);
    }
}

bool common_server_artificial_table_backend_t::lookup(
        ql::datum_t primary_key,
        servers_semilattice_metadata_t *servers,
        name_string_t *name_out,
        server_id_t *server_id_out,
        server_semilattice_metadata_t **server_out) {
    assert_thread();
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, server_id_out, &dummy_error)) {
        return false;
    }
    std::map<server_id_t, deletable_t<server_semilattice_metadata_t> >::iterator it;
    if (!search_metadata_by_uuid(&servers->servers, *server_id_out, &it)) {
        return false;
    }
    *server_out = it->second.get_mutable();
    boost::optional<name_string_t> res =
        name_client->get_name_for_server_id(*server_id_out);
    if (!res) {
        /* This is probably impossible, but it could conceivably be possible due to a
        race condition */
        return false;
    }
    *name_out = *res;
    return true;
}

