// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/multi_table_manager.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_manager/table_manager.hpp"
#include "logger.hpp"

multi_table_manager_t::multi_table_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        server_config_client_t *_server_config_client,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *_connections_map,
        table_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        perfmon_collection_repo_t *_perfmon_collection_repo) :
    is_proxy_server(false),
    server_id(_server_id),
    mailbox_manager(_mailbox_manager),
    server_config_client(_server_config_client),
    multi_table_manager_directory(_multi_table_manager_directory),
    table_manager_directory(_table_manager_directory),
    connections_map(_connections_map),
    persistence_interface(_persistence_interface),
    base_path(_base_path),
    io_backender(_io_backender),
    perfmon_collection_repo(_perfmon_collection_repo) {

    /* Resurrect any tables that were sitting on disk from when we last shut down */
    cond_t non_interruptor;
    persistence_interface->read_all_metadata(
        [&](const namespace_id_t &table_id,
                const table_active_persistent_state_t &state,
                raft_storage_interface_t<table_raft_state_t> *raft_storage,
                metadata_file_t::read_txn_t *metadata_read_txn) {
            guarantee(tables.count(table_id) == 0);
            table_t *table;
            tables[table_id].init(table = new table_t);
            rwlock_acq_t table_lock_acq(&table->access_rwlock, access_t::write);
            perfmon_collection_repo_t::collections_t *perfmon_collections =
                perfmon_collection_repo->get_perfmon_collections_for_namespace(table_id);
            table->status = table_t::status_t::ACTIVE;
            persistence_interface->load_multistore(
                table_id, metadata_read_txn, &table->multistore_ptr, &non_interruptor,
                &perfmon_collections->serializers_collection);
            table->active = make_scoped<active_table_t>(
                this, table, table_id, state.epoch, state.raft_member_id, raft_storage,
                raft_start_election_immediately_t::NO, table->multistore_ptr.get(),
                &perfmon_collections->namespace_collection);
        },
        [&](const namespace_id_t &table_id,
                const table_inactive_persistent_state_t &state,
                metadata_file_t::read_txn_t *) {
            guarantee(tables.count(table_id) == 0);
            table_t *table;
            tables[table_id].init(table = new table_t);
            rwlock_acq_t table_lock_acq(&table->access_rwlock, access_t::write);
            table->status = table_t::status_t::INACTIVE;
            table->basic_configs_entry.create(&table_basic_configs, table_id,
                std::make_pair(state.second_hand_config, state.timestamp));
        },
        &non_interruptor);

    help_construct();
}

/* This constructor is used for proxy servers. */
multi_table_manager_t::multi_table_manager_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory) :
    is_proxy_server(true),
    server_id(nil_uuid()),
    mailbox_manager(_mailbox_manager),
    server_config_client(nullptr),
    multi_table_manager_directory(_multi_table_manager_directory),
    table_manager_directory(_table_manager_directory),
    connections_map(nullptr),
    persistence_interface(nullptr),
    base_path(boost::none),
    io_backender(nullptr),
    perfmon_collection_repo(nullptr)
{
    help_construct();
}

multi_table_manager_t::~multi_table_manager_t() {
    /* First, shut out further mailbox events or watchable callbacks. This ensures that
    tables are not created or destroyed, nor are their states changed (active vs.
    inactive vs. deleted). */
    get_status_mailbox.reset();
    action_mailbox.reset();
    table_manager_directory_subs.reset();
    multi_table_manager_directory_subs.reset();

    /* Next, destroy all of the `active_table_t`s. This is important because otherwise
    `active_table_t` can call `schedule_sync()`. */
    for (auto &&pair : tables) {
        rwlock_acq_t lock_acq(&pair.second->access_rwlock, access_t::write);
        pair.second->status = table_t::status_t::SHUTTING_DOWN;
        pair.second->active.reset();
        pair.second->multistore_ptr.reset();
        pair.second->basic_configs_entry.reset();
    }

    /* Drain any running coroutines */
    drainer.drain();
}

