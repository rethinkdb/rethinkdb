// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/logs/logs_backend.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "rdb_protocol/pseudo_time.hpp"

static const int entries_per_server = 1000;

ql::datum_t convert_timespec_to_datum(const timespec &t) {
    return ql::pseudo::make_time(
        t.tv_sec + static_cast<double>(t.tv_nsec) / BILLION, "+00:00");
}

ql::datum_t convert_timespec_duration_to_datum(const timespec &t) {
    return ql::datum_t(t.tv_sec + static_cast<double>(t.tv_nsec) / BILLION);
}

ql::datum_t convert_log_key_to_datum(const timespec &ts, const server_id_t &si) {
    ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
    id_builder.add(ql::datum_t(datum_string_t(
        format_time(ts, local_or_utc_time_t::utc))));
    id_builder.add(convert_uuid_to_datum(si));
    return std::move(id_builder).to_datum();
}

bool convert_log_key_from_datum(const ql::datum_t &d,
        timespec *ts_out, server_id_t *si_out, admin_err_t *error_out) {
    if (d.get_type() != ql::datum_t::R_ARRAY || d.arr_size() != 2) {
        *error_out = admin_err_t{
            "Expected two-element array, got:" + d.print(),
            query_state_t::FAILED};
        return false;
    }
    if (d.get(0).get_type() != ql::datum_t::R_STR) {
        *error_out = admin_err_t{
            "Expected string, got:" + d.print(),
            query_state_t::FAILED};
        return false;
    }
    std::string err;
    if (!parse_time(d.get(0).as_str().to_std(), local_or_utc_time_t::utc,
            ts_out, &err)) {
        *error_out = admin_err_t{"In timestamp: " + err, query_state_t::FAILED};
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
        admin_err_t *error_out) {
    return read_all_rows_raw(
        [&](const log_message_t &message,
                const peer_id_t &,
                const server_id_t &server_id,
                const ql::datum_t &server_name_datum) {
            rows_out->push_back(convert_log_message_to_datum(
                message, server_id, server_name_datum));
        },
        interruptor,
        error_out);
}

bool logs_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        admin_err_t *error_out) {
    timespec timestamp;
    server_id_t server_id;
    admin_err_t dummy_error;
    if (!convert_log_key_from_datum(primary_key, &timestamp, &server_id, &dummy_error)) {
        *row_out = ql::datum_t();
        return true;
    }

    ql::datum_t server_datum;
    name_string_t server_name;
    if (!convert_connected_server_id_to_datum(server_id, identifier_format,
            server_config_client, &server_datum, &server_name)) {
        /* The server is not currently connected */
        *row_out = ql::datum_t();
        return true;
    }

    boost::optional<peer_id_t> peer_id =
        server_config_client->get_server_to_peer_map()->get_key(server_id);
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
        if the server is connecting or disconnecting, and the `server_config_client` is
        out of sync with the directory. */
        *row_out = ql::datum_t();
        return true;
    }

    std::vector<log_message_t> messages;
    try {
        /* The timestamp filter is set so that we'll only get messages with the exact
        timestamp we're looking for, and there should be at most one such message. */
        messages = fetch_log_file(mailbox_manager, *bcard,
            entries_per_server, timestamp, timestamp, interruptor);
    } catch (const log_transfer_exc_t &) {
        /* Server disconnected during the query. */
        *row_out = ql::datum_t();
        return true;
    } catch (const log_read_exc_t &e) {
        *error_out = admin_err_t{
            strprintf("Problem when reading log file on server `%s`: %s",
                      server_name.c_str(), e.what()),
            query_state_t::FAILED};
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
        *error_out = admin_err_t{
            strprintf("Problem when reading log file on server `%s`: Found "
                      "multiple log entries with identical timestamps.",
                      server_name.c_str()),
            query_state_t::FAILED};
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
        admin_err_t *error_out) {
    *error_out = admin_err_t{
        "It's illegal to write to the `rethinkdb.logs` system table.",
        query_state_t::FAILED};
    return false;
}

logs_artificial_table_backend_t::cfeed_machinery_t::cfeed_machinery_t(
        logs_artificial_table_backend_t *_parent) :
    parent(_parent),
    starting(true),
    num_starters_left(0),
    dir_subs(
        parent->directory,
        std::bind(&cfeed_machinery_t::on_change, this, ph::_1, ph::_2),
        initial_call_t::YES)
{
    starting = false;
    /* In the unlikely event that we're not connected to any servers (not even ourself)
    there will be nothing to pulse `all_starters_done`, so we have to do it here. */
    if (num_starters_left == 0) {
        all_starters_done.pulse_if_not_already_pulsed();
    }
}

void logs_artificial_table_backend_t::cfeed_machinery_t::on_change(
        const peer_id_t &peer,
        const cluster_directory_metadata_t *dir) {
    if (dir == nullptr || peers_handled.count(peer) != 0) {
        return;
    }
    peers_handled.insert(peer);
    if (starting) {
        ++num_starters_left;
    }
    coro_t::spawn_sometime(std::bind(
        &cfeed_machinery_t::run, this,
        peer, dir->server_id, dir->log_mailbox, starting,
        auto_drainer_t::lock_t(&drainer)));
}

