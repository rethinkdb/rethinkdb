// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/executor/executor.hpp"

#include "clustering/table_contract/executor/exec_erase.hpp"
#include "clustering/table_contract/executor/exec_primary.hpp"
#include "clustering/table_contract/executor/exec_secondary.hpp"
#include "store_subview.hpp"

contract_executor_t::contract_executor_t(
        const server_id_t &_server_id,
        mailbox_manager_t *_mailbox_manager,
        const clone_ptr_t<watchable_t<table_raft_state_t> > &_raft_state,
        watchable_map_t<std::pair<server_id_t, branch_id_t>, contract_execution_bcard_t>
            *_remote_contract_execution_bcards,
        multistore_ptr_t *_multistore,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        backfill_throttler_t *_backfill_throttler,
        backfill_progress_tracker_t *_backfill_progress_tracker,
        perfmon_collection_t *_perfmons) :
    server_id(_server_id),
    raft_state(_raft_state),
    multistore(_multistore),
    perfmons(_perfmons),
    perfmon_counter(0),
    update_pumper(new pump_coro_t(
        std::bind(&contract_executor_t::update_blocking, this, ph::_1))),
    raft_state_subs([this]() { update_pumper->notify(); })
{
    execution_context.server_id = _server_id;
    execution_context.mailbox_manager = _mailbox_manager;
    execution_context.branch_history_manager = multistore->get_branch_history_manager();
    execution_context.base_path = _base_path;
    execution_context.io_backender = _io_backender;
    execution_context.backfill_progress_tracker = _backfill_progress_tracker;
    execution_context.backfill_throttler = _backfill_throttler;
    execution_context.remote_contract_execution_bcards
        = _remote_contract_execution_bcards;
    execution_context.local_contract_execution_bcards
        = &local_contract_execution_bcards;
    execution_context.local_table_query_bcards = &local_table_query_bcards;

    multistore->assert_thread();

    watchable_t<table_raft_state_t>::freeze_t freeze(raft_state);
    raft_state_subs.reset(raft_state, &freeze);
    update_pumper->notify();
}

contract_executor_t::~contract_executor_t() {
    /* Run destructors manually so that we can control how `executions` is destroyed. We
    want to first destroy every individual execution without modifying the `std::map`,
    then we want to destroy the `std::map`. This is because the `execution_t`s can
    access the `std::map` via `send_ack()` until they are all destroyed. */

    raft_state_subs.reset();

    /* Call `drain()` before setting `update_pumper` itself to null because
    `update_blocking()` will sometimes try to dereference `update_pumper`. */
    update_pumper->drain();
    update_pumper.reset();

    for (auto &&pair : executions) {
        pair.second->execution.reset();
    }
}

range_map_t<key_range_t::right_bound_t, table_shard_status_t>
contract_executor_t::get_shard_status() {
    range_map_t<key_range_t::right_bound_t, table_shard_status_t> result(
        key_range_t::right_bound_t(store_key_t::min()),
        key_range_t::right_bound_t::make_unbounded());
    raft_state->apply_read([&](const table_raft_state_t *state) {
        for (const auto &pair : state->contracts) {
            if (pair.second.second.replicas.count(server_id) == 0) {
                continue;
            }
            result.visit_mutable(
            key_range_t::right_bound_t(pair.second.first.inner.left),
            pair.second.first.inner.right,
            [&](const key_range_t::right_bound_t &,
                    const key_range_t::right_bound_t &,
                    table_shard_status_t *status) {
                ack_map.read_key(std::make_pair(server_id, pair.first),
                [&](const contract_ack_t *ack) {
                    if (ack == nullptr) {
                        status->transitioning = true;
                        return;
                    }
                    switch (ack->state) {
                        case contract_ack_t::state_t::primary_need_branch:
                            status->primary = true;
                            status->need_quorum = true;
                            break;
                        case contract_ack_t::state_t::primary_in_progress:  // fall thru
                        case contract_ack_t::state_t::primary_ready:
                            status->primary = true;
                            break;
                        case contract_ack_t::state_t::secondary_need_primary:
                            status->secondary = true;
                            status->need_primary = true;
                            break;
                        case contract_ack_t::state_t::secondary_backfilling:
                            status->secondary = true;
                            status->backfilling = true;
                            break;
                        case contract_ack_t::state_t::secondary_streaming:
                            status->secondary = true;
                            break;
                    }
                });
            });
        }
    });
    return result;
}

contract_executor_t::execution_key_t contract_executor_t::get_contract_key(
        const std::pair<region_t, contract_t> &pair,
        const branch_id_t &branch) {
    execution_key_t key;
    key.region = pair.first;
    if (static_cast<bool>(pair.second.primary) &&
            pair.second.primary->server == server_id) {
        key.role = execution_key_t::role_t::primary;
        key.primary = nil_uuid();
        key.branch = nil_uuid();
    } else if (pair.second.replicas.count(server_id) == 1) {
        key.role = execution_key_t::role_t::secondary;
        if (static_cast<bool>(pair.second.primary)) {
            key.primary = pair.second.primary->server;
        } else {
            key.primary = nil_uuid();
        }
        key.branch = branch;
    } else {
        key.role = execution_key_t::role_t::erase;
        key.primary = nil_uuid();
        key.branch = nil_uuid();
    }
    return key;
}

