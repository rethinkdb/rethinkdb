// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_status.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"

/* Names like `reactor_activity_entry_t::secondary_without_primary_t` are too long to
type without this */
using reactor_business_card_details::primary_when_safe_t;
using reactor_business_card_details::primary_t;
using reactor_business_card_details::secondary_up_to_date_t;
using reactor_business_card_details::secondary_without_primary_t;
using reactor_business_card_details::secondary_backfilling_t;
using reactor_business_card_details::nothing_when_safe_t;
using reactor_business_card_details::nothing_when_done_erasing_t;
using reactor_business_card_details::nothing_t;

static bool check_complete_set(const std::vector<reactor_activity_entry_t> &status) {
    std::vector<hash_region_t<key_range_t> > regions;
    for (const reactor_activity_entry_t &entry : status) {
        regions.push_back(entry.region);
    }
    hash_region_t<key_range_t> joined;
    region_join_result_t res = region_join(regions, &joined);
    return res == REGION_JOIN_OK &&
        joined.beg == 0 &&
        joined.end == HASH_REGION_HASH_SIZE;
}

template<class T>
static size_t count_in_state(const std::vector<reactor_activity_entry_t> &status) {
    int count = 0;
    for (const reactor_activity_entry_t &entry : status) {
        if (boost::get<T>(&entry.activity) != NULL) {
            ++count;
        }
    }
    return count;
}

ql::datum_t convert_director_status_to_datum(
        const name_string_t &name,
        const std::vector<reactor_activity_entry_t> *status,
        bool *has_director_out) {
    ql::datum_object_builder_t object_builder;
    object_builder.overwrite("server", convert_name_to_datum(name));
    object_builder.overwrite("role", ql::datum_t("director"));
    const char *state;
    *has_director_out = false;
    if (!status) {
        state = "missing";
    } else if (!check_complete_set(*status)) {
        state = "transitioning";
    } else {
        size_t tally = count_in_state<primary_t>(*status);
        if (tally == status->size()) {
            state = "ready";
            *has_director_out = true;
        } else {
            tally += count_in_state<primary_when_safe_t>(*status);
            if (tally == status->size()) {
                state = "backfilling_data";
                /* RSI(reql_admin): Implement backfill progress */
                object_builder.overwrite("backfill_progress",
                    ql::datum_t("not_implemented"));
            } else {
                state = "transitioning";
            }
        }
    }
    object_builder.overwrite("state", ql::datum_t(state));
    return std::move(object_builder).to_datum();
}

ql::datum_t convert_replica_status_to_datum(
        const name_string_t &name,
        const std::vector<reactor_activity_entry_t> *status,
        bool *has_outdated_reader_out,
        bool *has_replica_out) {
    ql::datum_object_builder_t object_builder;
    object_builder.overwrite("server", convert_name_to_datum(name));
    object_builder.overwrite("role", ql::datum_t("replica"));
    const char *state;
    *has_outdated_reader_out = *has_replica_out = false;
    if (!status) {
        state = "missing";
    } else if (!check_complete_set(*status)) {
        state = "transitioning";
    } else {
        size_t tally = count_in_state<secondary_up_to_date_t>(*status);
        if (tally == status->size()) {
            state = "ready";
            *has_outdated_reader_out = *has_replica_out = true;
        } else {
            tally += count_in_state<secondary_without_primary_t>(*status);
            if (tally == status->size()) {
                state = "looking_for_director";
                *has_outdated_reader_out = true;
            } else {
                tally += count_in_state<secondary_backfilling_t>(*status);
                if (tally == status->size()) {
                    state = "backfilling_data";
                    /* RSI(reql_admin): Implement backfill progress */
                    object_builder.overwrite("backfill_progress",
                        ql::datum_t("not_implemented"));
                } else {
                    state = "transitioning";
                }
            }
        }
    }
    object_builder.overwrite("state", ql::datum_t(std::move(state)));
    return std::move(object_builder).to_datum();
}