void logs_artificial_table_backend_t::cfeed_machinery_t::run(
        const peer_id_t &peer,
        const server_id_t &server_id,
        const log_server_business_card_t &bcard,
        bool is_a_starter,
        auto_drainer_t::lock_t keepalive) {
    guarantee(!starting, "starting should be set to false before run() actually starts");

    /* `poll_interval_ms` is how long to wait between polling for new messages. */
    static const int poll_interval_ms = 1000;

    try {
        /* First, fetch the initial value of the latest timestamp in the log. The reason
        this is in a loop is so that we keep retrying if something goes wrong: the log
        file is empty, we can't read it for some reason, etc. */
        timespec initial_latest_timestamp;
        while (true) {
            if (!check_disconnected(peer)) {
                /* The peer is disconnected, so this instance of `run()` is going to
                exit. So we have to check this here because we're not going to get to it
                later. */
                if (is_a_starter) {
                    guarantee(num_starters_left > 0);
                    --num_starters_left;
                    if (num_starters_left == 0) {
                        all_starters_done.pulse();
                    }
                }
                return;
            }

            /* Fetch the last message in the server's log file */
            std::vector<log_message_t> messages;
            try {
                timespec min_time = { 0, 0 };
                timespec max_time = { std::numeric_limits<time_t>::max(), 0 };
                messages = fetch_log_file(
                    parent->mailbox_manager,
                    bcard,
                    1,   /* only fetch latest entry */
                    min_time,
                    max_time,
                    keepalive.get_drain_signal());
            } catch (const log_transfer_exc_t &) {
                /* The server disconnected. However, to avoid race conditions, we can't
                exit unless we see that the server is absent from the directory, which we
                check above. So we ignore the error and go around the loop again. */
            } catch (const log_read_exc_t &) {
                /* Something went wrong reading the log file on the other server. Go
                around the loop again. */
            }

            if (messages.empty()) {
                /* This could mean that the log file is empty, or that an error
                occurred. In either case, retry after a short delay. */
                nap(poll_interval_ms, keepalive.get_drain_signal());
                continue;
            }

            guarantee(messages.size() == 1, "We asked for at most 1 log message.");
            initial_latest_timestamp = messages[0].timestamp;
            break;
        }

        map_insertion_sentry_t<peer_id_t, timespec> last_timestamp(
            &last_timestamps, peer, initial_latest_timestamp);

        /* Now that we've fetched the initial timestamp, we can let the call to
        `.changes()` return */
        if (is_a_starter) {
            guarantee(num_starters_left > 0);
            --num_starters_left;
            if (num_starters_left == 0) {
                all_starters_done.pulse();
            }
        }

        /* Loop forever to check for new messages. */
        while (true) {
            if (!check_disconnected(peer)) {
                return;
            }

            /* Fetch messages since our last request */
            std::vector<log_message_t> messages;
            try {
                /* We choose `min_time` so as to exclude the last message from before */
                timespec min_time = *last_timestamp.get_value();
                add_to_timespec(&min_time, 1);
                timespec max_time = { std::numeric_limits<time_t>::max(), 0 };
                messages = fetch_log_file(
                    parent->mailbox_manager,
                    bcard,
                    /* We might miss some notifications if more than `entries_per_server`
                    entries are appended to the log file in one iteration of the loop.
                    But this table already "cheats" regarding the relationship between
                    the contents of the table and the changefeed, so it's no big deal. */
                    entries_per_server,
                    min_time,
                    max_time,
                    keepalive.get_drain_signal());
            } catch (const log_transfer_exc_t &) {
                /* Just like in the earlier loop, we ignore the error and rely on
                `check_disconnected()` to do the work. */
            } catch (const log_read_exc_t &) {
                /* Just like in the earlier loop. */
            }

            if (!messages.empty()) {
                /* Compute the server name to attach to the log messages */
                ql::datum_t server_datum;
                if (!convert_connected_server_id_to_datum(server_id,
                        parent->identifier_format, parent->server_config_client,
                        &server_datum, nullptr)) {
                    /* The server is disconnected. Don't retrieve log messages until it
                    reconnects. */
                    peers_handled.erase(peer);
                    return;
                }

                new_mutex_acq_t mutex_acq(&mutex, keepalive.get_drain_signal());

                for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
                    /* It's possible that `get_initial_values()` has changed
                    `last_timestamp` since we started fetching the log file. We need to
                    filter out any entries that started after `last_timestamp`. */
                    if (it->timestamp <= *last_timestamp.get_value()) {
                        continue;
                    }
                    *last_timestamp.get_value() = it->timestamp;

                    ql::datum_t row = convert_log_message_to_datum(
                        *it, server_id, server_datum);
                    store_key_t key(
                        convert_log_key_to_datum(
                            it->timestamp,
                            server_id
                        ).print_primary());
                    send_all_change(&mutex_acq, key, ql::datum_t(), row);
                }
            }

            nap(poll_interval_ms, keepalive.get_drain_signal());
        }

    } catch (const interrupted_exc_t &) {
        /* we're shutting down, do nothing */
    }
}

