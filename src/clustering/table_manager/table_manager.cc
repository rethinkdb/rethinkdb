// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_manager.hpp"

#include <functional>
#include <set>
#include <string>

#include "clustering/generic/minidir.tcc"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.tcc"

table_manager_t::table_manager_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        server_config_client_t *_server_config_client,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        backfill_throttler_t *_backfill_throttler,
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *_connections_map,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        const namespace_id_t &_table_id,
        const multi_table_manager_timestamp_t::epoch_t &_epoch,
        const raft_member_id_t &_raft_member_id,
        raft_storage_interface_t<table_raft_state_t> *raft_storage,
        const raft_start_election_immediately_t start_election_immediately,
        multistore_ptr_t *multistore_ptr,
        perfmon_collection_t *perfmon_collection_namespace) :
    table_id(_table_id),
    epoch(_epoch),
    raft_member_id(_raft_member_id),
    mailbox_manager(_mailbox_manager),
    server_config_client(_server_config_client),
    connections_map(_connections_map),
    perfmon_membership(perfmon_collection_namespace, &perfmon_collection, "regions"),
    raft(raft_member_id, _mailbox_manager, raft_directory.get_values(), raft_storage,
        "Table " + uuid_to_str(table_id), start_election_immediately),
    table_manager_bcard(table_manager_bcard_t()),   /* we'll set this later */
    raft_bcard_copier(&table_manager_bcard_t::raft_business_card,
        raft.get_business_card(), &table_manager_bcard),
    execution_bcard_read_manager(_mailbox_manager),
    contract_executor(
        _server_id, mailbox_manager,
        raft.get_raft()->get_committed_state()->subview(
            [](const raft_member_t<table_raft_state_t>::state_and_config_t &sc)
                    -> table_raft_state_t {
                return sc.state;
            }),
        execution_bcard_read_manager.get_values(), multistore_ptr, _base_path,
        _io_backender, _backfill_throttler, &backfill_progress_tracker,
        &perfmon_collection),
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
    table_directory_subs(
        _table_manager_directory,
        std::bind(&table_manager_t::on_table_directory_change, this, ph::_1, ph::_2),
        initial_call_t::YES),
    raft_readiness_subs(std::bind(&table_manager_t::on_raft_readiness_change, this))
{
    guarantee(!raft_member_id.is_nil());
    guarantee(!epoch.is_unset());

    /* Set up the initial table bcard */
    {
        table_manager_bcard_t bcard;
        bcard.timestamp.epoch = epoch;
        raft.get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
                bcard.timestamp.log_index = sc->log_index;
            });
        bcard.raft_member_id = raft_member_id;
        bcard.raft_business_card = raft.get_business_card()->get();
        bcard.execution_bcard_minidir_bcard = execution_bcard_read_manager.get_bcard();
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

