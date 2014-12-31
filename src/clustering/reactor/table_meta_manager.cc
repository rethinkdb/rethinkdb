// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/table_meta_manager.hpp"

table_meta_manager_t::table_meta_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_meta_manager_business_card_t>
            *_table_meta_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_business_card_t>
            *_table_meta_directory,
        table_meta_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        backfill_throttler_t *_backfill_throttler,
        branch_history_manager_t *_branch_history_manager,
        perfmon_collection_t *_perfmon_collection,
        rdb_context_t *_rdb_context,
        watchable_map_t<
            std::pair<peer_id_t, namespace_id_t>,
            namespace_directory_metadata_t
            > *_reactor_directory_view) :
    server_id(_server_id),
    mailbox_manager(_mailbox_manager),
    table_meta_manager_directory(_table_meta_manager_directory),
    table_meta_directory(_table_meta_directory),
    persistence_interface(_persistence_interface),
    base_path(_base_path),
    backfill_throttler(_backfill_throttler),
    branch_history_manager(_branch_history_manager),
    perfmon_collection(_perfmon_collection),
    rdb_context(_rdb_context),
    reactor_directory_view(_reactor_directory_view),
    table_meta_manager_directory_subs(
        table_meta_manager_directory,
        [this](const peer_id_t &peer, const table_meta_manager_business_card_t *) {
            mutex_assertion_t::acq_t mutex_acq(&mutex);
            for (const auto &pair : tables) {
                schedule_sync(pair.first, &pair.second, peer);
            }
        }, false),
    table_manager_directory_subs(
        table_manager_directory,
        [this](const std::pair<peer_id_t, namespace_id_t> &key,
                const table_manager_business_card_t *) {
            mutex_assertion_t::acq_t mutex_acq(&mutex);
            auto it = tables.find(key.second);
            if (it != tables.end()) {
                schedule_sync(key.second, &it->second, key.first);
            }
        }, false),
    action_mailbox(mailbox_manager,
        std::bind(&table_meta_manager_t::on_action, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6))
{
    /* Resurrect any tables that were sitting on disk from when we last shut down */
    cond_t non_interruptor;
    persistence_interface->read_all_tables(
        [&](const table_id_t &table_id, const table_meta_persistent_state_t &state,
                scoped_ptr_t<multistore_ptr_t> &&multistore_ptr) {
            mutex_assertion_t::acq_t global_mutex_acq(&mutex);
            guarantee(tables.count(table_id) == 0);
            table_t *table = &tables[table_id];
            new_mutex_acq_t table_mutex_acq(&table->mutex);
            global_mutex_acq.reset();
            table->multistore_ptr = std::move(multistore_ptr);
            table->active = make_scoped<active_table_t>(this, table_id, state.member_id,
                state.raft_state, table->multistore_ptr.get());
            raft_member_t<table_raft_state_t>::state_and_config_t raft_state =
                table->active->raft->get_raft()->get_committed_state()->get();
            guarantee(raft_state.state.member_ids.at(server_id) == state.member_id,
                "Somehow we persisted a state in which we were not a member of the "
                "cluster");
            table->timestamp.epoch = state.epoch;
            table->timestamp.log_index = raft_state.log_index;
            table->is_deleted = false;

            table_meta_manager_directory->read_all_keys(
                [&](const peer_id_t &peer, const table_meta_manager_business_card_t &) {
                    schedule_sync(table_id, table, peer);
                });
        },
        &non_interruptor);
}

table_meta_manager_t::active_table_t::active_table_t(
        table_meta_manager_t *_parent,
        const namespace_id_t &_table_id,
        const table_meta_business_card_t::action_timestamp_t::epoch_t &_epoch,
        const raft_member_id_t &_member_id,
        const raft_persistent_state_t<table_raft_state_t> &initial_state,
        multistore_ptr_t *multistore_ptr) :
    parent(_parent),
    table_id(_table_id),
    member_id(_member_id),
    raft(member_id, parent->mailbox_manager, &directory_transform, this, initial_state),
    table_manager(
        parent->server_id,
        table_id,
        parent->mailbox_manager,
        raft.get_raft(),
        multistore_ptr,
        parent->base_path,
        parent->io_backender,
        parent->backfill_throttler,
        parent->branch_history_manager,
        parent->perfmon_collection,
        parent->rdb_context,
        parent->reactor_directory_view)
{
    table_meta_business_card_t bcard;
    bcard.epoch = _epoch;
    bcard.raft_member_id = member_id;
    bcard.raft_business_card = raft.get_business_card();
    bcard.table_business_card = table_manager.get_business_card();
    bcard.server_id = parent->server_id;
    parent->table_meta_manager.set_key(table_id, bcard);
}

table_meta_manager_t::active_table_t::~active_table_t() {
    parent->table_meta_manager.delete_key(table_id);
}

