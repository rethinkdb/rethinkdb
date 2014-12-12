// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/server_common.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"

common_server_artificial_table_backend_t::common_server_artificial_table_backend_t(
        boost::shared_ptr< semilattice_readwrite_view_t<
            servers_semilattice_metadata_t> > _servers_sl_view,
        server_config_client_t *_server_config_client) :
    servers_sl_view(_servers_sl_view),
    server_config_client(_server_config_client),
    subs([this]() { notify_all(); }, _servers_sl_view)
{
    servers_sl_view->assert_thread();
    server_config_client->assert_thread();
}

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
    /* Note that we don't have a subscription watching `get_server_id_to_name_map()`,
    even though we use it in this calculation. This is OK because any time
    `get_server_id_to_name_map()` changes, we'll also get a notification through
    `servers_sl_view`, and since the `cfeed_artificial_table_backend_t` does its
    recalculations asynchronously, it shouldn't matter that there is a slight delay
    between when we get a notification through `servers_sl_view` and when
    `get_server_id_to_name_map()` actually changes. But it's still a bit fragile, and
    maybe we shouldn't do it. */
    std::map<server_id_t, name_string_t> server_map =
        server_config_client->get_server_id_to_name_map()->get();
    for (auto it = server_map.begin(); it != server_map.end(); ++it) {
        ql::datum_t row;
        std::map<server_id_t, deletable_t<server_semilattice_metadata_t> >
            ::iterator sl_it;
        if (!search_metadata_by_uuid(&servers_sl.servers, it->first, &sl_it)) {
            continue;
        }
        if (!format_row(it->second, it->first, sl_it->second.get_ref(),
                        interruptor, &row, error_out)) {
            return false;
        }
        rows_out->push_back(row);
    }
    return true;
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
        server_config_client->get_name_for_server_id(*server_id_out);
    if (!res) {
        /* This is probably impossible, but it could conceivably be possible due to a
        race condition */
        return false;
    }
    *name_out = *res;
    return true;
}