void table_manager_t::get_status(
        const table_status_request_t &request,
        signal_t *interruptor,
        table_status_response_t *response)
        THROWS_ONLY(interrupted_exc_t) {
    if (request.want_config) {
        get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *s) {
                response->config.set(s->state.config);
            });
    }
    if (request.want_sindexes) {
        response->sindexes = sindex_manager.get_status(interruptor);
    }
    if (request.want_raft_state) {
        get_raft()->get_committed_state()->apply_read(
            [&](const raft_member_t<table_raft_state_t>::state_and_config_t *s) {
                response->raft_state.set(s->state);
                multi_table_manager_timestamp_t ts;
                ts.epoch = epoch;
                ts.log_index = s->log_index;
                response->raft_state_timestamp.set(ts);
            });
    }
    if (request.want_contract_acks) {
        contract_executor.get_acks()->read_all(
        [&](const std::pair<server_id_t, contract_id_t> &k, const contract_ack_t *ack) {
            response->contract_acks.insert(std::make_pair(k.second, *ack));
        });
    }
    if (request.want_shard_status) {
        response->shard_status = contract_executor.get_shard_status();
    }
    if (request.want_all_replicas_ready) {
        switch (request.all_replicas_ready_mode) {
        case all_replicas_ready_mode_t::EXCLUDE_RAFT_TEST: {
            rwlock_in_line_t leader_in_line(&leader_lock, access_t::read);
            // If we cannot acquire the leader_lock immediately, that implies
            // that the leader is currently transitioning. Instead of waiting for the
            // lock, we simply bail out and report not all replicas ready.
            if (leader_in_line.read_signal()->is_pulsed() && static_cast<bool>(leader)) {
                response->all_replicas_ready =
                    leader->get_contract_coordinator()->
                        check_outdated_all_replicas_ready(interruptor);
            } else {
                response->all_replicas_ready = false;
            }
        } break;
        case all_replicas_ready_mode_t::INCLUDE_RAFT_TEST: {
            rwlock_acq_t leader_acq(&leader_lock, access_t::read, interruptor);
            if (static_cast<bool>(leader)) {
                response->all_replicas_ready =
                    leader->get_contract_coordinator()->
                        check_all_replicas_ready(interruptor);
            } else {
                response->all_replicas_ready = false;
            }
        } break;
        default:
            unreachable();
        }
    }
}

table_manager_t::leader_t::leader_t(table_manager_t *_parent) :
    parent(_parent),
    contract_ack_read_manager(parent->mailbox_manager),
    coordinator(parent->get_raft(), contract_ack_read_manager.get_values(),
        parent->connections_map),
    server_name_cache_updater(parent->get_raft(), parent->server_config_client),
    set_config_mailbox(parent->mailbox_manager,
        std::bind(&leader_t::on_set_config, this, ph::_1, ph::_2, ph::_3))
{
    parent->table_manager_bcard.apply_atomic_op([&](table_manager_bcard_t *bcard) {
        table_manager_bcard_t::leader_bcard_t leader_bcard;
        leader_bcard.uuid = generate_uuid();
        leader_bcard.set_config_mailbox = set_config_mailbox.get_address();
        leader_bcard.contract_ack_minidir_bcard = contract_ack_read_manager.get_bcard();
        bcard->leader = make_optional(leader_bcard);
        return true;
    });
}

table_manager_t::leader_t::~leader_t() {
    parent->table_manager_bcard.apply_atomic_op([&](table_manager_bcard_t *bcard) {
        bcard->leader = r_nullopt;
        return true;
    });
}

void table_manager_t::leader_t::on_set_config(
        signal_t *interruptor,
        const table_config_and_shards_change_t &table_config_and_shards_change,
        const mailbox_addr_t<optional<multi_table_manager_timestamp_t>, bool> &reply_addr) {
    logINF("Table %s: Configuration is changing.",
        uuid_to_str(parent->table_id).c_str());
    bool is_change_successful = false;
    optional<raft_log_index_t> result = coordinator.change_config(
        [&](table_config_and_shards_t *config_and_shards) {
            is_change_successful =
                table_config_and_shards_change.apply_change(config_and_shards);
        },
        interruptor);
    if (static_cast<bool>(result) && is_change_successful) {
        multi_table_manager_timestamp_t timestamp;
        timestamp.epoch = parent->epoch;
        timestamp.log_index = *result;
        send(parent->mailbox_manager, reply_addr,
            make_optional(timestamp), true);
    } else {
        /* If `is_change_successful` is false the change was considered a no-op and the
        returned log_index is that of the last change, which we safely ignore. */
        send(parent->mailbox_manager, reply_addr,
            optional<multi_table_manager_timestamp_t>(), is_change_successful);
    }
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
        rwlock_acq_t mutex_acq(&leader_lock, access_t::write);
        bool ready = raft.get_raft()->get_readiness_for_change()->get();
        if (ready && !leader.has()) {
            leader.init(new leader_t(this));
        } else if (!ready && leader.has()) {
            leader.reset();
        }
    });
}

