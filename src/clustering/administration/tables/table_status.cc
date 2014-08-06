// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_status.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

counted_t<const ql::datum_t> convert_backfill_to_datum(
        UNUSED const reactor_business_card_details::backfill_location_t &location) {
    return make_counted<const ql::datum_t>("backfill progress is not implemented");
}

counted_t<const ql::datum_t> convert_director_status_to_datum(
        const boost::optional<reactor_activity_entry_t::activity_t> *status) {
    ql::datum_object_builder_t object_builder;
    object_builder.overwrite("role", make_counted<const ql::datum_t>("director"));
    std::string state;
    if (!status) {
        state = "missing";
    } else if (!*status) {
        state = "transitioning";
    } else if (const reactor_activity_entry_t::primary_when_safe_t *details =
            boost::get<reactor_activity_entry_t::primary_when_safe_t>(&**status)) {
        state = "backfill_data";
        object_builder.overwrite("backfill_sources",
            convert_vector_to_datum<reactor_business_card_details::backfill_location_t>(
                &convert_backfill_to_datum,
                details->backfills_waited_on));
    } else if (boost::get<reactor_activity_entry_t::primary_t>(&**status) != NULL) {
        state = "ready";
    } else {
        /* The other server looks like it's still trying to be something other than a
        primary, which means the metadata hasn't propagated yet. */
        state = "transitioning";
    }
    object_builder.overwrite("state", make_counted<const ql::datum_t>(std::move(state)));
    return std::move(object_builder).to_counted();
} 

counted_t<const ql::datum_t> convert_replica_status_to_datum(
        const boost::optional<reactor_activity_entry_t::activity_t> *status) {
    ql::datum_object_builder_t object_builder;
    object_builder.overwrite("role", make_counted<const ql::datum_t>("replica"));
    std::string state;
    if (!status) {
        state = "missing";
    } else if (!*status) {
        state = "transitioning";
    } else if (boost::get<reactor_activity_entry_t::secondary_without_primary_t>(
            &**status) != NULL) {
        state = "need_director";
    } else if (const reactor_activity_entry_t::secondary_backfilling_t *details =
            boost::get<reactor_activity_entry_t::secondary_backfilling_t>(&**status)) {
        state = "backfill_data";
        std::vector<counted_t<const ql::datum_t> > sources;
        sources.push_back(convert_backfill_to_datum(details->backfill));
        object_builder.overwrite("backfill_sources",
            make_counted<const ql::datum_t>(std::move(sources),
                                            ql::configured_limits_t()));
    } else if (boost::get<reactor_activity_entry_t::secondary_up_to_date_t>(
            &**status) != NULL) {
        state = "ready";
    } else {
        state = "transitioning";
    }
    object_builder.overwrite("state", make_counted<const ql::datum_t>(std::move(state)));
    return std::move(object_builder).to_counted();
}

counted_t<const ql::datum_t> convert_nothing_status_to_datum(
         const boost::optional<reactor_activity_entry_t::activity_t> *status) {
    ql::datum_object_builder_t object_builder;
    object_builder.overwrite("role", make_counted<const ql::datum_t>("nothing"));
    std::string state;
    if (!status) {
        state = "missing";
    } else if (!*status) {
        state = "transitioning";
    } else if (boost::get<reactor_activity_entry_t::nothing_when_safe_t>(
            &**status) != NULL) {
        state = "offload_data";
    } else if (boost::get<reactor_activity_entry_t::nothing_when_done_erasing_t>(
            &**status) != NULL) {
        state = "erase_data";
    } else if (boost::get<reactor_activity_entry_t::nothing_t>(&**status) != NULL) {
        return counted_t<const ql::datum_t>();
    } else {
        state = "transitioning";
    }
    object_builder.overwrite("state", make_counted<const ql::datum_t>(std::move(state)));
    return std::move(object_builder).to_counted();
}

