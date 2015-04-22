// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/multi_table_manager.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_manager/table_manager.hpp"
#include "logger.hpp"

multi_table_manager_t::multi_table_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        table_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender) :
    server_id(_server_id),
    mailbox_manager(_mailbox_manager),
    multi_table_manager_directory(_multi_table_manager_directory),
    table_manager_directory(_table_manager_directory),
    persistence_interface(_persistence_interface),
    base_path(_base_path),
    io_backender(_io_backender),
    /* Whenever a server connects, we need to sync all of our tables to it. */
    multi_table_manager_directory_subs(
        multi_table_manager_directory,
        [this](const peer_id_t &peer, const multi_table_manager_bcard_t *bcard) {
            if (peer != mailbox_manager->get_me() && bcard != nullptr) {
                mutex_assertion_t::acq_t mutex_acq(&mutex);
                for (const auto &pair : tables) {
                    schedule_sync(pair.first, pair.second.get(), peer);
                }
            }
        }, false),
    /* Whenever a server changes its entry for a table in the directory, we need to
    re-sync that table to that server. */
    table_manager_directory_subs(
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
        }, false),
    action_mailbox(mailbox_manager,
        std::bind(&multi_table_manager_t::on_action, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7)),
    get_config_mailbox(mailbox_manager,
        std::bind(&multi_table_manager_t::on_get_config, this,
            ph::_1, ph::_2, ph::_3))
{
    guarantee(!server_id.is_unset());

    /* Resurrect any tables that were sitting on disk from when we last shut down */
    cond_t non_interruptor;
    persistence_interface->read_all_tables(
        [&](const namespace_id_t &table_id, const table_persistent_state_t &state) {
            mutex_assertion_t::acq_t global_mutex_acq(&mutex);
            guarantee(tables.count(table_id) == 0);
            table_t *table;
            tables[table_id].init(table = new table_t);
            new_mutex_acq_t table_mutex_acq(&table->mutex);
            global_mutex_acq.reset();

            if (table_persistent_state_t::active_t *active =
                    boost::get<table_persistent_state_t::active_t>(&state.value)) {
                table->status = table_t::status_t::ACTIVE;
                persistence_interface->load_multistore(
                    table_id, &table->multistore_ptr, &non_interruptor);
                table->active = make_scoped<active_table_t>(
                    this, table_id,
                    active->epoch, active->raft_member_id, active->raft_state,
                    table->multistore_ptr.get());
            } else if (table_persistent_state_t::inactive_t *inactive =
                    boost::get<table_persistent_state_t::inactive_t>(&state.value)) {
                table->status = table_t::status_t::INACTIVE;
                table->second_hand_config =
                    boost::make_optional(inactive->second_hand_config);
                table->second_hand_timestamp = boost::make_optional(inactive->timestamp);
            }

            /* This probably won't do anything, since we probably won't be connected to
            any other servers when `multi_table_manager_t` is created; but just in case,
            sync to any other servers that are connected. */
            multi_table_manager_directory->read_all(
                [&](const peer_id_t &peer, const multi_table_manager_bcard_t *) {
                    if (peer != mailbox_manager->get_me()) {
                        schedule_sync(table_id, table, peer);
                    }
                });
        },
        &non_interruptor);
}

multi_table_manager_t::active_table_t::active_table_t(
        multi_table_manager_t *_parent,
        const namespace_id_t &table_id,
        const multi_table_manager_bcard_t::timestamp_t::epoch_t &epoch,
        const raft_member_id_t &member_id,
        const raft_persistent_state_t<table_raft_state_t> &initial_state,
        multistore_ptr_t *multistore_ptr) :
    parent(_parent),
    manager(parent->server_id, parent->mailbox_manager, parent->table_manager_directory,
        &parent->backfill_throttler, parent->persistence_interface, parent->base_path,
        parent->io_backender, table_id, epoch, member_id, initial_state, multistore_ptr),
    table_manager_bcard_copier(
        &parent->table_manager_bcards, table_id, manager.get_table_manager_bcard()),
    table_query_bcard_source(
        &parent->table_query_bcard_combiner, table_id, manager.get_table_query_bcards()),
    raft_committed_subs(std::bind(&active_table_t::on_raft_commit, this))
{
    watchable_t<raft_member_t<table_raft_state_t>::state_and_config_t>::freeze_t
        freeze(manager.get_raft()->get_committed_state());
    raft_committed_subs.reset(manager.get_raft()->get_committed_state(), &freeze);
}

