// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_meta_client.hpp"

#include "clustering/administration/servers/config_client.hpp"
#include "clustering/generic/raft_core.tcc"
#include "clustering/table_contract/emergency_repair.hpp"
#include "clustering/table_manager/multi_table_manager.hpp"
#include "concurrency/cross_thread_signal.hpp"

table_meta_client_t::table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        multi_table_manager_t *_multi_table_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        server_config_client_t *_server_config_client) :
    mailbox_manager(_mailbox_manager),
    multi_table_manager(_multi_table_manager),
    multi_table_manager_directory(_multi_table_manager_directory),
    table_manager_directory(_table_manager_directory),
    server_config_client(_server_config_client),
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
        std::map<namespace_id_t, table_basic_config_t> *names_out) const {
    table_basic_configs.get_watchable()->read_all(
        [&](const namespace_id_t &table_id, const timestamped_basic_config_t *value) {
            (*names_out)[table_id] = value->first;
        });
}

bool supersedes_with_leader(
        const multi_table_manager_bcard_t::timestamp_t &lhs_timestamp,
        bool lhs_leader,
        const multi_table_manager_bcard_t::timestamp_t &rhs_timestamp,
        bool rhs_leader) {
    /* This returns true if `lhs.supersedes(rhs)`, lexically ordering by
    `(timestamp.epoch, leader, timestamp.log_index)`. */
    if (lhs_timestamp.epoch.supersedes(rhs_timestamp.epoch)) {
        return true;
    } else if (rhs_timestamp.epoch.supersedes(lhs_timestamp.epoch)) {
        return false;
    } else {
        if (lhs_leader > rhs_leader) {
            return true;
        } else if (rhs_leader > lhs_leader) {
            return false;
        } else {
            return lhs_timestamp.log_index > rhs_timestamp.log_index;
        }
    }
}

void table_meta_client_t::get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        table_config_and_shards_t *config_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    retry([&](signal_t *interruptor2) {
        /* Find a mailbox of a server that claims to be hosting the given table */
        multi_table_manager_bcard_t::get_config_mailbox_t::address_t best_mailbox;
        multi_table_manager_bcard_t::timestamp_t best_timestamp;
        bool best_leader = false;
        table_manager_directory->read_all(
        [&](const std::pair<peer_id_t, namespace_id_t> &key,
                const table_manager_bcard_t *table_bcard) {
            if (key.second == table_id) {
                multi_table_manager_directory->read_key(key.first,
                [&](const multi_table_manager_bcard_t *server_bcard) {
                    if (server_bcard != nullptr) {
                        if (best_mailbox.is_nil() ||
                                supersedes_with_leader(
                                    table_bcard->timestamp,
                                    static_cast<bool>(table_bcard->leader),
                                    best_timestamp,
                                    best_leader)) {
                            best_mailbox = server_bcard->get_config_mailbox;
                            best_timestamp = table_bcard->timestamp;
                            best_leader = static_cast<bool>(table_bcard->leader);
                        }
                    }
                });
            }
        });
        if (best_mailbox.is_nil()) {
            throw_appropriate_exception(table_id);
        }

        typedef std::pair<
            table_config_and_shards_t,
            multi_table_manager_bcard_t::timestamp_t> timestamped_config_and_shards_t;

        /* Send a request to the server we found */
        disconnect_watcher_t dw(mailbox_manager, best_mailbox.get_peer());
        promise_t<std::map<namespace_id_t, timestamped_config_and_shards_t> > promise;
        mailbox_t<void(std::map<namespace_id_t, timestamped_config_and_shards_t>
                )> ack_mailbox(
            mailbox_manager,
            [&](signal_t *,
                    const std::map<namespace_id_t, timestamped_config_and_shards_t>
                        &configs) {
                promise.pulse(configs);
            });
        send(mailbox_manager, best_mailbox,
            boost::make_optional(table_id), ack_mailbox.get_address());
        wait_any_t done_cond(promise.get_ready_signal(), &dw);
        wait_interruptible(&done_cond, interruptor2);
        if (!promise.is_pulsed()) {
            throw_appropriate_exception(table_id);
        }
        std::map<namespace_id_t, timestamped_config_and_shards_t> maybe_result =
            promise.assert_get_value();
        if (maybe_result.empty()) {
            throw_appropriate_exception(table_id);
        }
        guarantee(maybe_result.size() == 1);
        *config_out = maybe_result.at(table_id).first;
    }, &interruptor);
}

