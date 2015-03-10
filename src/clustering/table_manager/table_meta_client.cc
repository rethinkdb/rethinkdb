// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_meta_client.hpp"

#include "clustering/generic/raft_core.tcc"
#include "concurrency/cross_thread_signal.hpp"

table_meta_client_t::table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory) :
    mailbox_manager(_mailbox_manager),
    multi_table_manager_directory(_multi_table_manager_directory),
    table_manager_directory(_table_manager_directory),
    table_metadata_by_id(&table_metadata_by_id_var),
    table_manager_directory_subs(table_manager_directory,
        std::bind(&table_meta_client_t::on_directory_change, this, ph::_1, ph::_2),
        true)
    { }

table_meta_client_t::find_res_t table_meta_client_t::find(
        const database_id_t &database,
        const name_string_t &name,
        namespace_id_t *table_id_out,
        std::string *primary_key_out) {
    size_t count = 0;
    table_metadata_by_id.get_watchable()->read_all(
        [&](const namespace_id_t &key, const table_metadata_t *value) {
            if (value->database == database && value->name == name) {
                ++count;
                *table_id_out = key;
                if (primary_key_out != nullptr) {
                    *primary_key_out = value->primary_key;
                }
            }
        });
    if (count == 0) {
        return find_res_t::none;
    } else if (count == 1) {
        return find_res_t::ok;
    } else {
        return find_res_t::multiple;
    }
}

bool table_meta_client_t::get_name(
        const namespace_id_t &table_id,
        database_id_t *db_out,
        name_string_t *name_out) {
    bool ok;
    table_metadata_by_id.get_watchable()->read_key(table_id,
        [&](const table_metadata_t *metadata) {
            if (metadata == nullptr) {
                ok = false;
            } else {
                ok = true;
                *db_out = metadata->database;
                *name_out = metadata->name;
            }
        });
    return ok;
}

void table_meta_client_t::list_names(
        std::map<namespace_id_t, std::pair<database_id_t, name_string_t> > *names_out) {
    table_metadata_by_id.get_watchable()->read_all(
        [&](const namespace_id_t &table_id, const table_metadata_t *metadata) {
            (*names_out)[table_id] = std::make_pair(metadata->database, metadata->name);
        });
}

bool table_meta_client_t::get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        table_config_and_shards_t *config_out) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Find a mailbox of a server that claims to be hosting the given table */
    multi_table_manager_bcard_t::get_config_mailbox_t::address_t best_mailbox;
    multi_table_manager_bcard_t::timestamp_t best_timestamp;
    table_manager_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_manager_bcard_t *table_bcard) {
        if (key.second == table_id) {
            multi_table_manager_directory->read_key(key.first,
            [&](const multi_table_manager_bcard_t *server_bcard) {
                if (server_bcard != nullptr) {
                    if (best_mailbox.is_nil() ||
                            table_bcard->timestamp.supersedes(best_timestamp)) {
                        best_mailbox = server_bcard->get_config_mailbox;
                        best_timestamp = table_bcard->timestamp;
                    }
                }
            });
        }
    });
    if (best_mailbox.is_nil()) {
        return false;
    }

    /* Send a request to the server we found */
    disconnect_watcher_t dw(mailbox_manager, best_mailbox.get_peer());
    promise_t<std::map<namespace_id_t, table_config_and_shards_t> > promise;
    mailbox_t<void(std::map<namespace_id_t, table_config_and_shards_t>)> ack_mailbox(
        mailbox_manager,
        [&](signal_t *,
                const std::map<namespace_id_t, table_config_and_shards_t> &configs) {
            promise.pulse(configs);
        });
    send(mailbox_manager, best_mailbox,
        boost::make_optional(table_id), ack_mailbox.get_address());
    wait_any_t done_cond(promise.get_ready_signal(), &dw);
    wait_interruptible(&done_cond, &interruptor);
    std::map<namespace_id_t, table_config_and_shards_t> maybe_result = promise.wait();
    if (maybe_result.empty()) {
        return false;
    }
    guarantee(maybe_result.size() == 1);
    *config_out = maybe_result.at(table_id);
    return true;
}

