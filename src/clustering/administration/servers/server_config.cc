// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_config.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

counted_t<const ql::datum_t> convert_server_config_to_datum(
        const name_string_t &name,
        const machine_id_t &machine_id,
        std::set<name_string_t> tags) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(name));
    builder.overwrite("uuid", convert_uuid_to_datum(machine_id));
    builder.overwrite("tags", convert_set_to_datum<name_string_t>(
            &convert_name_to_datum, tags));
    return std::move(builder).to_counted();
}

bool convert_server_config_from_datum(
        counted_t<const ql::datum_t> datum,
        name_string_t *name_out,
        machine_id_t *machine_id_out,
        std::set<name_string_t> *tags_out,
        std::string *error_out) {
    converter_from_datum_object_t converter;
    if (!converter.init(datum, error_out)) {
        return false;
    }

    counted_t<const ql::datum_t> name_datum;
    if (!converter.get("name", &name_datum, error_out)) {
        return false;
    }
    if (!convert_name_from_datum(name_datum, "server name", name_out, error_out)) {
        *error_out = "In `name`: " + *error_out;
        return false;
    }

    counted_t<const ql::datum_t> uuid_datum;
    if (!converter.get("uuid", &uuid_datum, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(uuid_datum, machine_id_out, error_out)) {
        *error_out = "In `uuid`: " + *error_out;
        return false;
    }

    counted_t<const ql::datum_t> tags_datum;
    if (!converter.get("tags", &tags_datum, error_out)) {
        return false;
    }
    if (!convert_set_from_datum<name_string_t>(
            [] (counted_t<const ql::datum_t> datum2, name_string_t *val2_out,
                    std::string *error2_out) {
                return convert_name_from_datum(datum2, "server tag", val2_out,
                    error2_out);
            },
            true,   /* don't complain if a tag appears twice */
            tags_datum, tags_out, error_out)) {
        *error_out = "In `tags`: " + *error_out;
        return false;
    }

    if (!converter.check_no_extra_keys(error_out)) {
        return false;
    }

    return true;
}

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
        UNUSED std::string *error_out) {
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
        if (!name_client->retag_server(machine_id, server_name, new_tags, interruptor,
                                       error_out)) {
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
    if (!convert_name_from_datum(primary_key, "server name", name_out, &dummy_error)) {
        return false;
    }
    boost::optional<machine_id_t> res = name_client->get_machine_id_for_name(*name_out);
    if (!res) {
        return false;
    }
    *machine_id_out = *res;
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