ql::datum_t convert_nothing_status_to_datum(
        const name_string_t &name,
        const std::vector<reactor_activity_entry_t> *status,
        bool *is_unfinished_out) {
    *is_unfinished_out = true;
    ql::datum_object_builder_t object_builder;
    object_builder.overwrite("server", convert_name_to_datum(name));
    object_builder.overwrite("role", ql::datum_t("nothing"));
    const char *state;
    if (!status) {
        state = "missing";
        /* Don't display all the tables as unfinished just because one machine is missing
        */
        *is_unfinished_out = false;
    } else if (!check_complete_set(*status)) {
        state = "transitioning";
    } else {
        size_t tally = count_in_state<nothing_t>(*status);
        if (tally == status->size()) {
            *is_unfinished_out = false;
            /* This machine shouldn't even appear in the map */
            return ql::datum_t();
        } else {
            tally += count_in_state<nothing_when_done_erasing_t>(*status);
            if (tally == status->size()) {
                state = "erasing_data";
            } else {
                tally += count_in_state<nothing_when_safe_t>(*status);
                if (tally == status->size()) {
                    state = "offloading_data";
                } else {
                    state = "transitioning";
                }
            }
        }
    }
    object_builder.overwrite("state", ql::datum_t(std::move(state)));
    return std::move(object_builder).to_datum();
}

enum class table_readiness_t {
    unavailable,
    outdated_reads,
    reads,
    writes,
    finished
};

ql::datum_t convert_table_status_shard_to_datum(
        namespace_id_t uuid,
        key_range_t range,
        const table_config_t::shard_t &shard,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                        namespace_directory_metadata_t> *dir,
        server_name_client_t *name_client,
        table_readiness_t *readiness_out) {
    /* `server_states` will contain one entry per connected server. That entry will be a
    vector with the current state of each hash-shard on the server whose key range
    matches the expected range. */
    std::map<machine_id_t, std::vector<reactor_activity_entry_t> > server_states;
    dir->read_all(
        [&](const std::pair<peer_id_t, namespace_id_t> &key,
                const namespace_directory_metadata_t *value) {
            if (key.second != uuid) {
                return;
            }
            /* Translate peer ID to machine ID */
            boost::optional<machine_id_t> machine_id =
                name_client->get_machine_id_for_peer_id(key.first);
            if (!static_cast<bool>(machine_id)) {
                /* This can occur as a race condition if the peer has just connected or just
                disconnected */
                return;
            }
            /* Extract activity from reactor business card. `server_state` may be left
            empty if the reactor doesn't have a business card for this table, or if no
            entry has the same region as the target region. */
            std::vector<reactor_activity_entry_t> server_state;
            for (const auto &pair : value->internal->activities) {
                if (pair.second.region.inner == range) {
                    server_state.push_back(pair.second);
                }
            }
            server_states[*machine_id] = std::move(server_state);
        });

    ql::datum_array_builder_t array_builder(ql::configured_limits_t::unlimited);
    std::set<machine_id_t> already_handled;

    bool has_director = false;
    boost::optional<name_string_t> director_name =
        name_client->get_name_for_machine_id(shard.director);
    if (static_cast<bool>(director_name)) {
        array_builder.add(convert_director_status_to_datum(
            *director_name,
            server_states.count(shard.director) == 1 ?
                &server_states[shard.director] : NULL,
            &has_director));
        already_handled.insert(shard.director);
    } else {
        /* Director was permanently removed; in `table_config` the `director` field will
        have a value of `null`. So we don't show a director entry in `table_status`. */
    }

    bool has_outdated_reader = false;
    for (const machine_id_t &replica : shard.replicas) {
        if (already_handled.count(replica) == 1) {
            /* Don't overwrite the director's entry */
            continue;
        }
        boost::optional<name_string_t> replica_name =
            name_client->get_name_for_machine_id(replica);
        if (!static_cast<bool>(replica_name)) {
            /* Replica was permanently removed. It won't show up in `table_config`. So
            we act as if it wasn't in `shard.replicas`. */
            continue;
        }
        /* RSI(reql_admin): Currently we don't use `this_one_has_replica`. When we figure
        out what to do about acks, we'll use it to compute if the table is available for
        writes. */
        bool this_one_has_replica, this_one_has_outdated_reader;
        array_builder.add(convert_replica_status_to_datum(
            *replica_name,
            server_states.count(replica) == 1 ?
                &server_states[replica] : NULL,
            &this_one_has_outdated_reader,
            &this_one_has_replica));
        if (this_one_has_outdated_reader) {
            has_outdated_reader = true;
        }
        already_handled.insert(replica);
    }

    std::multimap<name_string_t, machine_id_t> other_names =
        name_client->get_name_to_machine_id_map()->get();
    bool is_unfinished = false;
    for (auto it = other_names.begin(); it != other_names.end(); ++it) {
        if (already_handled.count(it->second) == 1) {
            /* Don't overwrite a director or replica entry */
            continue;
        }
        bool this_one_is_unfinished;
        ql::datum_t entry = convert_nothing_status_to_datum(
            it->first,
            server_states.count(it->second) == 1 ?
                &server_states[it->second] : NULL,
            &this_one_is_unfinished);
        if (this_one_is_unfinished) {
            is_unfinished = true;
        }
        if (entry.has()) {
            array_builder.add(entry);
        }
    }

    /* RSI(reql_admin): Currently we assume that only one ack is necessary to perform a
    write. Therefore, any table that's available for up-to-date reads is also available
    for writes. This is consistent with the current behavior of the `reactor_driver_t`.
    But eventually we'll implement some sort of reasonable thing with write acks, and
    then this will change. */
    if (has_director) {
        if (!is_unfinished) {
            *readiness_out = table_readiness_t::finished;
        } else {
            *readiness_out = table_readiness_t::writes;
        }
    } else {
        if (has_outdated_reader) {
            *readiness_out = table_readiness_t::outdated_reads;
        } else {
            *readiness_out = table_readiness_t::unavailable;
        }
    }

    return std::move(array_builder).to_datum();
}