multi_table_manager_t::active_table_t::active_table_t(
        multi_table_manager_t *_parent,
        table_t *_table,
        const namespace_id_t &_table_id,
        const multi_table_manager_timestamp_t::epoch_t &epoch,
        const raft_member_id_t &member_id,
        raft_storage_interface_t<table_raft_state_t> *raft_storage,
        const raft_start_election_immediately_t start_election_immediately,
        multistore_ptr_t *multistore_ptr,
        perfmon_collection_t *perfmon_collection_namespace) :
    parent(_parent),
    table(_table),
    table_id(_table_id),
    manager(parent->server_id, parent->mailbox_manager, parent->server_config_client,
        parent->table_manager_directory, &parent->backfill_throttler,
        parent->connections_map, *parent->base_path, parent->io_backender, table_id,
        epoch, member_id, raft_storage, start_election_immediately, multistore_ptr,
        perfmon_collection_namespace),
    table_manager_bcard_copier(
        &parent->table_manager_bcards, table_id, manager.get_table_manager_bcard()),
    table_query_bcard_source(
        &parent->table_query_bcard_combiner, table_id, manager.get_table_query_bcards()),
    raft_committed_subs(std::bind(&active_table_t::on_raft_commit, this))
{
    guarantee(!parent->is_proxy_server, "proxy server shouldn't be hosting data");
    watchable_t<raft_member_t<table_raft_state_t>::state_and_config_t>::freeze_t
        freeze(manager.get_raft()->get_committed_state());
    raft_committed_subs.reset(manager.get_raft()->get_committed_state(), &freeze);

    update_basic_configs_entry();
}

void multi_table_manager_t::active_table_t::on_raft_commit() {
    /* If the Raft transaction changed the table's name, database, or primary key, push
    the changes out to the `table_meta_client_t`. */
    bool basic_config_changed = update_basic_configs_entry();

    /* Maybe re-sync to other servers in the cluster in reaction to this change */
    parent->multi_table_manager_directory->read_all(
    [&](const peer_id_t &peer, const multi_table_manager_bcard_t *multi_bcard) {
        if (peer != parent->mailbox_manager->get_me()) {
            /* We don't re-sync to every server all the time, for performance
            reasons. Instead, we try to determine whether it's actually necessary to
            sync or not, and if we're sure it's unnecessary we don't sync. */
            if (basic_config_changed ||
                    should_sync_assuming_no_name_change(peer, multi_bcard->server_id)) {
                parent->schedule_sync(manager.table_id, table, peer);
            }
        }
    });
}

bool multi_table_manager_t::active_table_t::update_basic_configs_entry() {
    bool changed;
    manager.get_raft()->get_committed_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
            std::pair<table_basic_config_t, multi_table_manager_timestamp_t> p;
            p.first = sc->state.config.config.basic;
            p.second.epoch = manager.epoch;
            p.second.log_index = sc->log_index;
            if (table->basic_configs_entry.has()) {
                table->basic_configs_entry->change(
                    [&](std::pair<table_basic_config_t,
                                  multi_table_manager_timestamp_t> *value
                            ) -> bool {
                        changed = !(value->first == p.first);
                        *value = p;
                        return true;
                    });
            } else {
                table->basic_configs_entry.create(
                    &parent->table_basic_configs, table_id, p);
                changed = true;
            }
        });
    return changed;
}

