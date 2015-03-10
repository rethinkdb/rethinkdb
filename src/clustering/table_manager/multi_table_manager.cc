// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/multi_table_manager.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_manager/table_manager.hpp"
#include "logger.hpp"

/* RSI: Should be `since_N` where `N` is the version for Raft */
RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(table_persistent_state_t,
    epoch, member_id, raft_state);

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
        [&](const namespace_id_t &table_id, const table_persistent_state_t &state,
                scoped_ptr_t<multistore_ptr_t> &&multistore_ptr) {
            mutex_assertion_t::acq_t global_mutex_acq(&mutex);
            guarantee(tables.count(table_id) == 0);
            table_t *table;
            tables[table_id].init(table = new table_t);
            new_mutex_acq_t table_mutex_acq(&table->mutex);
            global_mutex_acq.reset();
            table->multistore_ptr = std::move(multistore_ptr);
            table->active = make_scoped<table_manager_t>(this, table_id, state.epoch,
                state.member_id, state.raft_state, table->multistore_ptr.get());
            raft_member_t<table_raft_state_t>::state_and_config_t raft_state =
                table->active->get_raft()->get_committed_state()->get();
            guarantee(raft_state.state.member_ids.at(server_id) == state.member_id,
                "Somehow we persisted a state in which we were not a member of the "
                "cluster");
            table->timestamp.epoch = state.epoch;
            table->timestamp.log_index = raft_state.log_index;
            table->is_deleted = false;

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

multi_table_manager_t::table_t::table_t() : sync_coro_running(false) { }

multi_table_manager_t::table_t::~table_t() {
    /* Declared in the `.cc` file so we don't have to include `table_manager.hpp` in the
    header file */
}

void multi_table_manager_t::on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const multi_table_manager_bcard_t::timestamp_t &timestamp,
        bool is_deletion,
        const boost::optional<raft_member_id_t> &member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_state,
        const mailbox_t<void()>::address_t &ack_addr) {
    /* Validate the incoming message */
    guarantee(static_cast<bool>(member_id) == static_cast<bool>(initial_state));
    guarantee(!(is_deletion && static_cast<bool>(member_id)));
    guarantee(is_deletion == timestamp.epoch.id.is_nil());

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
        guarantee(!(table->is_deleted && !is_deletion), "Can't un-delete a table");
        guarantee(table->is_deleted == table->timestamp.epoch.id.is_nil());
        guarantee(table->multistore_ptr.has() == table->active.has());
    }

    /* Reject outdated or redundant messages */
    if (!is_new && !timestamp.supersedes(table->timestamp)) {
        if (!ack_addr.is_nil()) {
            send(mailbox_manager, ack_addr);
        }
        return;
    }

    /* Bring record up to date */
    if (static_cast<bool>(member_id) && !table->active.has()) {
        /* The table is being created, or we are joining it */
        table_persistent_state_t st;
        st.epoch = timestamp.epoch;
        st.member_id = *member_id;
        st.raft_state = *initial_state;
        persistence_interface->add_table(
            table_id, st, &table->multistore_ptr, interruptor);
        table->active = make_scoped<table_manager_t>(this, table_id, timestamp.epoch,
            *member_id, *initial_state,
            table->multistore_ptr.get());
        logDBG("Added replica for table %s", uuid_to_str(table_id).c_str());
    } else if (!static_cast<bool>(member_id) && table->active.has()) {
        /* The table is being dropped, or we are leaving it */
        table->active.reset();
        table->multistore_ptr.reset();
        persistence_interface->remove_table(table_id, interruptor);
        if (is_deletion) {
            logDBG("Deleting table %s", uuid_to_str(table_id).c_str());
        } else {
            logDBG("Removed replica for table %s", uuid_to_str(table_id).c_str());
        }
    } else if (static_cast<bool>(member_id) && table->active.has() &&
            (table->timestamp.epoch != timestamp.epoch ||
                table->active->member_id != *member_id)) {
        /* If the epoch changed, then the table's configuration was manually overridden.
        If the member ID changed, then we were removed and then re-added, but we never
        processed the removal message. In either case, we have to destroy and re-create
        `table->active`. */
        table->active.reset();
        table_persistent_state_t st;
        st.epoch = timestamp.epoch;
        st.member_id = *member_id;
        st.raft_state = *initial_state;
        persistence_interface->update_table(table_id, st, interruptor);
        table->active = make_scoped<table_manager_t>(this, table_id, timestamp.epoch,
            *member_id, *initial_state, table->multistore_ptr.get());
        logDBG("Overrode replica for table %s", uuid_to_str(table_id).c_str());
    }

    table->timestamp = timestamp;
    table->is_deleted = is_deletion;

    /* Sync this table to all other peers. This is usually redundant, but if we encounter
    non-transitive connectivity it can prevent a stall. For example, suppose that server
    A creates a new table, which is hosted on B, C, and D; but the messages from A to
    C and D are lost. This code path will cause B to forward the messages to C and D
    instead of being stuck in a limbo state. */
    multi_table_manager_directory->read_all(
    [&](const peer_id_t &peer, const multi_table_manager_bcard_t *) {
        if (peer != mailbox_manager->get_me()) {
            schedule_sync(table_id, table, peer);
        }
    });

    if (!ack_addr.is_nil()) {
        send(mailbox_manager, ack_addr);
    }
}