ql::datum_t convert_table_status_to_datum(
        name_string_t table_name,
        name_string_t db_name,
        namespace_id_t uuid,
        const table_replication_info_t &repli_info,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                        namespace_directory_metadata_t> *dir,
        server_name_client_t *name_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(table_name));
    builder.overwrite("db", convert_name_to_datum(db_name));
    builder.overwrite("uuid", convert_uuid_to_datum(uuid));

    table_readiness_t readiness = table_readiness_t::finished;
    ql::datum_array_builder_t array_builder((ql::configured_limits_t::unlimited));
    for (size_t i = 0; i < repli_info.config.shards.size(); ++i) {
        table_readiness_t this_shard_readiness;
        array_builder.add(
            convert_table_status_shard_to_datum(
                uuid,
                repli_info.shard_scheme.get_shard_range(i),
                repli_info.config.shards[i],
                dir,
                name_client,
                &this_shard_readiness));
        readiness = std::min(readiness, this_shard_readiness);
    }
    builder.overwrite("shards", std::move(array_builder).to_datum());

    builder.overwrite("ready_for_outdated_reads", ql::datum_t::boolean(
        readiness >= table_readiness_t::outdated_reads));
    builder.overwrite("ready_for_reads", ql::datum_t::boolean(
        readiness >= table_readiness_t::reads));
    builder.overwrite("ready_for_writes", ql::datum_t::boolean(
        readiness >= table_readiness_t::writes));
    builder.overwrite("ready_completely", ql::datum_t::boolean(
        readiness == table_readiness_t::finished));

    return std::move(builder).to_datum();
}

bool table_status_artificial_table_backend_t::read_row_impl(
        namespace_id_t table_id,
        name_string_t table_name,
        name_string_t db_name,
        const namespace_semilattice_metadata_t &metadata,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    assert_thread();
    *row_out = convert_table_status_to_datum(
        table_name,
        db_name,
        table_id,
        metadata.replication_info.get_ref(),
        directory_view,
        name_client);
    return true;
}

bool table_status_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb.table_status` table.";
    return false;
}