bool multi_table_manager_t::active_table_t::should_sync_assuming_no_name_change(
        const peer_id_t &other_peer_id, const server_id_t &other_server_id) {
    guarantee(other_peer_id != parent->mailbox_manager->get_me());
    bool should_sync = false;
    manager.get_raft()->get_committed_state()->apply_read(
    [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
        parent->table_manager_directory->read_key(
        std::make_pair(other_peer_id, table_id),
        [&](const table_manager_bcard_t *table_bcard) {
            if (table_bcard == nullptr) {
                /* Sync if the other server is supposed to be involved in
                this table. */
                should_sync = (sc->state.member_ids.count(other_server_id) == 1);
            } else {
                /* Sync if:
                - The other server shouldn't be involved in this table
                - The other server has the wrong raft member ID
                - The other server has an epoch that is too old */
                should_sync =
                    sc->state.member_ids.count(other_server_id) == 0 ||
                    sc->state.member_ids.at(other_server_id) !=
                        table_bcard->raft_member_id ||
                    manager.epoch.supersedes(table_bcard->timestamp.epoch);
            }
        });
    });
    return should_sync;
}

void multi_table_manager_t::help_construct() {
    /* Whenever a server connects, we need to sync all of our tables to it.
    Additionally: sync all of our existing tables with all servers that are
    already connected. */
    multi_table_manager_directory_subs.init(
        new watchable_map_t<peer_id_t, multi_table_manager_bcard_t>::all_subs_t(
            multi_table_manager_directory,
            [this](const peer_id_t &peer, const multi_table_manager_bcard_t *bcard) {
                if (peer != mailbox_manager->get_me() && bcard != nullptr) {
                    mutex_assertion_t::acq_t mutex_acq(&mutex);
                    for (const auto &pair : tables) {
                        schedule_sync(pair.first, pair.second.get(), peer);
                    }
                }
            }, initial_call_t::YES));

    /* Whenever a server changes its entry for a table in the directory, we need to
    re-sync that table to that server. */
    table_manager_directory_subs.init(
        new watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
                ::all_subs_t(
            table_manager_directory,
            [this](const std::pair<peer_id_t, namespace_id_t> &key,
                    const table_manager_bcard_t *) {
                if (key.first != mailbox_manager->get_me()) {
                    mutex_assertion_t::acq_t mutex_acq(&mutex);
                    auto it = tables.find(key.second);
                    if (it != tables.end()) {
                        schedule_sync(key.second, it->second.get(), key.first);
                    }
                }
            }, initial_call_t::NO));

    action_mailbox.init(new multi_table_manager_bcard_t::action_mailbox_t(
        mailbox_manager,
        std::bind(&multi_table_manager_t::on_action, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8, ph::_9)));

    get_status_mailbox.init(new multi_table_manager_bcard_t::get_status_mailbox_t(
        mailbox_manager,
        std::bind(&multi_table_manager_t::on_get_status, this,
            ph::_1, ph::_2, ph::_3, ph::_4)));
}