void contract_executor_t::update_blocking(UNUSED signal_t *interruptor) {
    std::set<execution_key_t> to_delete;
    {
        ASSERT_NO_CORO_WAITING;
        raft_state->apply_read([&](const table_raft_state_t *state) {
            update(*state, &to_delete); });
    }
    if (!to_delete.empty()) {
        for (const execution_key_t &key : to_delete) {
            auto it = executions.find(key);
            guarantee(it != executions.end());
            /* Resetting `execution` is the part that can block. */
            it->second->execution.reset();
            /* Remove the entry from the ack map, only once we are sure that the
            execution won't recreate it. */
            ack_map.delete_key(std::make_pair(server_id, it->second->contract_id));
            /* Note that we do a two-step process: first we reset `execution`, then we
            erase the entry from `executions`. This is because until we finish resetting
            `execution`, it may still be calling `send_ack()`, and it's important that
            `send_ack()` be able to see the entry in `executions`. */
            executions.erase(it);
        }
        /* Now that we've deleted the executions, `update()` is likely to have new
        instructions for us, so we should run again. */
        update_pumper->notify();
    }
}

void contract_executor_t::update(const table_raft_state_t &new_state,
                                 std::set<execution_key_t> *to_delete_out) {
    assert_thread();
    /* Go through the new contracts and try to match them to existing executions */
    std::set<execution_key_t> dont_delete;
    for (const auto &new_pair : new_state.contracts) {
        /* Extract the current branch ID for the region covered by this contract.
        If there are multiple branches for different subregions, we consider
        the branch as incoherent and don't set a branch ID. */
        branch_id_t branch = nil_uuid();
        bool branch_mismatch = false;
        new_state.current_branches.visit(new_pair.second.first,
        [&](const region_t &, const branch_id_t &b) {
            if (branch.is_nil()) {
                branch = b;
            } else if (branch != b) {
                branch_mismatch = true;
            }
        });
        if (branch_mismatch) {
            branch = nil_uuid();
        }

        execution_key_t key = get_contract_key(new_pair.second, branch);
        dont_delete.insert(key);
        auto it = executions.find(key);
        if (it != executions.end()) {
            /* Update the existing execution */
            contract_id_t old_contract_id = it->second->contract_id;
            it->second->contract_id = new_pair.first;
            /* Note that `update_contract_or_raft_state()` will never block.
            Also note that we call it even if the contract ID has not actually changed,
            because it might care about the changes to the parts of the Raft state
            other than the contract. */
            it->second->execution->update_contract_or_raft_state(
                new_pair.first, new_state);
            if (old_contract_id != new_pair.first) {
                /* Delete the old contract ack, if there was one */
                ack_map.delete_key(std::make_pair(server_id, old_contract_id));
            }
        } else {
            /* Create a new execution, unless there's already an execution whose region
            overlaps ours. In the latter case, the execution will be deleted soon. */
            bool ok_to_create = true;
            for (const auto &old_pair : executions) {
                if (region_overlaps(old_pair.first.region, new_pair.second.first)) {
                    ok_to_create = false;
                    break;
                }
            }
            if (ok_to_create) {
                executions[key] = make_scoped<execution_data_t>();
                execution_data_t *data = executions[key].get();
                data->contract_id = new_pair.first;

                data->store_subview = make_scoped<store_subview_t>(
                    multistore->get_cpu_sharded_store(get_cpu_shard_number(key.region)),
                    key.region);

                /* We generate perfmon keys of the form "primary-3", "secondary-8", etc.
                The numbers are arbitrary but unique. */
                data->perfmon_membership = make_scoped<perfmon_membership_t>(
                    perfmons, &data->perfmon_collection,
                    strprintf("%s-%d", key.role_name().c_str(), ++perfmon_counter));

                std::function<void(const contract_id_t &, const contract_ack_t &)>
                    acker = std::bind(&contract_executor_t::send_ack, this,
                        key, ph::_1, ph::_2);
                /* Note that these constructors will never block. */
                switch (key.role) {
                case execution_key_t::role_t::primary:
                    data->execution.init(new primary_execution_t(
                        &execution_context, data->store_subview.get(),
                        &data->perfmon_collection, acker,
                        new_pair.first, new_state));
                    break;
                case execution_key_t::role_t::secondary:
                    data->execution.init(new secondary_execution_t(
                        &execution_context, data->store_subview.get(),
                        &data->perfmon_collection, acker,
                        new_pair.first, new_state, branch));
                    break;
                case execution_key_t::role_t::erase:
                    data->execution.init(new erase_execution_t(
                        &execution_context, data->store_subview.get(),
                        &data->perfmon_collection, acker,
                        new_pair.first, new_state));
                    break;
                default: unreachable();
                }
            }
        }
    }
    /* Go through our existing executions and delete the ones that don't correspond to
    any of the new contracts */
    for (const auto &old_pair : executions) {
        if (dont_delete.count(old_pair.first) == 0) {
            to_delete_out->insert(old_pair.first);
        }
    }
}

void contract_executor_t::send_ack(const execution_key_t &key, const contract_id_t &cid,
        const contract_ack_t &ack) {
    assert_thread();
    /* If the contract is out of date, don't send the ack */
    if (executions.at(key)->contract_id == cid) {
        ack_map.set_key_no_equals(std::make_pair(server_id, cid), ack);
    }
}