void multi_table_manager_t::active_table_t::on_raft_commit() {
    /* Every time the Raft cluster commits a transaction, we re-sync to every other
    server in the cluster. This is because the transaction might consist of adding or
    removing a server, in which case we need to notify that server.

    This is likely to cause performance issues. We could reduce the number of syncs by
    not resyncing to a given peer unless the peer is mentioned in the Raft state; the
    peer has a `table_manager_bcard_t` for this table; or the table's name or database
    has changed. */
    mutex_assertion_t::acq_t mutex_acq(&parent->mutex);
    auto it = parent->tables.find(manager.table_id);
    guarantee(it != parent->tables.end());
    parent->multi_table_manager_directory->read_all(
    [&](const peer_id_t &peer, const multi_table_manager_bcard_t *) {
        if (peer != parent->mailbox_manager->get_me()) {
            parent->schedule_sync(manager.table_id, it->second.get(), peer);
        }
    });
}

void multi_table_manager_t::on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const multi_table_manager_bcard_t::timestamp_t &timestamp,
        multi_table_manager_bcard_t::status_t action_status,
        const boost::optional<table_basic_config_t> &basic_config,
        const boost::optional<raft_member_id_t> &raft_member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_raft_state,
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

    /* Find or create the table record for this table */
    mutex_assertion_t::acq_t global_mutex_acq(&mutex);
    bool is_new = (tables.count(table_id) == 0);
    table_t *table;
    if (is_new) {
        tables[table_id].init(table = new table_t);
    } else {
        table = tables.at(table_id).get();
    }
    new_mutex_in_line_t table_mutex_in_line(&table->mutex);
    global_mutex_acq.reset();
    wait_interruptible(table_mutex_in_line.acq_signal(), interruptor);

    /* Validate existing record */
    if (!is_new) {
        guarantee(table->multistore_ptr.has() ==
            (table->status == table_t::status_t::ACTIVE));
        guarantee(table->active.has() ==
            (table->status == table_t::status_t::ACTIVE));
        guarantee(static_cast<bool>(table->second_hand_config) ==
            (table->status == table_t::status_t::INACTIVE));
        guarantee(static_cast<bool>(table->second_hand_timestamp) ==
            (table->status == table_t::status_t::INACTIVE));
        if (table->status == table_t::status_t::INACTIVE) {
            guarantee(!table->second_hand_timestamp->is_deletion());
        }
    }

    /* Reject outdated or redundant messages */
    if (!is_new) {
        multi_table_manager_bcard_t::timestamp_t current_timestamp;
        switch (table->status) {
            case table_t::status_t::ACTIVE:
                current_timestamp.epoch = table->active->manager.epoch;
                current_timestamp.log_index =
                    table->active->get_raft()->get_committed_state()->get().log_index;
                break;
            case table_t::status_t::INACTIVE:
                current_timestamp = *table->second_hand_timestamp;
                break;
            case table_t::status_t::DELETION:
                current_timestamp = multi_table_bcard_t::timestamp_t::deletion();
                break;
            default: unreachable();
        }
        if (timestamp.supersedes(current_timestamp)) {
            if (!ack_addr.is_nil()) {
                send(mailbox_manager, ack_addr);
            }
            return;
        }
    }

    guarantee(table->status != table_t::status_t::DELETION,
        "It shouldn't be possible to undelete a table.");

    /* Bring record up to date */
    if (action_status == action_status_t::ACTIVE) {
        if (is_new || table->status != table_t::status_t::ACTIVE)) {
            /* The table is being created, or we are joining it */
            table->status = table_t::status_t::ACTIVE;

            /* If we were `INACTIVE` before, clean up `second_hand_config` and
            `second_hand_timestamp` because we won't be using them anymore */
            table->second_hand_config = boost::none;
            table->second_hand_timestamp = boost::none;

            /* Write a record to disk for the new table */
            pesistence_interface->write_metadata(
                table_id,
                table_persistent_state_t::active(
                    timestamp.epoch, *raft_member_id, *initial_raft_state),
                interruptor);

            /* Open files for the new table. We do this after writing the record, because
            this way if we crash we won't leak the file. */
            persistence_interface->create_multistore(
                table_id,
                &table->multistore_ptr,
                interruptor);

            /* Create the `active_table_t`, which contains the `raft_member_t` and does
            all of the important work of actually handing queries */
            table->active = make_scoped<active_table_t>(
                this, table_id, timestamp.epoch, *raft_member_id, *initial_raft_state,
                table->multistore_ptr.get());

            logDBG("Added replica for table %s", uuid_to_str(table_id).c_str());

        } else if (table->active->manager.epoch != timestamp.epoch ||
                table->active->manager.raft_member_id != *raft_member_id {
            /* The table was in an active state before, and it should still be in an
            active state; but the epoch or member ID changed. So we have to destroy and
            then re-create `table->active`. */

            table->active.reset();

            /* Update the record on disk */
            pesistence_interface->write_metadata(
                table_id,
                table_persistent_state_t::active(
                    timestamp.epoch, *raft_member_id, *initial_raft_state),
                interruptor);

            table->active = make_scoped<active_table_t>(
                this, table_id, timestamp.epoch, *raft_member_id, *initial_raft_state,
                table->multistore_ptr.get());

            logDBG("Reset replica for table %s", uuid_to_str(table_id).c_str());
        }

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
                table_id, &table->multistore_ptr, interruptor);

            logDBG("Removed replica for table %s", uuid_to_str(table_id).c_str());
        }

        /* We just found out about this table, or we are updating an existing
        second-hand information record with newer information */
        table->status = table_t::status_t::INACTIVE;

        table->second_hand_config = boost::make_optional(*basic_config);
        table->second_hand_timestamp = boost::make_optional(timestamp);

        /* Update the record on disk */
        persistence_interface->write_metadata(
            table_id,
            table_persistent_state_t::inactive(*basic_config, timestamp),
            interruptor);

    } else if (action_status == action_status_t::DELETED) {
        if (!is_new) {
            /* Clean up the table's current records */
            if (table->status == table_t::status_t::ACTIVE) {
                table->active.reset();

                /* Destroy files for the table before we update the metadata record, so
                that if we crash we won't leak the file. */
                persistence_interface->destroy_multistore(
                    table_id, &table->multistore_ptr, interruptor);

                logDBG("Deleted table %s", uuid_to_str(table_id).c_str());
            } else {
                table->second_hand_config = boost::none;
                table->second_hand_timestamp = boost::none;
            }
        }

        table->status = table_t::status_t::DELETED;

        /* Remove the record on disk */
        persistence_interface->delete_metadata(table_id, interruptor);

    }

    /* If we're in the `ACTIVE` or `DELETED` state, sync this table to all other peers.
    This is usually redundant, but if we encounter non-transitive connectivity it can
    prevent a stall. For example, suppose that server A creates a new table, which is
    hosted on B, C, and D; but the messages from A to C and D are lost. This code path
    will cause B to forward the messages to C and D instead of being stuck in a limbo
    state. */
    if (table->status != table_t::status_t::INACTIVE) {
        multi_table_manager_directory->read_all(
        [&](const peer_id_t &peer, const multi_table_manager_bcard_t *) {
            if (peer != mailbox_manager->get_me()) {
                schedule_sync(table_id, table, peer);
            }
        });
    }

    if (!ack_addr.is_nil()) {
        send(mailbox_manager, ack_addr);
    }
}