void multi_table_manager_t::on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const multi_table_manager_timestamp_t &timestamp,
        multi_table_manager_bcard_t::status_t action_status,
        const boost::optional<table_basic_config_t> &basic_config,
        const boost::optional<raft_member_id_t> &raft_member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_raft_state,
        const boost::optional<raft_start_election_immediately_t>
            &start_election_immediately,
        const mailbox_t<void()>::address_t &ack_addr) {
    typedef multi_table_manager_bcard_t::status_t action_status_t;

    /* Validate the incoming message */
    guarantee(static_cast<bool>(basic_config) ==
        (action_status == action_status_t::INACTIVE ||
            action_status == action_status_t::MAYBE_ACTIVE));
    guarantee(static_cast<bool>(raft_member_id) ==
        (action_status == action_status_t::ACTIVE));
    guarantee(static_cast<bool>(initial_raft_state) ==
        (action_status == action_status_t::ACTIVE));
    guarantee(timestamp.is_deletion() ==
        (action_status == action_status_t::DELETED));
    guarantee(action_status != action_status_t::ACTIVE ||
        static_cast<bool>(start_election_immediately));

    /* Find or create the table record for this table */
    mutex_assertion_t::acq_t global_mutex_acq(&mutex);
    bool is_new = (tables.count(table_id) == 0);
    table_t *table;
    if (is_new) {
        tables[table_id].init(table = new table_t);
    } else {
        table = tables.at(table_id).get();
    }
    rwlock_in_line_t table_lock_in_line(&table->access_rwlock, access_t::write);
    global_mutex_acq.reset();
    wait_interruptible(table_lock_in_line.write_signal(), interruptor);

    /* Validate existing record */
    if (!is_new) {
        guarantee(table->status != table_t::status_t::ACTIVE || !is_proxy_server);
        guarantee(table->multistore_ptr.has() ==
            (table->status == table_t::status_t::ACTIVE));
        guarantee(table->active.has() ==
            (table->status == table_t::status_t::ACTIVE));
        guarantee(table->basic_configs_entry.has() ==
            (table->status != table_t::status_t::DELETED));
    }

    /* Reject outdated or redundant messages */
    if (!is_new) {
        multi_table_manager_timestamp_t current_timestamp;
        switch (table->status) {
            case table_t::status_t::ACTIVE:
                current_timestamp.epoch = table->active->manager.epoch;
                current_timestamp.log_index =
                    table->active->get_raft()->get_committed_state()->get().log_index;
                break;
            case table_t::status_t::INACTIVE:
                current_timestamp = table->basic_configs_entry->get_value().second;
                break;
            case table_t::status_t::DELETED:
                current_timestamp = multi_table_manager_timestamp_t::deletion();
                break;
            case table_t::status_t::SHUTTING_DOWN:   /* fall through */
            default: unreachable();
        }
        /* Rejecting old actions is absolutely critical for correctness.
        ACTIVE actions contain the Raft member ID. If we accepted an INACTIVE and then
        an ACTIVE action in the wrong order, we might swipe away our Raft state due to
        the INACTIVE action, and then become active again *with an old Raft ID* that
        we had been active with before. This will violate Raft invariants, and can lead
        to split-brain configurations and data loss. */
        if (!timestamp.supersedes(current_timestamp)) {
            /* If we are inactive and are told to become active, we print a special
            message. The `base_table_config_t` in our inactive state might be
            a left-over from an emergency repair that none of the currently active
            servers has seen. In that case we would have no chance to become active
            again for this table until another emergency repair happened
            (which might be impossible, if the table is otherwise still available). */
            bool outdated_activation =
                table->status == table_t::status_t::INACTIVE &&
                action_status == action_status_t::ACTIVE &&
                timestamp.epoch != current_timestamp.epoch;
            if (outdated_activation) {
                logWRN("Table %s (%s): Not adding a replica on this server because the "
                       "active configuration conflicts with a more recent inactive "
                       "configuration. "
                       "If you see replicas get stuck in the `transitioning` state "
                       "after reconfiguring this table, you can try recovering by "
                       "running `.reconfigure({emergencyRepair: '_debug_recommit'})` on "
                       "it. Please make sure that the cluster is idle when running this "
                       "operation. RethinkDB does not guarantee consistency during "
                       "the emergency repair.",
                       uuid_to_str(table_id).c_str(),
                       table->basic_configs_entry->get_value().first.name.c_str());
            }
            if (!ack_addr.is_nil()) {
                send(mailbox_manager, ack_addr);
            }
            return;
        }
    }

    guarantee(is_new || table->status != table_t::status_t::DELETED,
        "It shouldn't be possible to undelete a table.");

    bool should_resync = false;

    /* Bring record up to date */
    if (action_status == action_status_t::ACTIVE) {
        guarantee(!is_proxy_server, "proxy server shouldn't be hosting data");

        perfmon_collection_repo_t::collections_t *perfmon_collections =
            perfmon_collection_repo->get_perfmon_collections_for_namespace(table_id);

        if (is_new || table->status != table_t::status_t::ACTIVE) {
            /* The table is being created, or we are joining it */
            table->status = table_t::status_t::ACTIVE;

            /* Write a record to disk for the new table */
            raft_storage_interface_t<table_raft_state_t> *raft_storage;
            persistence_interface->write_metadata_active(
                table_id,
                table_active_persistent_state_t { timestamp.epoch, *raft_member_id },
                *initial_raft_state,
                &raft_storage);

            /* Open files for the new table. We do this after writing the record, because
            this way if we crash we won't leak the file. */
            persistence_interface->create_multistore(
                table_id,
                &table->multistore_ptr,
                interruptor,
                &perfmon_collections->serializers_collection);

            /* Create the `active_table_t`, which contains the `raft_member_t` and does
            all of the important work of actually handing queries */
            table->active = make_scoped<active_table_t>(
                this, table, table_id, timestamp.epoch, *raft_member_id,
                raft_storage, *start_election_immediately, table->multistore_ptr.get(),
                &perfmon_collections->namespace_collection);

            logINF("Table %s: Added replica on this server.",
                uuid_to_str(table_id).c_str());

        } else if (table->active->manager.epoch != timestamp.epoch ||
                table->active->manager.raft_member_id != *raft_member_id) {
            /* The table was in an active state before, and it should still be in an
            active state; but the epoch or member ID changed. So we have to destroy and
            then re-create `table->active`. */

            table->active.reset();

            /* Update the record on disk */
            raft_storage_interface_t<table_raft_state_t> *raft_storage;
            persistence_interface->write_metadata_active(
                table_id,
                table_active_persistent_state_t { timestamp.epoch, *raft_member_id },
                *initial_raft_state,
                &raft_storage);

            table->active = make_scoped<active_table_t>(
                this, table, table_id, timestamp.epoch, *raft_member_id,
                raft_storage, *start_election_immediately, table->multistore_ptr.get(),
                &perfmon_collections->namespace_collection);

            logINF("Table %s: Reset replica on this server.",
                uuid_to_str(table_id).c_str());
        }

        should_resync = true;

    } else if (action_status == action_status_t::INACTIVE ||
            (action_status == action_status_t::MAYBE_ACTIVE &&
                (is_new || table->status != table_t::status_t::ACTIVE))) {
        if (!is_new && table->status == table_t::status_t::ACTIVE) {
            /* We are being demoted; we used to be hosting this table, but no longer are.
            But we still keep a record of the table's name, etc. just like every other
            server that's not hosting the table. */

            table->active.reset();

            /* Destroy files for the table before we update the metadata record, so that
            if we crash we won't leak the file. */
            persistence_interface->destroy_multistore(
                table_id, &table->multistore_ptr);

            logINF("Table %s: Removed replica on this server.",
                uuid_to_str(table_id).c_str());
        }

        /* We just found out about this table, or we are updating an existing
        second-hand information record with newer information */
        table->status = table_t::status_t::INACTIVE;

        if (table->basic_configs_entry.has()) {
            table->basic_configs_entry->set(std::make_pair(*basic_config, timestamp));
        } else {
            table->basic_configs_entry.create(&table_basic_configs, table_id,
                std::make_pair(*basic_config, timestamp));
        }

        /* Update the record on disk */
        if (!is_proxy_server) {
            persistence_interface->write_metadata_inactive(
                table_id,
                table_inactive_persistent_state_t { *basic_config, timestamp });
        }

    } else if (action_status == action_status_t::DELETED) {
        if (!is_new) {
            /* Clean up the table's current records */
            if (table->status == table_t::status_t::ACTIVE) {
                table->active.reset();

                /* Destroy files for the table before we update the metadata record, so
                that if we crash we won't leak the file. */
                persistence_interface->destroy_multistore(
                    table_id, &table->multistore_ptr);

                logINF("Table %s: Deleted the table.", uuid_to_str(table_id).c_str());
            }
            table->basic_configs_entry.reset();
        }

        table->status = table_t::status_t::DELETED;

        /* Remove the record on disk */
        if (!is_proxy_server) {
            persistence_interface->delete_metadata(table_id);
        }

        should_resync = true;
    }

    if (!ack_addr.is_nil()) {
        send(mailbox_manager, ack_addr);
    }

    /* If we just became active for the table or we just found out that the table was
    deleted, resync to every other server. This serves two purposes:
    - When a table is first created, the creating server sends the creation action to the
        active servers, which then send the table's name and other information to all the
        other servers via this resync.
    - Resyncing makes us more resilient to non-transitive connectivity.
    However, it might become a performance problem at some point. */
    if (should_resync) {
        multi_table_manager_directory->read_all(
        [&](const peer_id_t &peer, const multi_table_manager_bcard_t *) {
            if (peer != mailbox_manager->get_me()) {
                schedule_sync(table_id, table, peer);
            }
        });
    }
}