void multi_table_manager_t::on_get_config(
        signal_t *interruptor,
        const boost::optional<namespace_id_t> &table_id,
        const mailbox_t<void(std::map<namespace_id_t, table_config_and_shards_t>)>::
            address_t &reply_addr) {
    std::map<namespace_id_t, table_config_and_shards_t> result;
    if (static_cast<bool>(table_id)) {
        /* Fetch information for a specific table. */
        mutex_assertion_t::acq_t global_mutex_acq(&mutex);
        auto it = tables.find(*table_id);
        if (it != tables.end()) {
            new_mutex_in_line_t table_mutex_in_line(&it->second->mutex);
            global_mutex_acq.reset();
            wait_interruptible(table_mutex_in_line.acq_signal(), interruptor);
            if (it->second->active.has()) {
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
            if (it->second->active.has()) {
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
    if (table.is_deleted && static_cast<bool>(table_bcard)) {
        send(mailbox_manager, table_manager_bcard.action_mailbox,
            table_id, table.timestamp, true, boost::optional<raft_member_id_t>(),
            boost::optional<raft_persistent_state_t<table_raft_state_t> >(),
            mailbox_t<void()>::address_t());
    } else if (table.active.has()) {
        raft_log_index_t log_index;
        boost::optional<raft_member_id_t> member_id;
        table.active->get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *st) {
                log_index = st->log_index;
                auto it = st->state.member_ids.find(other_server_id);
                if (it != st->state.member_ids.end()) {
                    member_id = it->second;
                }
            });
        if (static_cast<bool>(member_id) != static_cast<bool>(table_bcard) ||
                (static_cast<bool>(member_id) && static_cast<bool>(table_bcard) &&
                    (table.timestamp.epoch.supersedes(table_bcard->timestamp.epoch) ||
                        *member_id != table_bcard->raft_member_id))) {
            boost::optional<raft_persistent_state_t<table_raft_state_t> > initial_state;
            if (static_cast<bool>(member_id)) {
                initial_state = table.active->get_raft()->get_state_for_init();
            }
            multi_table_manager_bcard_t::timestamp_t timestamp;
            timestamp.epoch = table.timestamp.epoch;
            timestamp.log_index = log_index;
            send(mailbox_manager, table_manager_bcard.action_mailbox,
                table_id, timestamp, false, member_id, initial_state,
                mailbox_t<void()>::address_t());
        }
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