bool logs_artificial_table_backend_t::cfeed_machinery_t::check_disconnected(
        const peer_id_t &peer) {
    /* We have to do this atomically. If we don't, then we would lose the guarantee that
    there is exactly one instance of `run()` for each connected peer. For example, if we
    delayed between checking the directory and removing `peer` from `peers_handled`, and
    the server reconnected in that time, then `on_change()` wouldn't spawn a new instance
    of `run()`, but we would still end up returning `false`, so there would be no
    instance of `run()` for that server. */
    ASSERT_FINITE_CORO_WAITING;

    bool connected;
    parent->directory->read_key(
        peer,
        [&](const cluster_directory_metadata_t *metadata) {
            connected = (metadata != nullptr);
        });
    if (!connected) {
        peers_handled.erase(peer);
    }
    return connected;
}

bool logs_artificial_table_backend_t::cfeed_machinery_t::get_initial_values(
        const new_mutex_acq_t *proof,
        std::vector<ql::datum_t> *initial_values_out,
        signal_t *interruptor) {
    admin_err_t dummy_error;
    return parent->read_all_rows_raw(
        [&](const log_message_t &message,
                const peer_id_t &peer_id,
                const server_id_t &server_id,
                const ql::datum_t &server_name_datum) {
            ql::datum_t row = convert_log_message_to_datum(
                message, server_id, server_name_datum);
            initial_values_out->push_back(row);

            auto it = last_timestamps.find(peer_id);
            if (it != last_timestamps.end() && it->second < message.timestamp) {
                it->second = message.timestamp;
                store_key_t key(convert_log_key_to_datum(
                    message.timestamp, server_id).print_primary());
                send_all_change(proof, key, ql::datum_t(), row);
            }
        },
        interruptor,
        &dummy_error);
}

bool logs_artificial_table_backend_t::read_all_rows_raw(
        const std::function<void(
            const log_message_t &message,
            const peer_id_t &peer_id,
            const server_id_t &server_id,
            const ql::datum_t &server_name_datum)> &callback,
        signal_t *interruptor,
        admin_err_t *error_out) {
    std::map<peer_id_t, std::pair<name_string_t, server_id_t>> server_names;
    std::map<peer_id_t, log_server_business_card_t> server_business_cards;
    directory->read_all(
        [&](const peer_id_t &peer_id, const cluster_directory_metadata_t *value) {
            if (value->peer_type == SERVER_PEER) {
                server_names.insert(std::make_pair(
                    peer_id,
                    std::make_pair(value->server_config.config.name, value->server_id)));
                server_business_cards.insert(std::make_pair(
                    peer_id, value->log_mailbox));
            }
        });

    boost::optional<std::string> error;
    pmap(server_business_cards.begin(), server_business_cards.end(),
        [&](const std::pair<peer_id_t, log_server_business_card_t> &pair) {
            auto server_name = server_names.find(pair.first);
            guarantee(server_name != server_names.end());

            std::vector<log_message_t> messages;
            try {
                struct timespec min_time = { 0, 0 };
                struct timespec max_time = { std::numeric_limits<time_t>::max(), 0 };
                messages = fetch_log_file(
                    mailbox_manager,
                    pair.second,
                    entries_per_server,
                    min_time,
                    max_time,
                    interruptor);
            } catch (const interrupted_exc_t &) {
                /* We'll deal with it outside the `pmap()` */
                return;
            } catch (const log_transfer_exc_t &) {
                /* The server disconnected. Ignore it. */
                return;
            } catch (const log_read_exc_t &e) {
                /* We'll deal with it outside the `pmap()` */
                error = strprintf("Problem with reading log file on server `%s`: %s",
                    server_name->second.first.c_str(), e.what());
                return;
            }

            ql::datum_t server_name_datum = convert_name_or_uuid_to_datum(
                server_name->second.first,
                server_name->second.second,
                identifier_format);
            for (const log_message_t &message : messages) {
                callback(
                    message,
                    pair.first,
                    server_name->second.second,
                    server_name_datum);
            }
        });

    /* We can't throw exceptions or `return false` from within the `pmap()`, so we have
    to do it here instead. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    } else if (static_cast<bool>(error)) {
        *error_out = admin_err_t{*error, query_state_t::FAILED};
        return false;
    }
    return true;
}

scoped_ptr_t<cfeed_artificial_table_backend_t::machinery_t>
    logs_artificial_table_backend_t::
        construct_changefeed_machinery(signal_t *interruptor) {
    scoped_ptr_t<cfeed_machinery_t> m(new cfeed_machinery_t(this));
    wait_interruptible(&m->all_starters_done, interruptor);
    return scoped_ptr_t<cfeed_artificial_table_backend_t::machinery_t>(m.release());
}