void multi_table_manager_t::on_get_status(
        signal_t *interruptor,
        const std::set<namespace_id_t> &tables_of_interest,
        const table_status_request_t &request,
        const mailbox_t<void(
            std::map<namespace_id_t, table_status_response_t>
            )>::address_t &reply_addr) {
    std::map<namespace_id_t, table_status_response_t> responses;
    for (const namespace_id_t &table_id : tables_of_interest) {
        /* Fetch information for a specific table. */
        mutex_assertion_t::acq_t global_mutex_acq(&mutex);
        auto it = tables.find(table_id);
        if (it != tables.end()) {
            rwlock_in_line_t table_lock_in_line(&it->second->access_rwlock, access_t::read);
            global_mutex_acq.reset();
            wait_interruptible(table_lock_in_line.read_signal(), interruptor);
            if (it->second->status == table_t::status_t::ACTIVE) {
                it->second->active->manager.get_status(
                    request, interruptor, &responses[table_id]);
            }
        }
    }
    send(mailbox_manager, reply_addr, responses);
}

void multi_table_manager_t::do_sync(
        const namespace_id_t &table_id,
        const table_t &table,
        const server_id_t &other_server_id,
        const boost::optional<table_manager_bcard_t> &table_bcard,
        const multi_table_manager_bcard_t &table_manager_bcard) {
    typedef multi_table_manager_bcard_t::status_t action_status_t;

    if (table.status == table_t::status_t::ACTIVE) {
        multi_table_manager_timestamp_t timestamp;
        timestamp.epoch = table.active->manager.epoch;
        action_status_t action_status;
        boost::optional<table_basic_config_t> basic_config;
        boost::optional<raft_member_id_t> raft_member_id;
        boost::optional<raft_persistent_state_t<table_raft_state_t> >
            initial_raft_state;
        {
            cond_t non_interruptor;
            raft_member_t<table_raft_state_t>::change_lock_t raft_change_lock(
                table.active->get_raft(), &non_interruptor);
            table.active->get_raft()->get_committed_state()->apply_read(
                [&](const raft_member_t<table_raft_state_t>::state_and_config_t *st) {
                    timestamp.log_index = st->log_index;
                    auto it = st->state.member_ids.find(other_server_id);
                    if (it != st->state.member_ids.end()) {
                        action_status = action_status_t::ACTIVE;
                        raft_member_id = boost::make_optional(it->second);
                        initial_raft_state = boost::make_optional(
                            table.active->get_raft()->get_state_for_init(
                                raft_change_lock));
                    } else {
                        action_status = action_status_t::INACTIVE;
                        basic_config =
                            boost::make_optional(st->state.config.config.basic);
                    }
                });
        }

        if (static_cast<bool>(table_bcard)) {
            /* If the peer already has an entry in the directory, we can use that to
            avoid sending unnecessary updates to save network traffic. */
            if (action_status == action_status_t::ACTIVE
                    && !timestamp.epoch.supersedes(table_bcard->timestamp.epoch)
                    && *raft_member_id == table_bcard->raft_member_id) {
                return;
            }
        }

        send(mailbox_manager, table_manager_bcard.action_mailbox,
            table_id,
            timestamp,
            action_status,
            basic_config,
            raft_member_id,
            initial_raft_state,
            boost::optional<raft_start_election_immediately_t>(raft_start_election_immediately_t::NO),
            mailbox_t<void()>::address_t());

    } else if (table.status == table_t::status_t::INACTIVE) {
        if (static_cast<bool>(table_bcard)) {
            /* No point in sending a `MAYBE_ACTIVE` message to a server that's actually
            hosting the table already (it would be a noop) */
            return;
        }

        send(mailbox_manager, table_manager_bcard.action_mailbox,
            table_id,
            table.basic_configs_entry->get_value().second,
            action_status_t::MAYBE_ACTIVE,
            boost::optional<table_basic_config_t>(
                table.basic_configs_entry->get_value().first),
            boost::optional<raft_member_id_t>(),
            boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
            boost::optional<raft_start_election_immediately_t>(),
            mailbox_t<void()>::address_t());

    } else if (table.status == table_t::status_t::DELETED) {
        send(mailbox_manager, table_manager_bcard.action_mailbox,
            table_id,
            multi_table_manager_timestamp_t::deletion(),
            action_status_t::DELETED,
            boost::optional<table_basic_config_t>(),
            boost::optional<raft_member_id_t>(),
            boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
            boost::optional<raft_start_election_immediately_t>(),
            mailbox_t<void()>::address_t());

    } else {
        unreachable();
    }
}

