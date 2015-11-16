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

void table_meta_client_t::get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        table_config_and_shards_t *config_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    table_status_request_t request;
    request.want_config = true;
    std::set<namespace_id_t> failures;
    get_status(
        boost::make_optional(table_id),
        request,
        server_selector_t::BEST_SERVER_ONLY,
        &interruptor,
        [&](const server_id_t &, const namespace_id_t &,
                const table_status_response_t &response) {
            *config_out = *response.config;
        },
        &failures);
    if (!failures.empty()) {
        throw_appropriate_exception(table_id);
    }
}

void table_meta_client_t::list_configs(
        signal_t *interruptor_on_caller,
        std::map<namespace_id_t, table_config_and_shards_t> *configs_out,
        std::map<namespace_id_t, table_basic_config_t> *disconnected_configs_out)
        THROWS_ONLY(interrupted_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    configs_out->clear();
    table_status_request_t request;
    request.want_config = true;
    std::set<namespace_id_t> failures;
    get_status(
        boost::none,
        request,
        server_selector_t::BEST_SERVER_ONLY,
        &interruptor,
        [&](const server_id_t &, const namespace_id_t &table_id,
                const table_status_response_t &response) {
            configs_out->insert(std::make_pair(table_id, *response.config));
        },
        &failures);
    for (const namespace_id_t &table_id : failures) {
        multi_table_manager->get_table_basic_configs()->read_key(table_id,
        [&](const timestamped_basic_config_t *value) {
            if (value != nullptr) {
                disconnected_configs_out->insert(std::make_pair(table_id, value->first));
            }
        });
    }
}

void table_meta_client_t::get_sindex_status(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *sindex_statuses_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    sindex_statuses_out->clear();
    table_config_and_shards_t config;
    get_config(table_id, &interruptor, &config);
    for (const auto &pair : config.config.sindexes) {
        auto it = sindex_statuses_out->insert(
            std::make_pair(pair.first, std::make_pair(pair.second,
                                                      sindex_status_t()))).first;
        it->second.second.outdated =
            (pair.second.func_version != reql_version_t::LATEST);
    }
    table_status_request_t request;
    request.want_sindexes = true;
    std::set<namespace_id_t> failures;
    get_status(
        boost::make_optional(table_id),
        request,
        server_selector_t::EVERY_SERVER,
        &interruptor,
        [&](const server_id_t &, const namespace_id_t &,
                const table_status_response_t &response) {
            for (auto &&pair : *sindex_statuses_out) {
                auto it = response.sindexes.find(pair.first);
                /* Note that we treat an index with the wrong definition like a
                missing index. */
                if (it != response.sindexes.end() &&
                        it->second.first == pair.second.first) {
                    pair.second.second.accum(it->second.second);
                } else {
                    pair.second.second.ready = false;
                }
            }
        },
        &failures);
    if (!failures.empty()) {
        throw_appropriate_exception(table_id);
    }
}

void table_meta_client_t::get_shard_status(
        const namespace_id_t &table_id,
        all_replicas_ready_mode_t all_replicas_ready_mode,
        signal_t *interruptor_on_caller,
        std::map<server_id_t, range_map_t<key_range_t::right_bound_t,
            table_shard_status_t> > *shard_statuses_out,
        bool *all_replicas_ready_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    *all_replicas_ready_out = false;
    table_status_request_t request;
    request.want_shard_status = (shard_statuses_out != nullptr);
    request.want_all_replicas_ready = true;
    request.all_replicas_ready_mode = all_replicas_ready_mode;
    std::set<namespace_id_t> failures;
    get_status(
        boost::make_optional(table_id),
        request,
        /* If we only care about `all_replicas_ready`, there's no need to contact any
        server other than the primary */
        shard_statuses_out != nullptr
            ? server_selector_t::EVERY_SERVER
            : server_selector_t::BEST_SERVER_ONLY,
        &interruptor,
        [&](const server_id_t &server_id, const namespace_id_t &,
                const table_status_response_t &response) {
            if (shard_statuses_out != nullptr) {
                shard_statuses_out->insert(
                    std::make_pair(server_id, response.shard_status));
            }
            *all_replicas_ready_out |= response.all_replicas_ready;
        },
        &failures);
    if (!failures.empty()) {
        throw_appropriate_exception(table_id);
    }
}

