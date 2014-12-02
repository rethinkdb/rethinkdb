// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/logs/logs_backend.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/name_client.hpp"
#include "rdb_protocol/pseudo_time.hpp"

static const int entries_per_server = 1000;

ql::datum_t convert_timespec_to_datum(const timespec &t) {
    return ql::pseudo::make_time(
        t.tv_sec + static_cast<double>(t.tv_nsec) / BILLION, "+00:00");
}

bool convert_timespec_from_datum(
        const ql::datum_t &d, timespec *t_out, std::string *error_out) {
    if (!d.is_ptype(ql::pseudo::time_string)) {
        *error_out = "Expected a timestamp, got: " + d.print();
        return false;
    }
    double epoch_time = ql::pseudo::time_to_epoch_time(d);
    double ipart, fpart;
    fpart = modf(epoch_time, &ipart);
    t_out->tv_sec = ipart;
    t_out->tv_nsec = fpart * BILLION;
    return true;
}

ql::datum_t convert_timespec_duration_to_datum(const timespec &t) {
    return ql::datum_t(t.tv_sec + static_cast<double>(t.tv_nsec) / BILLION);
}

ql::datum_t convert_log_key_to_datum(const timespec &ts, const server_id_t &si) {
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(convert_timespec_to_datum(ts));
    id_builder.add(convert_uuid_to_datum(si));
    return std::move(id_builder).to_datum();
}

bool convert_log_key_from_datum(const ql::datum_t &d,
        timespec *ts_out, server_id_t *si_out, std::string *error_out) {
    if (d.get_type() != ql::datum_t::R_ARRAY || d.arr_size() != 2) {
        *error_out = "Expected two-element array, got:" + d.print();
        return false;
    }
    if (!convert_timespec_from_datum(d.get(0), ts_out, error_out)) {
        return false;
    }
    if (!convert_uuid_from_datum(d.get(1), si_out, error_out)) {
        return false;
    }
    return true;
}

ql::datum_t convert_log_message_to_datum(
        const log_message_t &msg, const server_id_t &server_id,
        const ql::datum_t &server_datum) {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_log_key_to_datum(msg.timestamp, server_id));
    builder.overwrite("server", server_datum);
    builder.overwrite("timestamp", convert_timespec_to_datum(msg.timestamp));
    builder.overwrite("uptime", convert_timespec_duration_to_datum(msg.uptime));
    builder.overwrite("level", ql::datum_t(datum_string_t(format_log_level(msg.level))));
    builder.overwrite("message", ql::datum_t(datum_string_t(msg.message)));
    return std::move(builder).to_datum();
}

logs_artificial_table_backend_t::~logs_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

