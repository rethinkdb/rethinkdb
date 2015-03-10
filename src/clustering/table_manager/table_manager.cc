// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_manager.hpp"

#include "clustering/generic/minidir.tcc"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.tcc"
#include "clustering/table_manager/multi_table_manager.hpp"

table_manager_t::leader_t::leader_t(table_manager_t *_parent) :
    parent(_parent),
    contract_ack_read_manager(parent->parent->mailbox_manager),
    coordinator(parent->get_raft(), contract_ack_read_manager.get_values()),
    set_config_mailbox(parent->parent->mailbox_manager,
        std::bind(&leader_t::on_set_config, this, ph::_1, ph::_2, ph::_3))
{
    parent->bcard_entry->change([&](table_manager_bcard_t *bcard) {
        table_manager_bcard_t::leader_bcard_t leader_bcard;
        leader_bcard.uuid = generate_uuid();
        leader_bcard.set_config_mailbox = set_config_mailbox.get_address();
        leader_bcard.contract_ack_minidir_bcard = contract_ack_read_manager.get_bcard();
        bcard->leader = boost::make_optional(leader_bcard);
        return true;
    });
}

table_manager_t::leader_t::~leader_t() {
    parent->bcard_entry->change([&](table_manager_bcard_t *bcard) {
        bcard->leader = boost::none;
        return true;
    });
}

void table_manager_t::leader_t::on_set_config(
        signal_t *interruptor,
        const table_config_and_shards_t &new_config,
        const mailbox_t<void(
            boost::optional<multi_table_manager_bcard_t::timestamp_t>
            )>::address_t &reply_addr) {
    boost::optional<raft_log_index_t> result = coordinator.change_config(
        [&](table_config_and_shards_t *config) { *config = new_config; },
        interruptor);
    if (static_cast<bool>(result)) {
        multi_table_manager_bcard_t::timestamp_t timestamp;
        timestamp.epoch = parent->epoch;
        timestamp.log_index = *result;
        send(parent->parent->mailbox_manager, reply_addr,
            boost::make_optional(timestamp));
    } else {
        send(parent->parent->mailbox_manager, reply_addr,
            boost::optional<multi_table_manager_bcard_t::timestamp_t>());
    }
}

table_manager_t::table_manager_t(
        multi_table_manager_t *_parent,
        const namespace_id_t &_table_id,
        const multi_table_manager_bcard_t::timestamp_t::epoch_t &_epoch,
        const raft_member_id_t &_member_id,
        const raft_persistent_state_t<table_raft_state_t> &initial_state,
        multistore_ptr_t *multistore_ptr) :
    table_id(_table_id),
    epoch(_epoch),
    member_id(_member_id),
    parent(_parent),
    perfmon_membership(
        &get_global_perfmon_collection(), &perfmon_collection, uuid_to_str(_table_id)),
    raft(member_id, parent->mailbox_manager, raft_directory.get_values(), this,
        initial_state),
    execution_bcard_read_manager(parent->mailbox_manager),
    contract_executor(
        parent->server_id, parent->mailbox_manager,
        raft.get_raft()->get_committed_state()->subview(
            [](const raft_member_t<table_raft_state_t>::state_and_config_t &sc)
                    -> table_raft_state_t {
                return sc.state;
            }),
        execution_bcard_read_manager.get_values(), multistore_ptr, parent->base_path,
        parent->io_backender, &parent->backfill_throttler, &perfmon_collection),
    execution_bcard_write_manager(
        parent->mailbox_manager,
        contract_executor.get_local_contract_execution_bcards(),
        execution_bcard_minidir_directory.get_values()),
    contract_ack_write_manager(
        parent->mailbox_manager,
        contract_executor.get_acks(),
        contract_ack_minidir_directory.get_values()),
    table_query_bcard_source(
        &parent->table_query_bcard_combiner,
        table_id,
        contract_executor.get_local_table_query_bcards()),
    table_directory_subs(
        parent->table_manager_directory,
        std::bind(&table_manager_t::on_table_directory_change, this, ph::_1, ph::_2),
        true),
    raft_committed_subs(std::bind(&table_manager_t::on_raft_committed_change, this)),
    raft_readiness_subs(std::bind(&table_manager_t::on_raft_readiness_change, this))
{
    guarantee(!member_id.is_nil());
    guarantee(!epoch.id.is_unset());

    /* Set up the initial table bcard */
    {
        table_manager_bcard_t bcard;
        bcard.timestamp.epoch = epoch;
        raft.get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
                bcard.timestamp.log_index = sc->log_index;
                bcard.database = sc->state.config.config.database;
                bcard.name = sc->state.config.config.name;
                bcard.primary_key = sc->state.config.config.primary_key;
            });
        bcard.raft_member_id = member_id;
        bcard.raft_business_card = raft.get_business_card();
        bcard.execution_bcard_minidir_bcard = execution_bcard_read_manager.get_bcard();
        bcard.server_id = parent->server_id;
        bcard_entry = make_scoped<watchable_map_var_t<namespace_id_t,
            table_manager_bcard_t>::entry_t>(
                &parent->table_manager_bcards, table_id, bcard);
    }

    {
        watchable_t<raft_member_t<table_raft_state_t>::state_and_config_t>::freeze_t
            freeze(raft.get_raft()->get_committed_state());
        raft_committed_subs.reset(raft.get_raft()->get_committed_state(), &freeze);
    }

    {
        watchable_t<bool>::freeze_t freeze(raft.get_raft()->get_readiness_for_change());
        raft_readiness_subs.reset(raft.get_raft()->get_readiness_for_change(), &freeze);
    }
}

