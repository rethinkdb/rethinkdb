// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

std::string common_server_artificial_table_backend_t::get_primary_key_name() {
    return "uuid";
}

bool common_server_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<ql::datum_t> *keys_out,
        UNUSED std::string *error_out) {
    on_thread_t thread_switcher(home_thread());
    keys_out->clear();
    name_client->get_machine_id_to_name_map()->apply_read(
        [&](const std::map<machine_id_t, name_string_t> *map) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                keys_out->push_back(convert_uuid_to_datum(it->first));
            }
        });
    return true;
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

