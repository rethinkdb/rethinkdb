// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_meta_client.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_manager/multi_table_manager.hpp"
#include "concurrency/cross_thread_signal.hpp"

table_meta_client_t::table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        multi_table_manager_t *_multi_table_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory) :
    mailbox_manager(_mailbox_manager),
    multi_table_manager(_multi_table_manager),
    multi_table_manager_directory(_multi_table_manager_directory),
    table_manager_directory(_table_manager_directory),
    table_basic_configs(multi_table_manager->get_table_basic_configs())
    { }

void table_meta_client_t::find(
        const database_id_t &database,
        const name_string_t &name,
        namespace_id_t *table_id_out,
        std::string *primary_key_out)
        THROWS_ONLY(no_such_table_exc_t, ambiguous_table_exc_t) {
    size_t count = 0;
    table_basic_configs.get_watchable()->read_all(
        [&](const namespace_id_t &key, const timestamped_basic_config_t *value) {
            if (value->first.database == database && value->first.name == name) {
                ++count;
                *table_id_out = key;
                if (primary_key_out != nullptr) {
                    *primary_key_out = value->first.primary_key;
                }
            }
        });
    if (count == 0) {
        throw no_such_table_exc_t();
    } else if (count >= 2) {
        throw ambiguous_table_exc_t();
    }
}

bool table_meta_client_t::exists(const namespace_id_t &table_id) {
    bool found = false;
    table_basic_configs.get_watchable()->read_key(table_id,
        [&](const timestamped_basic_config_t *value) {
            found = (value != nullptr);
        });
    return found;
}

bool table_meta_client_t::exists(
        const database_id_t &database, const name_string_t &name) {
    bool found = false;
    table_basic_configs.get_watchable()->read_all(
        [&](const namespace_id_t &, const timestamped_basic_config_t *value) {
            if (value->first.database == database && value->first.name == name) {
                found = true;
            }
        });
    return found;
}

void table_meta_client_t::get_name(
        const namespace_id_t &table_id,
        table_basic_config_t *basic_config_out)
        THROWS_ONLY(no_such_table_exc_t) {
    table_basic_configs.get_watchable()->read_key(table_id,
        [&](const timestamped_basic_config_t *value) {
            if (value == nullptr) {
                throw no_such_table_exc_t();
            }
            *basic_config_out = value->first;
        });
}

void table_meta_client_t::list_names(
        std::map<namespace_id_t, table_basic_config_t> *names_out) {
    table_basic_configs.get_watchable()->read_all(
        [&](const namespace_id_t &table_id, const timestamped_basic_config_t *value) {
            (*names_out)[table_id] = value->first;
        });
}

void table_meta_client_t::get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        table_config_and_shards_t *config_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
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
        throw_appropriate_exception(table_id);
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
        throw_appropriate_exception(table_id);
    }
    guarantee(maybe_result.size() == 1);
    *config_out = maybe_result.at(table_id);
}

void table_meta_client_t::list_configs(
        signal_t *interruptor_on_caller,
        std::map<namespace_id_t, table_config_and_shards_t> *configs_out)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t) {
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

    /* Make sure we contacted at least one server for every table */
    multi_table_manager->get_table_basic_configs()->read_all(
        [&](const namespace_id_t &table_id, const timestamped_basic_config_t *) {
            if (configs_out->count(table_id) == 0) {
                throw failed_table_op_exc_t();
            }
        });
}

