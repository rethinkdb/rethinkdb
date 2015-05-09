// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_manager.hpp"

#include "clustering/generic/minidir.tcc"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.tcc"

table_manager_t::table_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        backfill_throttler_t *_backfill_throttler,
        table_persistence_interface_t *_persistence_interface,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        const namespace_id_t &_table_id,
        const multi_table_manager_bcard_t::timestamp_t::epoch_t &_epoch,
        const raft_member_id_t &_raft_member_id,
        const raft_persistent_state_t<table_raft_state_t> &initial_state,
        multistore_ptr_t *multistore_ptr,
        perfmon_collection_t *perfmon_collection_namespace) :
    table_id(_table_id),
    epoch(_epoch),
    raft_member_id(_raft_member_id),
    mailbox_manager(_mailbox_manager),
    persistence_interface(_persistence_interface),
    perfmon_membership(perfmon_collection_namespace, &perfmon_collection, "regions"),
    raft(raft_member_id, _mailbox_manager, raft_directory.get_values(), this,
        initial_state, "Table " + uuid_to_str(table_id)),
    table_manager_bcard(table_manager_bcard_t()),   /* we'll set this later */
    execution_bcard_read_manager(_mailbox_manager),
    contract_executor(
        _server_id, mailbox_manager,
        raft.get_raft()->get_committed_state()->subview(
            [](const raft_member_t<table_raft_state_t>::state_and_config_t &sc)
                    -> table_raft_state_t {
                return sc.state;
            }),
        execution_bcard_read_manager.get_values(), multistore_ptr, _base_path,
        _io_backender, _backfill_throttler, &perfmon_collection),
    execution_bcard_write_manager(
        mailbox_manager,
        contract_executor.get_local_contract_execution_bcards(),
        execution_bcard_minidir_directory.get_values()),
    contract_ack_write_manager(
        mailbox_manager,
        contract_executor.get_acks(),
        contract_ack_minidir_directory.get_values()),
    sindex_manager(
        multistore_ptr,
        raft.get_raft()->get_committed_state()->subview(
            [](const raft_member_t<table_raft_state_t>::state_and_config_t &sc)
                    -> table_config_t {
                return sc.state.config.config;
            })),
    get_status_mailbox(
        mailbox_manager,
        std::bind(&table_manager_t::on_get_status, this, ph::_1, ph::_2)),
    table_directory_subs(
        _table_manager_directory,
        std::bind(&table_manager_t::on_table_directory_change, this, ph::_1, ph::_2),
        true),
    raft_readiness_subs(std::bind(&table_manager_t::on_raft_readiness_change, this))
{
    guarantee(!raft_member_id.is_nil());
    guarantee(!epoch.id.is_unset());

    /* Set up the initial table bcard */
    {
        table_manager_bcard_t bcard;
        bcard.timestamp.epoch = epoch;
        raft.get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
                bcard.timestamp.log_index = sc->log_index;
            });
        bcard.raft_member_id = raft_member_id;
        bcard.raft_business_card = raft.get_business_card();
        bcard.execution_bcard_minidir_bcard = execution_bcard_read_manager.get_bcard();
        bcard.get_status_mailbox = get_status_mailbox.get_address();
        bcard.server_id = _server_id;
        table_manager_bcard.set_value_no_equals(bcard);
    }

    {
        watchable_t<bool>::freeze_t freeze(raft.get_raft()->get_readiness_for_change());
        raft_readiness_subs.reset(raft.get_raft()->get_readiness_for_change(), &freeze);
    }
}

table_manager_t::~table_manager_t() {
    /* This is defined in the `.cc` file so that we can see and instantiate the
    destructors for the templated types `raft_networked_member_t` and
    `minidir_write_manager_t`. The destructors are defined in `.tcc` files which are
    visible here but not in the `.hpp`. */
}

table_manager_t::leader_t::leader_t(table_manager_t *_parent) :
    parent(_parent),
    contract_ack_read_manager(parent->mailbox_manager),
    coordinator(parent->get_raft(), contract_ack_read_manager.get_values(),
        "Table " + uuid_to_str(parent->table_id)),
    set_config_mailbox(parent->mailbox_manager,
        std::bind(&leader_t::on_set_config, this, ph::_1, ph::_2, ph::_3))
{
    parent->table_manager_bcard.apply_atomic_op([&](table_manager_bcard_t *bcard) {
        table_manager_bcard_t::leader_bcard_t leader_bcard;
        leader_bcard.uuid = generate_uuid();
        leader_bcard.set_config_mailbox = set_config_mailbox.get_address();
        leader_bcard.contract_ack_minidir_bcard = contract_ack_read_manager.get_bcard();
        bcard->leader = boost::make_optional(leader_bcard);
        return true;
    });
}

table_manager_t::leader_t::~leader_t() {
    parent->table_manager_bcard.apply_atomic_op([&](table_manager_bcard_t *bcard) {
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
        send(parent->mailbox_manager, reply_addr,
            boost::make_optional(timestamp));
    } else {
        send(parent->mailbox_manager, reply_addr,
            boost::optional<multi_table_manager_bcard_t::timestamp_t>());
    }
}

void table_manager_t::write_persistent_state(
        const raft_persistent_state_t<table_raft_state_t> &inner_ps,
        signal_t *interruptor) {
    table_persistent_state_t::active_t active;
    active.epoch = epoch;
    active.raft_member_id = raft_member_id;
    active.raft_state = inner_ps;
    table_persistent_state_t outer_ps;
    outer_ps.value = active;
    persistence_interface->write_metadata(table_id, outer_ps, interruptor);
}

void table_manager_t::on_get_status(
        signal_t *interruptor,
        const mailbox_t<void(
            std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >,
            contracts_and_contract_acks_t
            )>::address_t &reply_addr) {
    std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > sindex_status =
        sindex_manager.get_status(interruptor);

    contracts_and_contract_acks_t contracts_and_contract_acks;
    {
        /* Note that despite the `ASSERT_NO_CORO_WAITING` there may be contract
           acknowledgements in `contract_acks` that refer to a contract that is not in
           `contracts`.

           This may happen because of the two-step process in
           `contract_executor_t::update_blocking` which first resets the executor and
           only then removes the acknowledgement from the `ack_map`. */
        ASSERT_NO_CORO_WAITING;

        contracts_and_contract_acks.timestamp.epoch = epoch;
        contracts_and_contract_acks.timestamp.log_index =
            raft.get_raft()->get_committed_state()->get().log_index;
        contracts_and_contract_acks.contracts =
            raft.get_raft()->get_committed_state()->get().state.contracts;
        for (const auto &contract_ack : contract_executor.get_acks()->get_all()) {
            contracts_and_contract_acks.contract_acks.insert(
                std::make_pair(contract_ack.first.second, contract_ack.second));
        }
    }

    send(mailbox_manager, reply_addr, sindex_status, contracts_and_contract_acks);
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