void table_meta_client_t::get_raft_leader(
        const namespace_id_t &table_id,
        signal_t *interruptor_on_caller,
        boost::optional<server_id_t> *raft_leader_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    table_manager_directory->read_all(
      [&](const std::pair<peer_id_t, namespace_id_t> &key,
          const table_manager_bcard_t *bcard) {
          if (key.second == table_id && static_cast<bool>(bcard->leader)) {
            *raft_leader_out = boost::make_optional(bcard->server_id);
          }
      });
}

void table_meta_client_t::get_debug_status(
        const namespace_id_t &table_id,
        all_replicas_ready_mode_t all_replicas_ready_mode,
        signal_t *interruptor_on_caller,
        std::map<server_id_t, table_status_response_t> *responses_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());
    table_status_request_t request;
    request.want_config = true;
    request.want_sindexes = true;
    request.want_raft_state = true;
    request.want_contract_acks = true;
    request.want_shard_status = true;
    request.want_all_replicas_ready = true;
    request.all_replicas_ready_mode = all_replicas_ready_mode;
    std::set<namespace_id_t> failures;
    get_status(
        boost::make_optional(table_id),
        request,
        server_selector_t::EVERY_SERVER,
        &interruptor,
        [&](const server_id_t &server_id, const namespace_id_t &,
                const table_status_response_t &response) {
            responses_out->insert(std::make_pair(server_id, response));
        },
        &failures);
    if (!failures.empty()) {
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

    create_or_emergency_repair(
        table_id,
        make_new_table_raft_state(initial_config),
        multi_table_manager_timestamp_t::epoch_t::make(
            multi_table_manager_timestamp_t::epoch_t::min()),
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
                multi_table_manager_timestamp_t::deletion(),
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
        const table_config_and_shards_change_t &table_config_and_shards_change,
        signal_t *interruptor_on_caller)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t, config_change_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    multi_table_manager_timestamp_t timestamp;
    retry([&](signal_t *interruptor2) {
        /* Find the server (if any) which is acting as leader for the table */
        uuid_u best_leader_uuid;
        table_manager_bcard_t::leader_bcard_t::set_config_mailbox_t::address_t
            best_mailbox;
        multi_table_manager_timestamp_t best_timestamp;
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
        promise_t<std::pair<boost::optional<multi_table_manager_timestamp_t>, bool> > promise;
        mailbox_t<void(boost::optional<multi_table_manager_timestamp_t>, bool)>
            ack_mailbox(mailbox_manager,
            [&](signal_t *,
                    const boost::optional<multi_table_manager_timestamp_t> &change_timestamp,
                    bool is_change_successful) {
                promise.pulse(std::make_pair(change_timestamp, is_change_successful));
            });
        send(mailbox_manager,
             best_mailbox,
             table_config_and_shards_change,
             ack_mailbox.get_address());
        wait_any_t done_cond(promise.get_ready_signal(),
            &leader_disconnected, &leader_stopped);
        wait_interruptible(&done_cond, interruptor2);
        if (!promise.get_ready_signal()->is_pulsed()) {
            throw maybe_failed_table_op_exc_t();
        }

        /* Sometimes the server will reply by indicating that something went wrong */
        std::pair<boost::optional<multi_table_manager_timestamp_t>, bool> response =
            promise.wait();
        if (response.second == false) {
            throw config_change_exc_t();
        } else if (!static_cast<bool>(response.first)) {
            throw maybe_failed_table_op_exc_t();
        }
        timestamp = response.first.get();
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
                table_config_and_shards_change.name_and_database_equal(value->first);
        },
        &interruptor);
}