void table_meta_client_t::get_status(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *sindex_statuses_out,
        std::map<peer_id_t, contracts_and_contract_acks_t> *contracts_and_acks_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    typedef std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
        index_statuses_t;
    typedef std::map<peer_id_t, contracts_and_contract_acks_t> contracts_and_acks_t;

    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Initialize the sindex map. We fill in the `sindex_config_t`s at this stage, but we
    set the `sindex_status_t`s to empty. The rest of this function is concerned with
    filling in the `sindex_status_t`s. */
    if (sindex_statuses_out != nullptr) {
        sindex_statuses_out->clear();
        table_config_and_shards_t config;
        get_config(table_id, &interruptor, &config);
        for (const auto &pair : config.config.sindexes) {
            (*sindex_statuses_out)[pair.first] =
                std::make_pair(pair.second, sindex_status_t());
        }
    }

    /* Collect status mailbox addresses for every single server we can see that's hosting
    data for this table. */
    std::vector<table_manager_bcard_t::get_status_mailbox_t::address_t> addresses;
    table_manager_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_manager_bcard_t *server_bcard) {
        if (key.second == table_id) {
            addresses.push_back(server_bcard->get_status_mailbox);
        }
    });

    /* Send a message to every server and collect all of the results in
       `sindex_statuses_out`, `contract_acks_t`, and `contracts_t`. */
    bool at_least_one_reply = false;
    pmap(addresses.begin(), addresses.end(),
    [&](const table_manager_bcard_t::get_status_mailbox_t::address_t &addr) {
        /* There are two things that can go wrong. One is that we'll lose contact with
        the other server; in this case `server_disconnected` will be pulsed. The other is
        that the server will stop hosting the given table; in this case `server_stopped`
        will be pulsed. */
        disconnect_watcher_t server_disconnected(mailbox_manager, addr.get_peer());
        cond_t server_stopped;
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
                ::key_subs_t bcard_subs(
            table_manager_directory, std::make_pair(addr.get_peer(), table_id),
            [&](const table_manager_bcard_t *bcard) {
                /* We check equality of `bcard->get_config_mailbox` because if the other
                server stops hosting the table and then immediately starts again, any
                messages we send will be dropped. */
                if (bcard == nullptr || !(bcard->get_status_mailbox == addr)) {
                    server_stopped.pulse_if_not_already_pulsed();
                }
            }, true);

        cond_t got_reply;
        mailbox_t<void(index_statuses_t, contracts_and_contract_acks_t)> ack_mailbox(
            mailbox_manager,
            [&](signal_t *,
                    const index_statuses_t &statuses,
                    const contracts_and_contract_acks_t &contracts_and_acks) {
                /* Make sure every sindex in the config is present in the reply from this
                server. If a sindex isn't present on this server, we set its `ready`
                field to false. */
                if (sindex_statuses_out != nullptr) {
                    for (auto &&pair : *sindex_statuses_out) {
                        auto it = statuses.find(pair.first);
                        /* Note that we treat an index with the wrong definition like a
                        missing index. */
                        if (it != statuses.end() &&
                                it->second.first == pair.second.first) {
                            pair.second.second.accum(it->second.second);
                        } else {
                            pair.second.second.ready = false;
                        }
                    }
                }

                if (contracts_and_acks_out != nullptr) {
                    contracts_and_acks_out->insert(
                        std::make_pair(addr.get_peer(), contracts_and_acks));
                }

                got_reply.pulse();
            });

        send(mailbox_manager, addr, ack_mailbox.get_address());
        wait_any_t done_cond(
            &server_disconnected, &server_stopped, &got_reply, &interruptor);
        done_cond.wait_lazily_unordered();

        /* The function succeeds as long as we get a reply from at least one server. If
        our attempt to get sindex statuses from a server fails, we silently ignore it. */
        at_least_one_reply |= got_reply.is_pulsed();
    });

    if (interruptor.is_pulsed()) {
        throw interrupted_exc_t();
    }

    if (!at_least_one_reply) {
        throw_appropriate_exception(table_id);
    }
}

void table_meta_client_t::create(
        namespace_id_t table_id,
        const table_config_and_shards_t &initial_config,
        signal_t *interruptor_on_caller)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Sanity-check that `table_id` is unique */
    multi_table_manager->get_table_basic_configs()->read_key(table_id,
        [&](const timestamped_basic_config_t *value) {
            guarantee(value == nullptr);
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

    if (bcards.empty()) {
        throw failed_table_op_exc_t();
    }

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
                multi_table_manager_bcard_t::status_t::ACTIVE,
                boost::optional<table_basic_config_t>(),
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
        throw maybe_failed_table_op_exc_t();
    }

    /* Wait until the table appears in the directory. */
    wait_until_change_visible(
        table_id,
        [](const timestamped_basic_config_t *value) { return value != nullptr; },
        &interruptor);
}

void table_meta_client_t::drop(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    /* Construct a special timestamp that supersedes all regular timestamps */
    multi_table_manager_bcard_t::timestamp_t drop_timestamp;
    drop_timestamp.epoch.timestamp = std::numeric_limits<microtime_t>::max();
    drop_timestamp.epoch.id = nil_uuid();
    drop_timestamp.log_index = std::numeric_limits<raft_log_index_t>::max();

    retry([&]() {
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

        if (bcards.empty()) {
            throw_appropriate_exception(table_id);
        }

        /* Send a message to each server. It's possible that the table will move to other
        servers while the messages are in-flight; but this is OK, since the servers will
        pass the deletion message on. */
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
                    table_id,
                    multi_table_manager_bcard_t::timestamp_t::deletion(),
                    multi_table_manager_bcard_t::status_t::DELETED,
                    boost::optional<table_basic_config_t>(),
                    boost::optional<raft_member_id_t>(),
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
            throw maybe_failed_table_op_exc_t();
        }
    });

    /* Wait until the table disappears from the directory. */
    wait_until_change_visible(
        table_id,
        [](const timestamped_basic_config_t *value) { return value == nullptr; },
        &interruptor);
}