counted_t<const ql::datum_t> convert_table_status_shard_to_datum(
        namespace_id_t uuid,
        region_t region,
        const table_config_t::shard_t &shard,
        machine_id_t chosen_director,
        const change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> &dir,
        server_name_client_t *name_client) {
    /* First, determine the state of each server with respect to this shard. If an entry
    is missing from this map, that means the server is not connected. If an entry is
    present but the `boost::optional` is empty, that means the server is connected but
    its state reflects a different sharding scheme; this will be described as
    `transitioning`. */
    std::map<name_string_t, boost::optional<reactor_activity_entry_t::activity_t> >
        server_states;
    for (auto it = dir.get_inner().begin(); it != dir.get_inner().end(); ++it) {

        /* Translate peer ID to machine ID */
        boost::optional<machine_id_t> machine_id =
            name_client->get_machine_id_for_peer_id(it->first);
        if (!machine_id) {
            /* This can occur as a race condition if the peer has just connected or just
            disconnected */
            continue;
        }

        /* Translate machine ID to server name */
        boost::optional<name_string_t> name =
            name_client->get_name_for_machine_id(*machine_id);
        if (!name) {
            /* This can occur if the peer was permanently removed */
            continue;
        }

        /* Extract activity from reactor business card. `server_state` may be left empty
        if the reactor doesn't have a business card for this table, or if no entry has
        the same region as the target region. */
        boost::optional<reactor_activity_entry_t::activity_t> server_state;
        auto jt = it->second.reactor_bcards.find(uuid);
        if (jt != it->second.reactor_bcards.end()) {
            cow_ptr_t<reactor_business_card_t> bcard = jt->second.internal;
            for (auto kt = bcard->activities.begin();
                      kt != bcard->activities.end();
                    ++kt) {
                if (kt->second.region == region) {
                    server_state = boost::optional<reactor_activity_entry_t::activity_t>(
                        kt->second.activity);
                    break;
                }
            }
        }

        server_states.insert(std::make_pair(*name, server_state));
    }

    ql::datum_object_builder_t object_builder;

    boost::optional<name_string_t> director_name =
        name_client->get_name_for_machine_id(chosen_director);
    if (director_name) {
        object_builder.overwrite(director_name->str(),
            convert_director_status_to_datum(
                server_states.count(*director_name) == 1 ?
                    &server_states[*director_name] : NULL));
    }

    for (const name_string_t &replica : shard.replica_names) {
        if (object_builder.try_get(replica.str()).has()) {
            /* Don't overwrite the director's entry */
            continue;
        }
        object_builder.overwrite(replica.str(),
            convert_replica_status_to_datum(
                server_states.count(replica) == 1 ?
                    &server_states[replica] : NULL));
    }

    std::map<name_string_t, machine_id_t> other_names =
        name_client->get_name_to_machine_id_map()->get();
    for (auto it = other_names.begin(); it != other_names.end(); ++it) {
        if (object_builder.try_get(it->first.str()).has()) {
            /* Don't overwrite a director or replica entry */
            continue;
        }
        counted_t<const ql::datum_t> entry =
            convert_nothing_status_to_datum(
                server_states.count(it->first) == 1 ?
                    &server_states[it->first] : NULL);
        if (entry.has()) {
            object_builder.overwrite(it->first.str(), entry);
        }
    }

    return std::move(object_builder).to_counted();
}

counted_t<const ql::datum_t> convert_table_status_to_datum(
        name_string_t name,
        namespace_id_t uuid,
        const table_replication_info_t &repli_info,
        const change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> &dir,
        server_name_client_t *name_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(name));
    builder.overwrite("uuid", convert_uuid_to_datum(uuid));
    
    ql::datum_array_builder_t array_builder((ql::configured_limits_t()));
    for (size_t i = 0; i < repli_info.config.shards.size(); ++i) {
        array_builder.add(
            convert_table_status_shard_to_datum(
                uuid,
                hash_region_t<key_range_t>(repli_info.config.get_shard_range(i)),
                repli_info.config.shards[i],
                repli_info.chosen_directors[i],
                dir,
                name_client));
    }
    builder.overwrite("shards", std::move(array_builder).to_counted());

    return std::move(builder).to_counted();
}

std::string table_status_artificial_table_backend_t::get_primary_key_name() {
    return "name";
}

bool table_status_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<counted_t<const ql::datum_t> > *keys_out,
        std::string *error_out) {
    cow_ptr_t<namespaces_semilattice_metadata_t> md = semilattice_view->get();
    for (auto it = md->namespaces.begin(); it != md->namespaces.end(); ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        if (it->second.get_ref().name.in_conflict()) {
            /* TODO: Find a better solution */
            *error_out = "There's a name conflict";
            return false;
        }
        keys_out->push_back(convert_name_to_datum(
            it->second.get_ref().name.get_ref()));
    }
    return true;
}

bool table_status_artificial_table_backend_t::read_row(
        counted_t<const ql::datum_t> primary_key,
        UNUSED signal_t *interruptor,
        counted_t<const ql::datum_t> *row_out,
        std::string *error_out) {
    name_string_t name;
    std::string dummy_error;
    if (!convert_name_from_datum(primary_key, "server name", &name, &dummy_error)) {
        /* If the primary key was not a valid table name, then it must refer to a
        nonexistent row. By setting `name` to an empty `name_string_t`, we ensure that
        the loop doesn't find any table, so it will correctly fall through to the case
        where the row does not exist. */
        name = name_string_t();
    }
    cow_ptr_t<namespaces_semilattice_metadata_t> md = semilattice_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted() || it->second.get_ref().name.in_conflict()) {
            /* TODO: Handle conflict differently */
            *error_out = "A name is in conflict.";
            return false;
        }
        if (it->second.get_ref().name.get_ref() == name) {
            if (it->second.get_ref().replication_info.in_conflict()) {
                *error_out = "Metadata is in conflict.";
                return false;
            }
            *row_out = convert_table_status_to_datum(
                name,
                it->first,
                it->second.get_ref().replication_info.get_ref(),
                directory_view->get(),
                name_client);
            return true;
        }
    }
    /* No table with the given name exists. Signal this by setting `*row_out` to an empty
    map, and return `true` to indicate that the read was performed. */
    *row_out = counted_t<const ql::datum_t>();
    return true;
}

bool table_status_artificial_table_backend_t::write_row(
        UNUSED counted_t<const ql::datum_t> primary_key,
        UNUSED counted_t<const ql::datum_t> new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    *error_out = "The `rethinkdb.table_status` table is read-only.";
    return false;
}