void table_meta_client_t::list_configs(
        signal_t *interruptor_on_caller,
        std::map<namespace_id_t, table_config_and_shards_t> *configs_out) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    configs_out->clear();

    /* Collect mailbox addresses for every single server we can see */
    std::vector<multi_table_manager_bcard_t::get_config_mailbox_t::address_t>
        addresses;
    multi_table_manager_directory->read_all(
    [&](const peer_id_t &, const multi_table_manager_bcard_t *server_bcard) {
        addresses.push_back(server_bcard->get_config_mailbox);
    });

    /* Send a message to every server and collect all of the results */
    pmap(addresses.begin(), addresses.end(),
    [&](const multi_table_manager_bcard_t::get_config_mailbox_t::address_t &a) {
        disconnect_watcher_t dw(mailbox_manager, a.get_peer());
        promise_t<std::map<namespace_id_t, table_config_and_shards_t> > promise;
        mailbox_t<void(std::map<namespace_id_t, table_config_and_shards_t>)> ack_mailbox(
        mailbox_manager,
        [&](signal_t *,
                const std::map<namespace_id_t, table_config_and_shards_t> &configs) {
            promise.pulse(configs);
        });
        send(mailbox_manager, a,
            boost::optional<namespace_id_t>(), ack_mailbox.get_address());
        wait_any_t done_cond(promise.get_ready_signal(), &dw, &interruptor);
        done_cond.wait_lazily_unordered();
        if (promise.get_ready_signal()->is_pulsed()) {
            std::map<namespace_id_t, table_config_and_shards_t> maybe_result =
                promise.wait();
            configs_out->insert(maybe_result.begin(), maybe_result.end());
        }
    });

    /* The `pmap()` above will abort early without throwing anything if the interruptor
    is pulsed, so we have to throw the exception here */
    if (interruptor.is_pulsed()) {
        throw interrupted_exc_t();
    }
}

bool table_meta_client_t::create(
        namespace_id_t table_id,
        const table_config_and_shards_t &initial_config,
        signal_t *interruptor_on_caller) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Sanity-check that `table_id` is unique */
    table_metadata_by_id_var.read_key(table_id,
        [&](const table_metadata_t *metadata) {
            guarantee(metadata == nullptr);
        });

    /* Prepare the message that we'll be sending to each server */
    multi_table_manager_bcard_t::timestamp_t timestamp;
    timestamp.epoch.timestamp = current_microtime();
    timestamp.epoch.id = generate_uuid();
    timestamp.log_index = 0;

    std::set<server_id_t> servers;
    for (const table_config_t::shard_t &shard : initial_config.config.shards) {
        servers.insert(shard.replicas.begin(), shard.replicas.end());
    }

    table_raft_state_t raft_state = make_new_table_raft_state(initial_config);

    raft_config_t raft_config;
    for (const server_id_t &server_id : servers) {
        raft_config.voting_members.insert(raft_state.member_ids.at(server_id));
    }
    raft_persistent_state_t<table_raft_state_t> raft_ps =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            raft_state, raft_config);

    /* Find the business cards of the servers we'll be sending to */
    std::map<server_id_t, multi_table_manager_bcard_t> bcards;
    multi_table_manager_directory->read_all(
        [&](const peer_id_t &, const multi_table_manager_bcard_t *bc) {
            if (servers.count(bc->server_id) == 1) {
                bcards[bc->server_id] = *bc;
            }
        });

    size_t num_acked = 0;
    pmap(bcards.begin(), bcards.end(),
    [&](const std::pair<server_id_t, multi_table_manager_bcard_t> &pair) {
        try {
            /* Send the message for the server and wait for a reply */
            disconnect_watcher_t dw(mailbox_manager,
                pair.second.action_mailbox.get_peer());
            cond_t got_ack;
            mailbox_t<void()> ack_mailbox(mailbox_manager,
                [&](signal_t *) { got_ack.pulse(); });
            send(mailbox_manager, pair.second.action_mailbox,
                table_id,
                timestamp,
                false,
                boost::optional<raft_member_id_t>(raft_state.member_ids.at(pair.first)),
                boost::optional<raft_persistent_state_t<table_raft_state_t> >(raft_ps),
                ack_mailbox.get_address());
            wait_any_t interruptor_combined(&dw, &interruptor);
            wait_interruptible(&got_ack, &interruptor_combined);

            ++num_acked;
        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    });
    if (interruptor.is_pulsed()) {
        throw interrupted_exc_t();
    }

    if (num_acked == 0) {
        return false;
    }

    /* Wait until the table appears in the directory. It may never appear in the
    directory if it's deleted or we lose contact immediately after the table is
    created; this is why we have a timeout. */
    signal_timer_t timeout;
    timeout.start(10*1000);
    wait_any_t interruptor_combined(&interruptor, &timeout);
    try {
        table_metadata_by_id_var.run_key_until_satisfied(table_id,
            [](const table_metadata_t *m) { return m != nullptr; },
            &interruptor_combined);
    } catch (const interrupted_exc_t &) {
        if (interruptor.is_pulsed()) {
            throw;
        } else {
            return false;
        }
    }
    table_metadata_by_id.flush();
    return true;
}

