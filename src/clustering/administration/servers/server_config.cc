// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_config.hpp"

counted_t<const ql::datum_t> 

std::string server_config_artificial_table_backend_t::get_primary_key_name() {
    return "name";
}

bool server_config_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<counted_t<const ql::datum_t> > *keys_out,
        UNUSED std::string *error_out) {
    keys_out->clear();
    name_client->get_name_to_machine_id_map()->apply_read(
        [&](const std::map<name_string_t, machine_id_t> *map) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                keys_out->push_back(convert_name_to_datum(it->first));
            }
        });
    return true;
}

bool server_config_artificial_table_backend_t::read_row(
        counted_t<const ql::datum_t> primary_key,
        UNUSED signal_t *interruptor,
        counted_t<const ql::datum_t> *row_out,
        std::string *error_out) {
    machines_semilattice_metadata_t servers_sl = servers_sl_view->get();
    name_string_t server_name;
    machine_id_t machine_id;
    machine_semilattice_metadata_t *server_sl;
    if (!lookup(primary_key, &servers_sl, &server_name, &machine_id, &server_sl)) {
        *row_out = counted_t<const ql::datum_t>();
        return true;
    }
    *row_out = convert_server_config_to_datum(
        server_name, machine_id, server_sl->tags.get_ref());
    return true;
}

bool server_config_artificial_table_backend_t::write_row(
        counted_t<const ql::datum_t> primary_key,
        counted_t<const ql::datum_t> new_value,
        signal_t *interruptor,
        std::string *error_out) {
    machines_semilattice_metadata_t servers_sl = servers_sl_view->get();
    name_string_t server_name;
    machine_id_t machine_id;
    machine_semilattice_metadata_t *server_sl;
    if (!lookup(primary_key, &servers_sl, &server_name, &machine_id, &server_sl)) {
        if (!new_value.has()) {
            /* We're deleting a row that already didn't exist. Okay. */
            return true;
        } else {
            *error_out = "It's illegal to insert new rows into the "
                "`rethinkdb.server_config` artificial table.";
            return false;
        }
    } else if (!new_value.has()) {
        *error_out = "It's illegal to delete rows from the `rethinkdb.server_config` "
            "artificial table. If you want to permanently remove a server, use "
            "r.server_permanently_remove().";
        return false;
    }
    name_string_t new_server_name;
    machine_id_t new_machine_id;
    std::set<name_string_t> new_tags;
    if (!convert_server_config_from_datum(new_value, &new_server_name, &new_machine_id,
            &new_tags, error_out)) {
        *error_out = "The row you're trying to put into `rethinkdb.server_config` "
            "has the wrong format. " + *error_out;
        return false;
    }
    guarantee(server_name == new_server_name, "artificial_table_t should ensure that "
        "primary key is unchanged.");
    if (machine_id != new_machine_id) {
        *error_out = "It's illegal to change a server's `uuid`.";
        return false;
    }
    if (new_tags != server_sl->tags.get_ref()) {
        if (!name_client->retag_server(machine_id, new_tags, interruptor, error_out)) {
            return false;
        }
    }
    return true;
}

bool server_config_artificial_table_backend_t::lookup(
        counted_t<const ql::datum_t> primary_key,
        machines_semilattice_metadata_t *machines,
        name_string_t *name_out,
        machine_id_t *machine_id_out,
        machine_semilattice_metadata_t **machine_out) {
    std::string dummy_error;
    if (!convert_name_from_datum(primary_key, "server name", name_out, &dummy_error) {
        return false;
    }
    boost::optional<machine_id_t> lookup =
            name_client->get_machine_id_for_name(*name_out);
    if (!lookup) {
        return false;
    }
    *machine_id_out = *lookup;
    auto it = machines->machines.find(*machine_id_out);
    if (it == machines->machines.end()) {
        /* This is probably impossible, but it could conceivably be possible due to a
        race condition */
        return false;
    }
    if (it->second.is_deleted()) {
        /* This could happen due to a race condition */
        return false;
    }
    *machine_out = it->second.get_mutable();
    return true;
}