void multi_table_manager_t::schedule_sync(
        const namespace_id_t &table_id,
        table_t *table,
        const peer_id_t &peer_id) {
    ASSERT_FINITE_CORO_WAITING;
    table->to_sync_set.insert(peer_id);
    if (table->sync_coro_running) {
        /* The sync coro will pick up our change when it finishes the current batch */
        return;
    }
    guarantee(table->to_sync_set.size() == 1, "If there were other changes queued up, "
        "the sync coro should have already been running");
    table->sync_coro_running = true;
    auto_drainer_t::lock_t keepalive(&drainer);
    /* Spawn the sync coroutine. Note that we have at most one instance of this coroutine
    per table running at any given time. The naive approach would be to spawn one
    instance every time that `schedule_sync()` is called. The problem is that
    `schedule_sync()` may be called redundantly many times; this approach means that a
    lot of the redundant calls will be collapsed. */
    coro_t::spawn_sometime(
    [this, keepalive /* important to capture */, table_id, table]() {
        try {
            mutex_assertion_t::acq_t global_mutex_acq(&mutex);
            while (!table->to_sync_set.empty()) {
                std::set<peer_id_t> to_sync_set;
                std::swap(table->to_sync_set, to_sync_set);
                rwlock_in_line_t table_lock_in_line(&table->access_rwlock, access_t::write);
                global_mutex_acq.reset();
                wait_interruptible(table_lock_in_line.write_signal(),
                    keepalive.get_drain_signal());
                if (table->status == table_t::status_t::SHUTTING_DOWN) {
                    return;
                }
                for (const peer_id_t &peer : to_sync_set) {
                    guarantee(peer != mailbox_manager->get_me());
                    /* Removing `peer` from `table->to_sync_set` isn't strictly
                    necessary, but it sometimes reduces redundant traffic */
                    table->to_sync_set.erase(peer);
                    boost::optional<multi_table_manager_bcard_t>
                        multi_table_manager_bcard =
                            multi_table_manager_directory->get_key(peer);
                    if (!static_cast<bool>(multi_table_manager_bcard)) {
                        /* Peer is not connected. */
                        continue;
                    }
                    do_sync(
                        table_id,
                        *table,
                        multi_table_manager_bcard->server_id,
                        table_manager_directory->get_key(std::make_pair(peer, table_id)),
                        *multi_table_manager_bcard);
                }
                global_mutex_acq.reset(&mutex);
            }
            table->sync_coro_running = false;
        } catch (const interrupted_exc_t &) {
            /* We're shutting down. Don't continue syncing. `sync_coro_running` will be
            left in the `true` state, but that doesn't matter. */
        }
    });
}

