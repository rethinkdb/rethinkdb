// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

std::string common_server_artificial_table_backend_t::get_primary_key_name() {
    return "uuid";
}

bool common_server_artificial_table_backend_t::read_all_rows_as_vector(
        UNUSED signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    rows_out->clear();
    machines_semilattice_metadata_t servers_sl = servers_sl_view->get();
    bool result = true;
    name_client->get_machine_id_to_name_map()->apply_read(
        [&](const std::map<machine_id_t, name_string_t> *map) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                ql::datum_t row;
                std::map<
                        machine_id_t, deletable_t<machine_semilattice_metadata_t>
                    >::iterator sl_it;
                if (!search_metadata_by_uuid(&servers_sl.machines, it->first, &sl_it)) {
                    continue;
                }
                if (!format_row(it->second, it->first, sl_it->second.get_ref(),
                                &row, error_out)) {
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
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    machines_semilattice_metadata_t servers_sl = servers_sl_view->get();
    name_string_t server_name;
    machine_id_t server_id;
    machine_semilattice_metadata_t *server_sl;
    if (!lookup(primary_key, &servers_sl, &server_name, &server_id, &server_sl)) {
        *row_out = ql::datum_t();
        return true;
    } else {
        return format_row(server_name, server_id, *server_sl, row_out, error_out);
    }
}

bool common_server_artificial_table_backend_t::lookup(
        ql::datum_t primary_key,
        machines_semilattice_metadata_t *machines,
        name_string_t *name_out,
        machine_id_t *server_id_out,
        machine_semilattice_metadata_t **machine_out) {
    assert_thread();
    std::string dummy_error;
    if (!convert_uuid_from_datum(primary_key, server_id_out, &dummy_error)) {
        return false;
    }
    std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it;
    if (!search_metadata_by_uuid(&machines->machines, *server_id_out, &it)) {
        return false;
    }
    *machine_out = it->second.get_mutable();
    boost::optional<name_string_t> res =
        name_client->get_name_for_machine_id(*server_id_out);
    if (!res) {
        /* This is probably impossible, but it could conceivably be possible due to a
        race condition */
        return false;
    }
    *name_out = *res;
    return true;
}