void table_manager_t::write_persistent_state(
        const raft_persistent_state_t<table_raft_state_t> &inner_ps,
        signal_t *interruptor) {
    table_persistent_state_t outer_ps;
    outer_ps.epoch = epoch;
    outer_ps.member_id = member_id;
    outer_ps.raft_state = inner_ps;
    parent->persistence_interface->update_table(table_id, outer_ps, interruptor);
}

void table_manager_t::on_table_directory_change(
        const std::pair<peer_id_t, namespace_id_t> &key,
        const table_manager_bcard_t *bcard) {
    if (key.second == table_id) {
        /* Update `raft_directory` */
        if (bcard != nullptr && bcard->timestamp.epoch == epoch) {
            raft_directory.set_key(
                key.first,
                bcard->raft_member_id,
                bcard->raft_business_card);
        } else {
            raft_directory.delete_key(key.first);
        }

        /* Update `execution_bcard_minidir_directory` */
        if (bcard != nullptr) {
            execution_bcard_minidir_directory.set_key(
                key.first,
                bcard->server_id,
                bcard->execution_bcard_minidir_bcard);
        } else {
            execution_bcard_minidir_directory.delete_key(key.first);
        }

        /* Update `current_ack_minidir_directory` */
        if (bcard != nullptr && static_cast<bool>(bcard->leader)) {
            contract_ack_minidir_directory.set_key(
                key.first,
                bcard->leader->uuid,
                bcard->leader->contract_ack_minidir_bcard);
        } else {
            contract_ack_minidir_directory.delete_key(key.first);
        }
    }
}

void table_manager_t::on_raft_committed_change() {
    /* Check if the table's name or database changed, and update the bcard if so */
    raft.get_raft()->get_committed_state()->apply_read(
    [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
        bcard_entry->change([&](table_manager_bcard_t *bcard) {
            if (sc->state.config.config.name != bcard->name ||
                    sc->state.config.config.database != bcard->database) {
                bcard->timestamp.log_index = sc->log_index;
                bcard->name = sc->state.config.config.name;
                bcard->database = sc->state.config.config.database;
                return true;
            } else {
                /* If we return `false`, updates won't be sent to the other servers. This
                way, we don't send a message to every server every time Raft commits a
                change. */
                return false;
            }
        });
    });

    /* Every time the Raft cluster commits a transaction, we re-sync to every other
    server in the cluster. This is because the transaction might consist of adding or
    removing a server, in which case we need to notify that server. */
    mutex_assertion_t::acq_t mutex_acq(&parent->mutex);
    auto it = parent->tables.find(table_id);
    guarantee(it != parent->tables.end());
    parent->multi_table_manager_directory->read_all(
    [&](const peer_id_t &peer, const multi_table_manager_bcard_t *) {
        if (peer != parent->mailbox_manager->get_me()) {
            parent->schedule_sync(table_id, it->second.get(), peer);
        }
    });
}

void table_manager_t::on_raft_readiness_change() {
    /* Create or destroy `leader` depending on whether we are the Raft cluster's leader.
    Since `leader_t`'s constructor and destructor may block, we have to spawn a
    coroutine to do it. */
    auto_drainer_t::lock_t keepalive(&drainer);
    coro_t::spawn_sometime([this, keepalive /* important to capture */]() {
        new_mutex_acq_t mutex_acq(&leader_mutex);
        bool ready = raft.get_raft()->get_readiness_for_change()->get();
        if (ready && !leader.has()) {
            leader.init(new leader_t(this));
        } else if (!ready && leader.has()) {
            leader.reset();
        }
    });
}

