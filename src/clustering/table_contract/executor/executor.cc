// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/executor/executor.hpp"

#include "clustering/table_contract/branch_history_gc.hpp"
#include "clustering/table_contract/executor/exec_erase.hpp"
#include "clustering/table_contract/executor/exec_primary.hpp"
#include "clustering/table_contract/executor/exec_secondary.hpp"
#include "store_subview.hpp"

class contract_executor_t::execution_wrapper_t : private execution_t::params_t {
public:
    execution_wrapper_t(
            contract_executor_t *_parent,
            const execution_key_t &key,
            const branch_id_t &branch,
            const contract_id_t &_contract_id,
            const table_raft_state_t &state) :
        parent(_parent), contract_id(_contract_id),
        store_subview(
            parent->multistore->get_cpu_sharded_store(
                get_cpu_shard_number(key.region)),
            key.region),
        perfmon_membership(
            parent->perfmons,
            &perfmon_collection,
            strprintf("%s-%d", key.role_name().c_str(), ++parent->perfmon_counter))
    {
        switch (key.role) {
        case execution_key_t::role_t::primary:
            execution.init(new primary_execution_t(
                &parent->execution_context, this, contract_id, state));
            break;
        case execution_key_t::role_t::secondary:
            execution.init(new secondary_execution_t(
                &parent->execution_context, this, contract_id, state, branch));
            break;
        case execution_key_t::role_t::erase:
            execution.init(new erase_execution_t(
                &parent->execution_context, this, contract_id, state));
            break;
        default: unreachable();
        }
    }
    ~execution_wrapper_t() {
        /* Stop the execution first, so it can't call `send_ack()` after we've deleted
        the entry from `ack_map` */
        execution.reset();
        parent->ack_map.delete_key(std::make_pair(parent->server_id, contract_id));
    }

    void update(const contract_id_t &new_contract_id,
                const table_raft_state_t &new_state) {
        contract_id_t old_contract_id = contract_id;
        contract_id = new_contract_id;
        /* Note that `update_contract_or_raft_state()` will never block. Also note that
        we call it even if the contract ID has not actually changed, because it might
        care about the changes to the parts of the Raft state other than the contract. */
        execution->update_contract_or_raft_state(new_contract_id, new_state);
        if (old_contract_id != new_contract_id) {
            /* Delete the old contract ack, if there was one */
            parent->ack_map.delete_key(
                std::make_pair(parent->server_id, old_contract_id));
        }
    }

    contract_id_t get_contract_id() const {
        return contract_id;
    }

    /* If the execution has called its `enable_gc()` callback, this will contain the
    value that was provided. Otherwise, this will be empty. */
    boost::optional<branch_id_t> enable_gc_branch;

private:
    perfmon_collection_t *get_perfmon_collection() {
        return &perfmon_collection;
    }

    store_view_t *get_store() {
        return &store_subview;
    }

    void send_ack(const contract_id_t &cid, const contract_ack_t &ack) {
        parent->assert_thread();
        /* If the contract is out of date, don't send the ack */
        if (cid == contract_id) {
            parent->ack_map.set_key_no_equals(
                std::make_pair(parent->server_id, contract_id), ack);
        }
    }

    void enable_gc(const branch_id_t &new_enable_gc_branch) {
        parent->assert_thread();
        guarantee(!static_cast<bool>(enable_gc_branch));
        enable_gc_branch = boost::make_optional(new_enable_gc_branch);
        parent->gc_branch_history_pumper.notify();
    }

    contract_executor_t *const parent;

    /* The contract ID of the contract governing this execution. Note that this may
    change over the course of an execution; see the comment about `execution_key_t`. */
    contract_id_t contract_id;

    /* A `store_subview_t` containing only the sub-region of the store that this
    execution affects. */
    store_subview_t store_subview;

    /* We create a new perfmon category for each execution. This way the executions
    themselves don't have to think about perfmon key collisions. */
    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_membership;