void table_meta_manager_t::on_action(
        signal_t *interruptor,
        const namespace_id_t &table_id,
        const table_meta_business_card_t::action_timestamp_t &timestamp,
        bool is_deletion,
        const boost::optional<raft_member_id_t> &member_id,
        const boost::optional<raft_persistent_state_t<table_raft_state_t> >
            &initial_state) {
    /* Validate the incoming message */
    guarantee(static_cast<bool>(member_id) == static_cast<bool>(initial_state));
    guarantee(!(is_deletion && static_cast<bool>(member_id)));
    guarantee(is_deletion == timestamp.epoch.id.is_nil());

    /* Find or create the table record for this table */
    mutex_assertion_t global_mutex_acq(&mutex);
    bool is_new = (tables.count(table_id) == 0);
    table_t *table = tables[table_id];   /* might create a new record */
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
        return;
    }

    /* Bring record up to date */
    if (static_cast<bool>(member_id) && !table->active.has()) {
        /* The table is being created, or we are joining it */
        table_meta_persistent_state_t st;
        st.epoch = timestamp.epoch;
        st.member_id = *member_id;
        st.raft_state = *initial_state;
        persistence_interface->add_table(
            table_id, st, &table->multistore_ptr, interruptor);
        table->active = make_scoped<active_table_t>(this, table_id, timestamp.epoch,
            *member_id, *initial_state, table->multistore_ptr.get());
    } else if (!static_cast<bool>(member_id) && table->active.has()) {
        /* The table is being dropped, or we are leaving it */
        table->active.reset();
        table->multistore_ptr.reset();
        persistence_interface->remove_table(table_id, interruptor);
    } else if (static_cast<bool>(member_id) && table->active.has() &&
            (table->timestamp.epoch != timestamp.epoch ||
                table->active->member_id != *member_id)) {
        /* If the epoch changed, then the table's configuration was manually overridden.
        If the member ID changed, then we were removed and then re-added, but we never
        processed the removal message. In either case, we have to destroy and re-create
        `table->active`. */
        table->active.reset();
        table_meta_persistent_state_t st;
        st.epoch = timestamp.epoch;
        st.member_id = *member_id;
        st.raft_state = *initial_state;
        persistence_interface->update_table(table_id, st, interruptor);
        table->active = make_scoped<active_table_t>(this, table_id, timestamp.epoch,
            *member_id, *initial_state, table->multistore_ptr.get());
    }

    table->timestamp = timestamp;
    table->is_deleted = is_deletion;
}

void table_meta_manager_t::do_sync(
        const namespace_id_t &table_id,
        const table_t &table,
        const server_id_t &server_id,
        const table_meta_business_card_t *table_bcard,
        const table_meta_manager_business_card_t &table_manager_bcard) {
    if (table.is_deleted && table_bcard != nullptr) {
        send(mailbox_manager, table_manager_bcard.action,
            table_id, table.timestamp, true, boost::none, boost::none);
    } else if (table->active.has()) {
        raft_log_index_t log_index;
        boost::optional<member_id_t> member_id;
        table->active->raft->get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t &st) {
                log_index = st.log_index;
                auto it = st.state.member_ids.find(server_id);
                if (it != st.state.member_ids.end()) {
                    member_id = it->second;
                }
            });
        if (static_cast<bool>(member_id) != static_cast<bool>(table_bcard) ||
                (static_cast<bool>(member_id) && static_cast<bool>(table_bcard) &&
                    (table->timestamp.epoch.supersedes(table_bcard->epoch) ||
                        *member_id != table_bcard->member_id))) {
            boost::optional<raft_persistent_state_t<table_raft_state_t> > initial_state;
            if (static_cast<bool>(member_id)) {
                initial_state = table->active->raft->get_raft()->get_state_for_init();
            }
            table_meta_business_card_t::action_timestamp_t timestamp;
            timestamp.epoch = table->timestamp.epoch;
            timestamp.log_index = log_index;
            send(mailbox_manager, table_manager_bcard.action,
                table_id, timestamp, false, member_id, initial_state);
        }
    }
}

void table_meta_manager_t::schedule_sync(
        const namespace_id_t &table_id,
        table_id_t *table,
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
    coro_t::spawn_sometime([this, keepalive /* important to capture */, table]() {
        try {
            mutex_assertion_t global_mutex_acq(&mutex);
            while (!table->to_sync_set.empty()) {
                std::set<peer_id_t> to_sync_set;
                std::swap(table->to_sync_set, to_sync_set);
                new_mutex_in_line_t table_mutex_in_line(&table->mutex);
                global_mutex_acq.reset();
                wait_interruptible(table_mutex_in_line.acq_signal(),
                    keepalive.get_drain_signal());
                for (const peer_id_t &peer : to_sync_set) {
                    boost::optional<table_meta_manager_business_card_t>
                        table_meta_manager_bcard =
                            table_meta_manager_directory->get_key(peer);
                    if (!static_cast<bool>(table_meta_manager_bcard)) {
                        /* Peer is not connected. */
                        continue;
                    }
                    do_sync(
                        table_id,
                        *table,
                        table_meta_manager_bcard->server_id,
                        table_manager_directory->get(std::make_pair(peer, table_id)),
                        *table_meta_manager_bcard);
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