void multi_table_manager_t::on_get_config(
        signal_t *interruptor,
        const boost::optional<namespace_id_t> &table_id,
        const mailbox_t<void(
            std::map<namespace_id_t, table_config_and_shards_t>
            )>::address_t &reply_addr) {
    std::map<namespace_id_t, table_config_and_shards_t> result;
    if (static_cast<bool>(table_id)) {
        /* Fetch information for a specific table. */
        mutex_assertion_t::acq_t global_mutex_acq(&mutex);
        auto it = tables.find(*table_id);
        if (it != tables.end()) {
            new_mutex_in_line_t table_mutex_in_line(&it->second->mutex);
            global_mutex_acq.reset();
            wait_interruptible(table_mutex_in_line.acq_signal(), interruptor);
            if (it->second->status == table_t::status_t::ACTIVE) {
                it->second->active->get_raft()->get_committed_state()->apply_read(
                [&](const raft_member_t<table_raft_state_t>::state_and_config_t *s) {
                    result[*table_id] = s->state.config;
                });
            }
        }
    } else {
        /* Fetch information for all tables that we know about. First we get in line for
        each mutex, then we release the global mutex assertion, then we wait for each
        mutex to be ready and copy out its data. */
        mutex_assertion_t::acq_t global_mutex_acq(&mutex);
        std::map<namespace_id_t, scoped_ptr_t<new_mutex_in_line_t> >
            table_mutex_in_lines;
        for (const auto &pair : tables) {
            table_mutex_in_lines[pair.first] =
                make_scoped<new_mutex_in_line_t>(&pair.second->mutex);
        }
        global_mutex_acq.reset();
        for (const auto &pair : table_mutex_in_lines) {
            wait_interruptible(pair.second->acq_signal(), interruptor);
            auto it = tables.find(pair.first);
            guarantee(it != tables.end());
            if (it->second->status == table_t::status_t::ACTIVE) {
                it->second->active->get_raft()->get_committed_state()->apply_read(
                [&](const raft_member_t<table_raft_state_t>::state_and_config_t *s) {
                    result[pair.first] = s->state.config;
                });
            }
        }
    }
    send(mailbox_manager, reply_addr, result);
}