bool table_meta_client_t::drop(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Construct a special timestamp that supersedes all regular timestamps */
    multi_table_manager_bcard_t::timestamp_t drop_timestamp;
    drop_timestamp.epoch.timestamp = std::numeric_limits<microtime_t>::max();
    drop_timestamp.epoch.id = nil_uuid();
    drop_timestamp.log_index = std::numeric_limits<raft_log_index_t>::max();

    /* Find all servers that are hosting the table */
    std::map<server_id_t, multi_table_manager_bcard_t> bcards;
    table_manager_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_manager_bcard_t *) {
        if (key.second == table_id) {
            multi_table_manager_directory->read_key(key.first,
            [&](const multi_table_manager_bcard_t *bc) {
                if (bc != nullptr) {
                    bcards[bc->server_id] = *bc;
                }
            });
        }
    });

    /* Send a message to each server. It's possible that the table will move to other
    servers while the messages are in-flight; but this is OK, since the servers will pass
    the deletion message on. */
    size_t num_acked = 0;
    pmap(bcards.begin(), bcards.end(),
    [&](const std::pair<server_id_t, multi_table_manager_bcard_t> &pair) {
        try {
            disconnect_watcher_t dw(mailbox_manager,
                pair.second.action_mailbox.get_peer());
            cond_t got_ack;
            mailbox_t<void()> ack_mailbox(mailbox_manager,
                [&](signal_t *) { got_ack.pulse(); });
            send(mailbox_manager, pair.second.action_mailbox,
                table_id, drop_timestamp, true, boost::optional<raft_member_id_t>(),
                boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
                ack_mailbox.get_address());
            wait_any_t interruptor_combined(&dw, &interruptor);
            wait_interruptible(&got_ack, &interruptor_combined);
            ++num_acked;
        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    });
    if (interruptor.is_pulsed()) {
        throw interrupted_exc_t();
    }

    if (num_acked == 0) {
        return false;
    }

    /* Wait until the table disappears from the directory. */
    signal_timer_t timeout;
    timeout.start(10*1000);
    wait_any_t interruptor_combined(&interruptor, &timeout);
    try {
        table_metadata_by_id_var.run_key_until_satisfied(table_id,
            [](const table_metadata_t *m) { return m == nullptr; },
            &interruptor_combined);
    } catch (const interrupted_exc_t &) {
        if (interruptor.is_pulsed()) {
            throw;
        } else {
            return false;
        }
    }
    table_metadata_by_id.flush();
    return true;
}