void table_meta_client_t::emergency_repair(
        const namespace_id_t &table_id,
        emergency_repair_mode_t mode,
        bool dry_run,
        signal_t *interruptor_on_caller,
        table_config_and_shards_t *new_config_out,
        bool *rollback_found_out,
        bool *erase_found_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    cross_thread_signal_t interruptor(interruptor_on_caller, home_thread());
    on_thread_t thread_switcher(home_thread());

    table_raft_state_t old_state;
    table_status_request_t request;
    request.want_raft_state = true;
    std::set<namespace_id_t> failures;
    get_status(
        boost::make_optional(table_id),
        request,
        server_selector_t::BEST_SERVER_ONLY,
        &interruptor,
        [&](const server_id_t &, const namespace_id_t &,
                const table_status_response_t &response) {
            old_state = *response.raft_state;
        },
        &failures);
    if (!failures.empty()) {
        throw_appropriate_exception(table_id);
    }

    std::set<server_id_t> dead_servers;
    for (const auto &pair : old_state.member_ids) {
        if (!static_cast<bool>(server_config_client->
                get_server_to_peer_map()->get_key(pair.first))) {
            dead_servers.insert(pair.first);
        }
    }

    table_raft_state_t new_state;
    calculate_emergency_repair(
        old_state,
        dead_servers,
        mode,
        &new_state,
        rollback_found_out,
        erase_found_out);

    *new_config_out = new_state.config;

    if ((*rollback_found_out || *erase_found_out ||
                mode == emergency_repair_mode_t::DEBUG_RECOMMIT) && !dry_run) {
        /* In theory, we don't always have to start a new epoch. Sometimes we run an
        emergency repair where we've lost a quorum of one shard, but still have a quorum
        of the Raft cluster as a whole. In that case we could run a regular Raft
        transaction, which could be made slightly safer. But it's simpler to do
        everything through the same code path. */

        /* Fetch the table's current epoch's timestamp to make sure that the new epoch
        has a higher timestamp, even if the server's clock is wrong. */
        multi_table_manager_timestamp_t::epoch_t old_epoch;
        multi_table_manager->get_table_basic_configs()->read_key(table_id,
            [&](const std::pair<table_basic_config_t,
                    multi_table_manager_timestamp_t> *pair) {
                if (pair == nullptr) {
                    throw no_such_table_exc_t();
                }
                old_epoch = pair->second.epoch;
            });

        create_or_emergency_repair(
            table_id,
            new_state,
            multi_table_manager_timestamp_t::epoch_t::make(old_epoch),
            &interruptor);
    }
}