void multi_table_manager_t::do_sync(
        const namespace_id_t &table_id,
        const table_t &table,
        const server_id_t &other_server_id,
        const boost::optional<table_manager_bcard_t> &table_bcard,
        const multi_table_manager_bcard_t &table_manager_bcard) {
    typedef multi_table_manager_bcard_t::status_t action_status_t;

    if (table->status == table_t::status_t::ACTIVE) {
        multi_table_manager_bcard_t::timestamp_t timestamp;
        timestamp.epoch = table.active->manager.epoch;
        action_status_t action_status;
        boost::optional<table_basic_config_t> basic_config;
        boost::optional<raft_member_id_t> raft_member_id;
        boost::optional<raft_persistent_state_t<table_raft_state_t> >
            initial_raft_state;
        table.active->get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *st) {
                timestamp.log_index = st->log_index;
                auto it = st->state.member_ids.find(other_server_id);
                if (it != st->state.member_ids.end()) {
                    action_status = action_status_t::ACTIVE;
                    raft_member_id = boost::make_optional(it->second);
                    initial_raft_state = boost::make_optional(
                        table.active->get_raft()->get_state_for_init());
                } else {
                    action_status = action_status_t::INACTIVE;
                    basic_config = boost::make_optional(st->state.config.config.basic);
                }
            });

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
            table_id, timestamp, action_status,
            basic_config, raft_member_id, initial_raft_state,
            mailbox_t<void()>::address_t());

    } else if (table->status == table_t::status_t::INACTIVE) {
        if (static_cast<bool>(table_bcard)) {
            /* No point in sending a `MAYBE_ACTIVE` message to a server that's actually
            hosting the table already (it would be a noop) */
            return;
        }

        send(mailbox_manager, table_manager_bcard.action_mailbox,
            table_id, *table->second_hand_timestamp, action_status_t::MAYBE_ACTIVE,
            *table->second_hand_config, boost;:optional<raft_member_id_t>(),
            boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
            mailbox_t<void()>::address_t());

    } else if (table->status == table_t::status_t::DELETED) {
        send(mailbox_manager, table_manager_bcard.action_mailbox,
            table_id, multi_table_manager_t::timestamp_t::deletion(),
            action_status_t::DELETED,
            boost::optional<table_basic_config_t>(), boost;:optional<raft_member_id_t>(),
            boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
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
                new_mutex_in_line_t table_mutex_in_line(&table->mutex);
                global_mutex_acq.reset();
                wait_interruptible(table_mutex_in_line.acq_signal(),
                    keepalive.get_drain_signal());
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