void table_meta_client_t::list_configs(
        signal_t *interruptor_on_caller,
        std::map<namespace_id_t, table_config_and_shards_t> *configs_out,
        std::map<namespace_id_t, table_basic_config_t> *disconnected_configs_out)
        THROWS_ONLY(interrupted_exc_t) {
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

    typedef std::pair<
        table_config_and_shards_t,
        multi_table_manager_bcard_t::timestamp_t> timestamped_config_and_shards_t;

    std::map<namespace_id_t, multi_table_manager_bcard_t::timestamp_t> timestamps;

    /* Send a message to every server and collect all of the results */
    pmap(addresses.begin(), addresses.end(),
    [&](const multi_table_manager_bcard_t::get_config_mailbox_t::address_t &a) {
        disconnect_watcher_t dw(mailbox_manager, a.get_peer());
        promise_t<std::map<namespace_id_t, timestamped_config_and_shards_t> > promise;
        mailbox_t<void(std::map<namespace_id_t, timestamped_config_and_shards_t>
            )> ack_mailbox(
        mailbox_manager,
        [&](signal_t *,
                const std::map<namespace_id_t, timestamped_config_and_shards_t>
                    &results) {
            promise.pulse(results);
        });
        send(mailbox_manager, a,
            boost::optional<namespace_id_t>(), ack_mailbox.get_address());
        wait_any_t done_cond(promise.get_ready_signal(), &dw, &interruptor);
        done_cond.wait_lazily_unordered();
        if (promise.get_ready_signal()->is_pulsed()) {
            std::map<namespace_id_t, timestamped_config_and_shards_t> maybe_results =
                promise.assert_get_value();
            for (const auto &result : maybe_results) {
                /* Insert the result if it's new, or if its timestamp supersedes the
                result we already have. */
                auto timestamp_it = timestamps.find(result.first);
                if (timestamp_it == timestamps.end() ||
                        result.second.second.supersedes(timestamp_it->second)) {
                    (*configs_out)[result.first] = std::move(result.second.first);
                    timestamps[result.first] = std::move(result.second.second);
                }
            }
        }
    });

    /* The `pmap()` above will abort early without throwing anything if the interruptor
    is pulsed, so we have to throw the exception here */
    if (interruptor.is_pulsed()) {
        throw interrupted_exc_t();
    }

    /* Make sure we contacted at least one server for every table */
    multi_table_manager->get_table_basic_configs()->read_all(
        [&](const namespace_id_t &table_id, const timestamped_basic_config_t *config) {
            if (configs_out->count(table_id) == 0) {
                disconnected_configs_out->insert(
                    std::make_pair(table_id, config->first));
            }
        });
}