std::string logs_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool logs_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        std::string *error_out) {
    std::map<server_id_t, log_server_business_card_t> servers;
    directory->read_all(
        [&](const peer_id_t &, const cluster_directory_metadata_t *value) {
            servers.insert(std::make_pair(value->server_id, value->log_mailbox));
        });

    boost::optional<std::string> error;
    pmap(servers.begin(), servers.end(),
        [&](const std::pair<server_id_t, log_server_business_card_t> &server) {
            ql::datum_t server_datum;
            name_string_t server_name;
            if (!convert_server_id_to_datum(server.first, identifier_format, name_client,
                    &server_datum, &server_name)) {
                /* The server was permanently removed. Don't display its log messages. */
                return;
            }
            std::vector<log_message_t> messages;
            try {
                struct timespec min_time = { 0, 0 };
                struct timespec max_time = { std::numeric_limits<time_t>::max(), 0 };
                messages = fetch_log_file(
                    mailbox_manager,
                    server.second,
                    entries_per_server,
                    min_time,
                    max_time,
                    interruptor);
            } catch (const interrupted_exc_t &) {
                /* We'll deal with it outside the `pmap()` */
                return;
            } catch (const resource_lost_exc_t &) {
                /* The server disconnected. Ignore it. */
                return;
            } catch (const std::runtime_error &e) {
                /* We'll deal with it outside the `pmap()` */
                error = strprintf("Problem with reading log file on server `%s`: %s",
                    server_name.c_str(), e.what());
                return;
            }
            for (const log_message_t &m : messages) {
                rows_out->push_back(convert_log_message_to_datum(
                    m, server.first, server_datum));
            }
        });

    /* We can't throw exceptions or `return false` from within the `pmap()`, so we have
    to do it here instead. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    } else if (static_cast<bool>(error)) {
        *error_out = *error;
        return false;
    }
    return true;
}

bool logs_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) {
    timespec timestamp;
    server_id_t server_id;
    std::string dummy_error;
    if (!convert_log_key_from_datum(primary_key, &timestamp, &server_id, &dummy_error)) {
        *row_out = ql::datum_t();
        return true;
    }

    ql::datum_t server_datum;
    name_string_t server_name;
    if (!convert_server_id_to_datum(server_id, identifier_format, name_client,
            &server_datum, &server_name)) {
        /* The server doesn't exist in the metadata */
        *row_out = ql::datum_t();
        return true;
    }

    boost::optional<peer_id_t> peer_id =
        name_client->get_peer_id_for_server_id(server_id);
    if (!static_cast<bool>(peer_id)) {
        /* Disconnected or nonexistent server, so log entries shouldn't be present in the
        table */
        *row_out = ql::datum_t();
        return true;
    }

    boost::optional<log_server_business_card_t> bcard;
    directory->read_key(*peer_id,
        [&](const cluster_directory_metadata_t *metadata) {
            if (metadata != nullptr) {
                bcard = metadata->log_mailbox;
            }
        });
    if (!static_cast<bool>(bcard)) {
        /* Server is not present in directory. This can happen due to a race condition,
        if the server is connecting or disconnecting, and the `name_client` is out of
        sync with the directory. */
        *row_out = ql::datum_t();
        return true;
    }

    /* When fetching the log entry, we want to allow a wide margin of error to avoid
    rounding errors. */
    timespec min_timespec = timestamp, max_timespec = timestamp;
    add_to_timespec(&min_timespec, -log_timestamp_interval_ns / 2);
    add_to_timespec(&max_timespec, log_timestamp_interval_ns / 2);

    std::vector<log_message_t> messages;
    try {
        messages = fetch_log_file(mailbox_manager, *bcard,
            entries_per_server, min_timespec, max_timespec, interruptor);
    } catch (const resource_lost_exc_t &) {
        /* Server disconnected during the query. */
        *row_out = ql::datum_t();
        return true;
    } catch (const std::runtime_error &e) {
        *error_out = strprintf("Problem when reading log file on server `%s`: %s",
            server_name.c_str(), e.what());
        return false;
    }

    if (messages.size() == 0) {
        /* There is no log entry with that timestamp */
        *row_out = ql::datum_t();
        return true;
    } else if (messages.size() >= 2) {
        /* This shouldn't happen unless the user tampered with the log file or the server
        clock ran backwards while the server was shut down (and even then it's very
        unlikely) */
        *error_out = strprintf("Problem when reading log file on server `%s`: Found "
            "multiple log entries with identical or similar timestamps.",
            server_name.c_str());
        return false;
    }

    *row_out = convert_log_message_to_datum(messages[0], server_id, server_datum);

    /* The `id` field should be present and mostly correct. But since the conversion
    between `timespec` and ReQL time objects is not perfect, the timestamp in the `id`
    field might not be exactly the same as the timestamp on the row the user requested.
    So we set the `id` field manually here. */
    ql::datum_object_builder_t builder(std::move(*row_out));
    builder.overwrite("id", primary_key);
    *row_out = std::move(builder).to_datum();

    return true;
}

bool logs_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb.logs` system table.";
    return false;
}