void table_meta_client_t::create_or_emergency_repair(
        const namespace_id_t &table_id,
        const table_raft_state_t &raft_state,
        const multi_table_manager_timestamp_t::epoch_t &epoch,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t) {
    assert_thread();

    /* Prepare the message that we'll be sending to each server */
    multi_table_manager_timestamp_t timestamp;
    timestamp.epoch = epoch;
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

/* `best_server_rank_t` is used for comparing servers when we're in `BEST_SERVER_ONLY`
mode. */
class best_server_rank_t {
public:
    /* Note that a default-constructed `best_server_rank_t` is superseded by any other
    `best_server_rank_t`. */
    best_server_rank_t() :
        is_leader(false), timestamp(multi_table_manager_timestamp_t::min()) { }
    best_server_rank_t(bool il, const multi_table_manager_timestamp_t ts) :
        is_leader(il), timestamp(ts) { }

    bool supersedes(const best_server_rank_t &other) const {
        /* Ordered comparison first on `timestamp.epoch`, then `is_leader`, then
        `timestamp.log_index`. This is slightly convoluted because `timestamp.epoch` is
        compared with `supersedes()` rather than `operator<()`. */
        return timestamp.epoch.supersedes(other.timestamp.epoch) ||
            (timestamp.epoch == other.timestamp.epoch &&
                std::tie(is_leader, timestamp.log_index) >
                    std::tie(other.is_leader, other.timestamp.log_index));
    }

    bool is_leader;
    multi_table_manager_timestamp_t timestamp;
};

void table_meta_client_t::get_status(
        const boost::optional<namespace_id_t> &table,
        const table_status_request_t &request,
        server_selector_t servers,
        signal_t *interruptor,
        const std::function<void(
            const server_id_t &server,
            const namespace_id_t &table,
            const table_status_response_t &response
            )> &callback,
        std::set<namespace_id_t> *failures_out)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    interruptor->assert_thread();

    /* Assemble a set of all the tables we need information for. As we get information
    for each table, we'll remove it from the set; we'll use this to track which tables
    we need to retry for. */
    std::set<namespace_id_t> tables_todo;
    if (static_cast<bool>(table)) {
        tables_todo.insert(*table);
    } else {
        table_basic_configs.get_watchable()->read_all(
            [&](const namespace_id_t &table_id, const timestamped_basic_config_t *) {
                tables_todo.insert(table_id);
            });
    }

    /* If we're in `BEST_SERVER_ONLY` mode, there's a risk that the table will move off
    the server we selected before we get a chance to run the query, which could cause
    spurious failures. So in that mode we try again if the first try fails. */
    int tries_left = servers == server_selector_t::BEST_SERVER_ONLY ? 2 : 1;

    while (!tables_todo.empty() && tries_left > 0) {
        --tries_left;

        /* Assemble a list of which servers we need to send messages to, and which tables
        we want from each server. */
        std::map<peer_id_t, std::set<namespace_id_t> > targets;
        switch (servers) {
            case server_selector_t::EVERY_SERVER: {
                table_manager_directory->read_all(
                [&](const std::pair<peer_id_t, namespace_id_t> &key,
                        const table_manager_bcard_t *) {
                    if (tables_todo.count(key.second) == 1) {
                        targets[key.first].insert(key.second);
                    }
                });
                break;
            }
            case server_selector_t::BEST_SERVER_ONLY: {
                std::map<namespace_id_t, peer_id_t> best_servers;
                std::map<namespace_id_t, best_server_rank_t> best_ranks;
                table_manager_directory->read_all(
                [&](const std::pair<peer_id_t, namespace_id_t> &key,
                        const table_manager_bcard_t *bcard) {
                    if (tables_todo.count(key.second) == 1) {
                        best_server_rank_t rank(
                            static_cast<bool>(bcard->leader), bcard->timestamp);
                        /* This relies on the fact that a default-constructed
                        `best_server_rank_t` is always superseded */
                        if (rank.supersedes(best_ranks[key.second])) {
                            best_ranks[key.second] = rank;
                            best_servers[key.second] = key.first;
                        }
                    }
                });
                for (const auto &pair : best_servers) {
                    targets[pair.second].insert(pair.first);
                }
                break;
            }
        }

        /* Dispatch all the messages in parallel. Probably this should really be a
        `throttled_pmap` instead. */
        pmap(targets.begin(), targets.end(),
        [&](const std::pair<peer_id_t, std::set<namespace_id_t> > &target) {
            boost::optional<multi_table_manager_bcard_t> bcard =
                multi_table_manager_directory->get_key(target.first);
            if (!static_cast<bool>(bcard)) {
                return;
            }
            disconnect_watcher_t dw(mailbox_manager,
                bcard->get_status_mailbox.get_peer());
            cond_t got_ack;
            mailbox_t<void(std::map<namespace_id_t, table_status_response_t>)>
            ack_mailbox(mailbox_manager,
                [&](signal_t *, const std::map<
                        namespace_id_t, table_status_response_t> &resp) {
                    for (const auto &pair : resp) {
                        callback(bcard->server_id, pair.first, pair.second);
                        tables_todo.erase(pair.first);
                    }
                    got_ack.pulse();
                });
            send(mailbox_manager, bcard->get_status_mailbox,
                target.second, request, ack_mailbox.get_address());
            wait_any_t waiter(&dw, interruptor, &got_ack);
            waiter.wait_lazily_unordered();
        });

        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }

    *failures_out = std::move(tables_todo);
}

void table_meta_client_t::retry(
        const std::function<void(signal_t *)> &fun,
        signal_t *interruptor) {
    /* 8 tries at 300ms initially with 1.5x exponential backoff means the last try will
    be about 15 seconds after the first try. Since that's about ten times the Raft
    election timeout, we can be reasonably certain that if it fails that many times there
    is a real problem and not just a transient failure. */
    static const int max_tries = 8;
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