    /* The execution itself */
    scoped_ptr_t<execution_t> execution;
};

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
        perfmon_collection_t *_perfmons) :
    server_id(_server_id),
    raft_state(_raft_state),
    multistore(_multistore),
    perfmons(_perfmons),
    perfmon_counter(0),
    update_pumper(
        std::bind(&contract_executor_t::update_blocking, this, ph::_1)),
    gc_branch_history_pumper(
        std::bind(&contract_executor_t::gc_branch_history, this, ph::_1)),
    raft_state_subs([this]() {
        update_pumper.notify();
        gc_branch_history_pumper.notify();
    })
{
    execution_context.server_id = _server_id;
    execution_context.mailbox_manager = _mailbox_manager;
    execution_context.branch_history_manager = multistore->get_branch_history_manager();
    execution_context.base_path = _base_path;
    execution_context.io_backender = _io_backender;
    execution_context.backfill_throttler = _backfill_throttler;
    execution_context.remote_contract_execution_bcards
        = _remote_contract_execution_bcards;
    execution_context.local_contract_execution_bcards
        = &local_contract_execution_bcards;
    execution_context.local_table_query_bcards = &local_table_query_bcards;

    multistore->assert_thread();

    watchable_t<table_raft_state_t>::freeze_t freeze(raft_state);
    raft_state_subs.reset(raft_state, &freeze);
    update_pumper.notify();
}

contract_executor_t::~contract_executor_t() {
    /* We have to destroy `raft_state_subs` before the `pump_coro_t`s because its
    callback accesses them. */
    raft_state_subs.reset();

    /* We have to drain `update_pumper` and `gc_branch_history_pumper` before
    `executions` because their callbacks access `executions`. */
    update_pumper.drain();
    gc_branch_history_pumper.drain();

    /* We have to clean out `executions` before destroying `gc_branch_history_pumper`
    because the `execution_wrapper_t`s may access `gc_branch_history_pumper`. */
    executions.clear();
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

void contract_executor_t::update_blocking(signal_t *interruptor) {
    new_mutex_acq_t executions_mutex_acq(&executions_mutex, interruptor);
    std::set<execution_key_t> dont_delete;
    {
        ASSERT_NO_CORO_WAITING;
        raft_state->apply_read([&](const table_raft_state_t *new_state) {
            for (const auto &new_pair : new_state->contracts) {
                /* Extract the current branch ID for the region covered by this contract.
                If there are multiple branches for different subregions, we consider the
                branch as incoherent and don't set a branch ID. */
                branch_id_t branch = nil_uuid();
                bool branch_mismatch = false;
                new_state->current_branches.visit(new_pair.second.first,
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
                    it->second->update(new_pair.first, *new_state);
                } else {
                    /* Create a new execution, unless there's already an execution whose
                    region overlaps ours. In the latter case, the execution will be
                    deleted soon. */
                    bool ok_to_create = true;
                    for (const auto &old_pair : executions) {
                        if (region_overlaps(old_pair.first.region,
                                            new_pair.second.first)) {
                            ok_to_create = false;
                            break;
                        }
                    }
                    if (ok_to_create) {
                        executions[key] = make_scoped<execution_wrapper_t>(
                            this, key, branch, new_pair.first, *new_state);
                    }
                }
            }
        });
    }
    bool deleted_any = false;
    for (auto it = executions.begin(); it != executions.end();) {
        if (dont_delete.count(it->first) == 1) {
            ++it;
        } else {
            /* This is the part that can block. */
            executions.erase(it++);
            deleted_any = true;
        }
    }
    if (deleted_any) {
        /* Now that we've deleted the executions, `update()` is likely to have new
        instructions for us, so we should run again. */
        update_pumper.notify();
    }
}

void contract_executor_t::gc_branch_history(signal_t *interruptor) {
    new_mutex_acq_t executions_mutex_acq(&executions_mutex, interruptor);
    std::set<branch_id_t> remove_branches;
    execution_context.branch_history_manager->prepare_gc(&remove_branches);
    bool ok = true;
    raft_state->apply_read([&](const table_raft_state_t *state) {
        std::set<contract_id_t> contract_ids;
        for (const auto &pair : state->contracts) {
            contract_ids.insert(pair.first);
        }
        for (const auto &pair : executions) {
            contract_ids.erase(pair.second->get_contract_id());
            if (!static_cast<bool>(pair.second->enable_gc_branch)) {
                ok = false;
                break;
            }
            if (!pair.second->enable_gc_branch->is_nil()) {
                mark_ancestors_since_base_live(
                    *pair.second->enable_gc_branch,
                    pair.first.region,
                    execution_context.branch_history_manager,
                    &state->branch_history,
                    &remove_branches);
            }
        }
        if (!contract_ids.empty()) {
            /* Since we were missing executions for one more more contracts, don't
            garbage collect. This is to make sure that if we're in a transitional state
            where some regions have no execution, we won't throw away the branches for
            those regions. */
            ok = false;
        }
    });
    if (ok) {
        execution_context.branch_history_manager->perform_gc(
            remove_branches, interruptor);
    }
}