void table_meta_client_t::get_status(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *sindex_statuses_out,
        std::map<server_id_t, table_server_status_t> *server_statuses_out,
        server_name_map_t *server_names_out,
        server_id_t *latest_server_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    typedef std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
        index_statuses_t;

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

    /* Collect business cards for every single server we can see that's hosting data for
    this table. */
    std::vector<table_manager_bcard_t> bcards;
    table_manager_directory->read_all(
    [&](const std::pair<peer_id_t, namespace_id_t> &key,
            const table_manager_bcard_t *server_bcard) {
        if (key.second == table_id) {
            bcards.push_back(*server_bcard);
        }
    });

    /* Send a message to every server and collect all of the results in
       `sindex_statuses_out`, `contract_acks_t`, and `contracts_t`. */
    bool at_least_one_reply = false;
    pmap(bcards.begin(), bcards.end(), [&](const table_manager_bcard_t &bcard) {
        /* There are two things that can go wrong. One is that we'll lose contact with
        the other server; in this case `server_disconnected` will be pulsed. The other is
        that the server will stop hosting the given table; in this case `server_stopped`
        will be pulsed. */
        disconnect_watcher_t server_disconnected(
            mailbox_manager, bcard.get_status_mailbox.get_peer());
        cond_t server_stopped;
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
                ::key_subs_t bcard_subs(
            table_manager_directory,
            std::make_pair(bcard.get_status_mailbox.get_peer(), table_id),
            [&](const table_manager_bcard_t *bcard2) {
                /* We check equality of `bcard->get_config_mailbox` because if the other
                server stops hosting the table and then immediately starts again, any
                messages we send will be dropped. */
                if (bcard2 == nullptr ||
                        !(bcard2->get_status_mailbox == bcard.get_status_mailbox)) {
                    server_stopped.pulse_if_not_already_pulsed();
                }
            }, initial_call_t::YES);

        cond_t got_reply;
        mailbox_t<void(index_statuses_t, table_server_status_t)> ack_mailbox(
            mailbox_manager,
            [&](signal_t *,
                    const index_statuses_t &statuses,
                    const table_server_status_t &server_statuses) {

                /* Fetch the server's name. The reason we fetch the server's name here is
                so that we want to put the name in `server_names_out` iff we put an entry
                in `server_statuses_out`. */
                boost::optional<server_config_versioned_t> config =
                    server_config_client->get_server_config_map()
                        ->get_key(bcard.server_id);
                if (!static_cast<bool>(config)) {
                    /* The server disconnected. */
                    server_stopped.pulse_if_not_already_pulsed();
                    return;
                }
                if (server_names_out != nullptr) {
                    server_names_out->names.insert(std::make_pair(bcard.server_id,
                        std::make_pair(config->version, config->config.name)));
                }

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

                if (server_statuses_out != nullptr) {
                    server_statuses_out->insert(
                        std::make_pair(bcard.server_id, server_statuses));
                }

                got_reply.pulse();
            });

        send(mailbox_manager, bcard.get_status_mailbox, ack_mailbox.get_address());
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

    /* Determine the most up-to-date server */
    if (server_statuses_out != nullptr && latest_server_out != nullptr) {
        *latest_server_out = nil_uuid();
        for (const auto &pair : *server_statuses_out) {
            if (*latest_server_out == nil_uuid() || pair.second.timestamp.supersedes(
                    server_statuses_out->at(*latest_server_out).timestamp)) {
                *latest_server_out = pair.first;
            }
        }
        guarantee(!latest_server_out->is_nil());
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

    create_or_emergency_repair(
        table_id,
        make_new_table_raft_state(initial_config),
        current_microtime(),
        &interruptor);
}

void table_meta_client_t::drop(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    if (!exists(table_id)) {
        throw no_such_table_exc_t();
    }

    /* Construct a special timestamp that supersedes all regular timestamps */
    multi_table_manager_bcard_t::timestamp_t drop_timestamp;
    drop_timestamp.epoch.timestamp = std::numeric_limits<microtime_t>::max();
    drop_timestamp.epoch.id = nil_uuid();
    drop_timestamp.log_index = std::numeric_limits<raft_log_index_t>::max();

    /* Find business cards for all servers, not just the ones that are hosting the table.
    This is because sometimes it makes sense to drop a table even if the table is
    completely unreachable. */
    std::map<peer_id_t, multi_table_manager_bcard_t> bcards =
        multi_table_manager_directory->get_all();
    guarantee(!bcards.empty(), "We should be connected to ourself");

    /* Send a message to each server. */
    size_t num_acked = 0;
    pmap(bcards.begin(), bcards.end(),
    [&](const std::pair<peer_id_t, multi_table_manager_bcard_t> &pair) {
        try {
            disconnect_watcher_t dw(mailbox_manager, pair.first);
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
    guarantee(num_acked != 0, "We should at least have an ack from ourself");

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
    retry([&](signal_t *interruptor2) {
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
        UUID changes. (The `key_subs_t` alone is not enough because it might fail to
        detect a transient connection failure.) */
        disconnect_watcher_t leader_disconnected(
            mailbox_manager, best_mailbox.get_peer());
        cond_t leader_stopped;
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            ::key_subs_t leader_stopped_subs(
                table_manager_directory,
                std::make_pair(best_mailbox.get_peer(), table_id),
                [&](const table_manager_bcard_t *bcard) {
                    if (bcard == nullptr || !static_cast<bool>(bcard->leader) ||
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
        wait_interruptible(&done_cond, interruptor2);
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
    }, &interruptor);

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

void table_meta_client_t::emergency_repair(
        const namespace_id_t &table_id,
        bool allow_erase,
        bool dry_run,
        signal_t *interruptor_on_caller,
        table_config_and_shards_t *new_config_out,
        bool *rollback_found_out,
        bool *erase_found_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    std::map<server_id_t, table_server_status_t> old_contracts;
    server_id_t latest_server;
    get_status(table_id, &interruptor, nullptr, &old_contracts, nullptr, &latest_server);

    std::set<server_id_t> dead_servers;
    for (const auto &pair : old_contracts.at(latest_server).state.member_ids) {
        if (old_contracts.count(pair.first) == 0) {
            dead_servers.insert(pair.first);
        }
    }

    table_raft_state_t new_state;
    calculate_emergency_repair(
        old_contracts.at(latest_server).state,
        dead_servers,
        allow_erase,
        &new_state,
        rollback_found_out,
        erase_found_out);

    *new_config_out = new_state.config;

    if ((*rollback_found_out || *erase_found_out) && !dry_run) {
        /* In theory, we don't always have to start a new epoch. Sometimes we run an
        emergency repair where we've lost a quorum of one shard, but still have a quorum
        of the Raft cluster as a whole. In that case we could run a regular Raft
        transaction, which could be made slightly safer. But it's simpler to do
        everything through the same code path. */

        /* Fetch the table's current epoch's timestamp to make sure that the new epoch
        has a higher timestamp, even if the server's clock is wrong. */
        microtime_t old_epoch_timestamp;
        multi_table_manager->get_table_basic_configs()->read_key(table_id,
            [&](const std::pair<table_basic_config_t,
                    multi_table_manager_bcard_t::timestamp_t> *pair) {
                if (pair == nullptr) {
                    throw no_such_table_exc_t();
                }
                old_epoch_timestamp = pair->second.epoch.timestamp;
            });

        create_or_emergency_repair(
            table_id,
            new_state,
            std::max(current_microtime(), old_epoch_timestamp + 1),
            &interruptor);
    }
}

void table_meta_client_t::create_or_emergency_repair(
        const namespace_id_t &table_id,
        const table_raft_state_t &raft_state,
        microtime_t epoch_timestamp,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    assert_thread();

    /* Prepare the message that we'll be sending to each server */
    multi_table_manager_bcard_t::timestamp_t timestamp;
    timestamp.epoch.timestamp = epoch_timestamp;
    timestamp.epoch.id = generate_uuid();
    timestamp.log_index = 0;

    std::set<server_id_t> all_servers, voting_servers;
    for (const table_config_t::shard_t &shard : raft_state.config.config.shards) {
        all_servers.insert(shard.all_replicas.begin(), shard.all_replicas.end());
        std::set<server_id_t> voters = shard.voting_replicas();
        voting_servers.insert(voters.begin(), voters.end());
    }

    raft_config_t raft_config;
    for (const server_id_t &server_id : all_servers) {
        if (voting_servers.count(server_id) == 1) {
            raft_config.voting_members.insert(raft_state.member_ids.at(server_id));
        } else {
            raft_config.non_voting_members.insert(raft_state.member_ids.at(server_id));
        }
    }

    raft_persistent_state_t<table_raft_state_t> raft_ps =
        raft_persistent_state_t<table_raft_state_t>::make_initial(
            raft_state, raft_config);

    /* Find the business cards of the servers we'll be sending to */
    std::map<server_id_t, multi_table_manager_bcard_t> bcards;
    multi_table_manager_directory->read_all(
        [&](const peer_id_t &, const multi_table_manager_bcard_t *bc) {
            if (all_servers.count(bc->server_id) == 1) {
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
            wait_any_t interruptor_combined(&dw, interruptor);
            wait_interruptible(&got_ack, &interruptor_combined);

            ++num_acked;
        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    });
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    if (num_acked == 0) {
        throw maybe_failed_table_op_exc_t();
    }

    /* Wait until the table appears in the directory. */
    wait_until_change_visible(
        table_id,
        [&](const timestamped_basic_config_t *value) {
            return value != nullptr &&
                (value->second.epoch == timestamp.epoch ||
                    value->second.epoch.supersedes(timestamp.epoch));
        },
        interruptor);
}

void table_meta_client_t::retry(
        const std::function<void(signal_t *)> &fun,
        signal_t *interruptor) {
    static const int max_tries = 5;
    static const int initial_wait_ms = 300;
    int tries_left = max_tries;
    int wait_ms = initial_wait_ms;
    bool maybe_succeeded = false;
    for (;;) {
        try {
            fun(interruptor);
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
        nap(wait_ms, interruptor);
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