void table_meta_client_t::set_config(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &new_config,
        signal_t *interruptor_on_caller)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    multi_table_manager_bcard_t::timestamp_t timestamp;
    retry([&]() {
        /* Find the server (if any) which is acting as leader for the table */
        uuid_u best_leader_uuid;
        table_manager_bcard_t::leader_bcard_t::set_config_mailbox_t::address_t
            best_mailbox;
        multi_table_manager_bcard_t::timestamp_t best_timestamp;
        table_manager_directory->read_all(
        [&](const std::pair<peer_id_t, namespace_id_t> &key,
                const table_manager_bcard_t *bcard) {
            if (key.second == table_id && static_cast<bool>(bcard->leader)) {
                if (best_mailbox.is_nil() ||
                        bcard->timestamp.supersedes(best_timestamp)) {
                    best_leader_uuid = bcard->leader->uuid;
                    best_mailbox = bcard->leader->set_config_mailbox;
                    best_timestamp = bcard->timestamp;
                }
            }
        });
        if (best_mailbox.is_nil()) {
            throw_appropriate_exception(table_id);
        }

        /* There are two reasons why our message might get lost. The first is that the
        other server disconnects. The second is that the other server remains connected
        but it stops being leader for that table. We detect the first with a
        `disconnect_watcher_t`. We detect the second by setting up a `key_subs_t` that
        pulses `leader_stopped` if the other server is no longer leader or if its leader
        UUID changes. */
        disconnect_watcher_t leader_disconnected(
            mailbox_manager, best_mailbox.get_peer());
        cond_t leader_stopped;
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            ::key_subs_t leader_stopped_subs(
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
            [&](signal_t *, const boost::optional<
                    multi_table_manager_bcard_t::timestamp_t> &res) {
                promise.pulse(res);
            });
        send(mailbox_manager, best_mailbox, new_config, ack_mailbox.get_address());
        wait_any_t done_cond(promise.get_ready_signal(),
            &leader_disconnected, &leader_stopped);
        wait_interruptible(&done_cond, &interruptor);
        if (!promise.get_ready_signal()->is_pulsed()) {
            throw maybe_failed_table_op_exc_t();
        }

        /* Sometimes the server will reply by indicating that something went wrong */
        boost::optional<multi_table_manager_bcard_t::timestamp_t> maybe_timestamp =
            promise.wait();
        if (!static_cast<bool>(maybe_timestamp)) {
            throw maybe_failed_table_op_exc_t();
        }
        timestamp = *maybe_timestamp;
    });

    /* We know for sure that the change has been applied; now we just need to wait until
    the change is visible in the directory before returning. The naive thing is to wait
    until the table's name and database match whatever we just changed the config to. But
    this could go wrong if the table's name and database are changed again in quick
    succession. We detect that other case by using the timestamps. */
    wait_until_change_visible(
        table_id,
        [&](const timestamped_basic_config_t *value) {
            return value == nullptr || value->second.supersedes(timestamp) ||
                (value->first.name == new_config.config.basic.name &&
                    value->first.database == new_config.config.basic.database);
        },
        &interruptor);
}

void table_meta_client_t::retry(const std::function<void()> &fun) {
    static const int max_tries = 5;
    static const int initial_wait_ms = 300;
    int tries_left = max_tries;
    int wait_ms = initial_wait_ms;
    bool maybe_succeeded = false;
    for (;;) {
        try {
            fun();
            return;
        } catch (const failed_table_op_exc_t &) {
            /* ignore */
        } catch (const maybe_failed_table_op_exc_t &) {
            maybe_succeeded = true;
        }
        --tries_left;
        if (tries_left == 0) {
            if (maybe_succeeded) {
                /* Throw `maybe_failed_table_op_exc_t` if any of the tries threw
                `maybe_failed_table_op_exc_t`. */
                throw maybe_failed_table_op_exc_t();
            } else {
                throw failed_table_op_exc_t();
            }
        }
        nap(wait_ms);
        wait_ms *= 1.5;
    }
}

NORETURN void table_meta_client_t::throw_appropriate_exception(
        const namespace_id_t &table_id)
        THROWS_ONLY(no_such_table_exc_t, failed_table_op_exc_t) {
    multi_table_manager->get_table_basic_configs()->read_key(
        table_id,
        [&](const timestamped_basic_config_t *value) {
            if (value == nullptr) {
                throw no_such_table_exc_t();
            } else {
                throw failed_table_op_exc_t();
            }
        });
    unreachable();
}

void table_meta_client_t::wait_until_change_visible(
        const namespace_id_t &table_id,
        const std::function<bool(const timestamped_basic_config_t *)> &cb,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, maybe_failed_table_op_exc_t)
{
    signal_timer_t timeout;
    timeout.start(10*1000);
    wait_any_t interruptor_combined(interruptor, &timeout);
    try {
        multi_table_manager->get_table_basic_configs()->run_key_until_satisfied(
            table_id, cb, &interruptor_combined);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            /* The timeout ran out. We know the change was applied, but it isn't visible
            to us yet, so we error anyway to preserve the guarantee that changes should
            be visible after the operation completes. */
            throw maybe_failed_table_op_exc_t();
        }
    }
    /* Wait until the change is also visible on other threads */
    table_basic_configs.flush();
}