bool table_meta_client_t::set_config(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &new_config,
        signal_t *interruptor_on_caller) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Find the server (if any) which is acting as leader for the table */
    uuid_u best_leader_uuid;
    table_manager_bcard_t::leader_bcard_t::set_config_mailbox_t::address_t best_mailbox;
    multi_table_manager_bcard_t::timestamp_t best_timestamp;
    table_manager_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_manager_bcard_t *bcard) {
        if (key.second == table_id && static_cast<bool>(bcard->leader)) {
            if (best_mailbox.is_nil() || bcard->timestamp.supersedes(best_timestamp)) {
                best_leader_uuid = bcard->leader->uuid;
                best_mailbox = bcard->leader->set_config_mailbox;
                best_timestamp = bcard->timestamp;
            }
        }
    });
    if (best_mailbox.is_nil()) {
        return false;
    }

    /* There are two reasons why our message might get lost. The first is that the other
    server disconnects. The second is that the other server remains connected but it
    stops being leader for that table. We detect the first with a `disconnect_watcher_t`.
    We detect the second by setting up a `key_subs_t` that pulses `leader_stopped` if the
    other server is no longer leader or if its leader UUID changes. */
    disconnect_watcher_t leader_disconnected(mailbox_manager, best_mailbox.get_peer());
    cond_t leader_stopped;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>::key_subs_t
        leader_stopped_subs(
            table_manager_directory,
            std::make_pair(best_mailbox.get_peer(), table_id),
            [&](const table_manager_bcard_t *bcard) {
                if (!static_cast<bool>(bcard->leader) ||
                        bcard->leader->uuid != best_leader_uuid) {
                    leader_stopped.pulse_if_not_already_pulsed();
                }
            });

    /* OK, now send the change and wait for a reply, or for something to go wrong */
    promise_t<boost::optional<multi_table_manager_bcard_t::timestamp_t> > promise;
    mailbox_t<void(boost::optional<multi_table_manager_bcard_t::timestamp_t>)>
        ack_mailbox(mailbox_manager,
        [&](signal_t *, boost::optional<multi_table_manager_bcard_t::timestamp_t> res) {
            promise.pulse(res);
        });
    send(mailbox_manager, best_mailbox, new_config, ack_mailbox.get_address());
    wait_any_t done_cond(promise.get_ready_signal(),
        &leader_disconnected, &leader_stopped);
    wait_interruptible(&done_cond, &interruptor);
    if (!promise.get_ready_signal()->is_pulsed()) {
        return false;
    }

    /* Sometimes the server will reply by indicating that something went wrong */
    boost::optional<multi_table_manager_bcard_t::timestamp_t> timestamp = promise.wait();
    if (!static_cast<bool>(timestamp)) {
        return false;
    }

    /* We know for sure that the change has been applied; now we just need to wait until
    the change is visible in the directory before returning. The naive thing is to wait
    until the table's name and database match whatever we just changed the config to. But
    this could go wrong if the table's name and database are changed again in quick
    succession. We detect that other case by using the timestamps. */
    signal_timer_t timeout;
    timeout.start(10*1000);
    wait_any_t interruptor_combined(&interruptor, &timeout);
    try {
        table_metadata_by_id_var.run_key_until_satisfied(table_id,
            [&](const table_metadata_t *m) {
                return m == nullptr || m->timestamp.supersedes(*timestamp) ||
                    (m->name == new_config.config.name &&
                        m->database == new_config.config.database);
            },
            &interruptor_combined);
    } catch (const interrupted_exc_t &) {
        if (interruptor.is_pulsed()) {
            throw;
        } else {
            /* We know the change was applied, but it isn't visible in the directory, so
            we return `false` anyway. */
            return false;
        }
    }

    table_metadata_by_id.flush();
    return true;
}

void table_meta_client_t::on_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_manager_bcard_t *dir_value) {
    table_metadata_by_id_var.change_key(key.second,
    [&](bool *md_exists, table_metadata_t *md_value) -> bool {
        if (dir_value != nullptr && !*md_exists) {
            *md_exists = true;
            md_value->witnesses = std::set<peer_id_t>({key.first});
            md_value->database = dir_value->database;
            md_value->name = dir_value->name;
            md_value->primary_key = dir_value->primary_key;
            md_value->timestamp = dir_value->timestamp;
        } else if (dir_value != nullptr && *md_exists) {
            md_value->witnesses.insert(key.first);
            if (dir_value->timestamp.supersedes(md_value->timestamp)) {
                md_value->database = dir_value->database;
                md_value->name = dir_value->name;
                md_value->timestamp = dir_value->timestamp;
            }
        } else {
            if (*md_exists) {
                md_value->witnesses.erase(key.first);
                if (md_value->witnesses.empty()) {
                    *md_exists = false;
                }
            }
        }
        return true;
    });
}

