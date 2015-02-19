// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/follower.hpp"

#include "store_subview.hpp"

namespace table_raft {

follower_t::follower_t(
        const server_id_t &_server_id,
        mailbox_manager_t *const _mailbox_manager,
        raft_member_t<state_t> *_raft,
        watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t>
            *_remote_primary_bcards,
        const multistore_ptr_t *_multistore,
        branch_history_manager_t *_branch_history_manager,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        backfill_throttler_t *_backfill_throttler,
        perfmon_collection_t *_perfmons) :
    server_id(_server_id),
    mailbox_manager(_mailbox_manager),
    raft(_raft),
    remote_primary_bcards(_remote_primary_bcards),
    multistore(_multistore),
    branch_history_manager(_branch_history_manager),
    base_path(_base_path),
    io_backender(_io_backender),
    backfill_throttler(_backfill_throttler),
    perfmons(_perfmons),
    update_coro_running(false),
    perfmon_counter(0),
    raft_state_subs(std::bind(&follower_t::on_raft_state_change, this))
{
    watchable_t<raft_member_t<state_t>::state_and_config_t>::freeze_t freeze(
        raft->get_committed_state());
    raft_state_subs.reset(raft->get_committed_state(), &freeze);
}

follower_t::ongoing_key_t follower_t::get_contract_key(
        const std::pair<region_t, contract_t> &pair) {
    ongoing_key_t key;
    key.region = pair.first;
    if (static_cast<bool>(pair.second.primary) &&
            pair.second.primary->server == server_id) {
        key.role = ongoing_key_t::role_t::primary;
        key.primary = nil_uuid();
        key.branch = nil_uuid();
    } else if (pair.second.replicas.count(server_id) == 1) {
        key.role = ongoing_key_t::role_t::secondary;
        if (static_cast<bool>(pair.second.primary)) {
            key.primary = pair.second.primary->server;
        } else {
            key.primary = nil_uuid();
        }
        key.branch = pair.second.branch;
    } else {
        key.role = ongoing_key_t::role_t::erase;
        key.primary = nil_uuid();
        key.branch = nil_uuid();
    }
    return key;
}

void follower_t::on_raft_state_change() {
    if (!update_coro_running) {
        update_coro_running = true;
        coro_t::spawn_sometime(std::bind(
            &follower_t::update_coro, this, drainer.lock()));
    }
}

void follower_t::update_coro(auto_drainer_t::lock_t keepalive) {
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        std::set<ongoing_key_t> to_delete;
        {
            ASSERT_NO_CORO_WAITING;
            raft->get_committed_state()->apply_read(
            [&](const raft_member_t<state_t>::state_and_config_t *cs) {
                update(cs->state, &to_delete);
            });
            if (to_delete.empty()) {
                /* There's no point in going around the loop again. Since we won't delete
                anything, we won't block, so `raft->get_committed_state()` won't have a
                chance to change; so any further calls to `update()` will be no-ops. When
                `raft->get_committed_state()` changes again, `on_raft_state_change()`
                will start another instance of `update_coro()`. */
                update_coro_running = false;
                return;
            }
        }
        /* This is the part that can block */
        for (const ongoing_key_t &key : to_delete) {
            ongoings.erase(key);
        }
    }
}

void follower_t::update(const state_t &new_state,
                        std::set<ongoing_key_t> *to_delete_out) {
    /* Go through the new contracts and try to match them to existing ongoings */
    std::set<ongoing_key_t> dont_delete;
    for (const auto &new_pair : new_state.contracts) {
        ongoing_key_t key = get_contract_key(new_pair.second);
        dont_delete.insert(key);
        auto it = ongoings.find(key);
        if (it != ongoings.end()) {
            /* Update the existing ongoing */
            if (it->second->contract_id != new_pair.first) {
                it->second->contract_id = new_pair.first;
                std::function<void(const contract_ack_t &)> acker =
                    std::bind(&follower_t::send_ack, this, new_pair.first, ph::_1);
                /* Note that `update_contract()` will never block. */
                switch (key.role) {
                case ongoing_key_t::role_t::primary:
                    it->second->primary->update_contract(
                        new_pair.second.second, acker);
                    break;
                case ongoing_key_t::role_t::secondary:
                    it->second->secondary->update_contract(
                        new_pair.second.second, acker);
                    break;
                case ongoing_key_t::role_t::erase:
                    it->second->erase->update_contract(
                        new_pair.second.second, acker);
                    break;
                default: unreachable();
                }
            }
        } else {
            /* Create a new ongoing, unless there's already an ongoing whose region
            overlaps ours. In the latter case, the ongoing will be deleted soon. */
            bool ok_to_create = true;
            for (const auto &old_pair : ongoings) {
                if (region_overlaps(old_pair.first.region, new_pair.second.first)) {
                    ok_to_create = false;
                    break;
                }
            }
            if (ok_to_create) {
                ongoings[key] = make_scoped<ongoing_data_t>();
                ongoing_data_t *data = ongoings[key].get();
                data->contract_id = new_pair.first;

                data->store_subview = make_scoped<store_subview_t>(
                    multistore->shards[get_cpu_shard_number(key.region)],
                    key.region);

                /* We generate perfmon keys of the form "primary-3", "secondary-8", etc.
                The numbers are arbitrary but unique. */
                data->perfmon_membership = make_scoped<perfmon_membership_t>(
                    perfmons, &data->perfmon_collection,
                    strprintf("%s-%d", key.role_name().c_str(), ++perfmon_counter));

                std::function<void(const contract_ack_t &)> acker =
                    std::bind(&follower_t::send_ack, this, new_pair.first, ph::_1);
                /* Note that these constructors will never block. */
                switch (key.role) {
                case ongoing_key_t::role_t::primary:
                    data->primary.init(new primary_t(
                        server_id, mailbox_manager, data->store_subview.get(),
                        branch_history_manager, key.region, &data->perfmon_collection,
                        new_pair.second.second, acker, &local_primary_bcards,
                        &replica_bcards, base_path, io_backender));
                    break;
                case ongoing_key_t::role_t::secondary:
                    data->secondary.init(new secondary_t(
                        server_id, mailbox_manager, data->store_subview.get(),
                        branch_history_manager, key.region, &data->perfmon_collection,
                        new_pair.second.second, acker, remote_primary_bcards, base_path,
                        io_backender, backfill_throttler));
                    break;
                case ongoing_key_t::role_t::erase:
                    data->erase.init(new erase_t(
                        server_id, data->store_subview.get(), branch_history_manager,
                        key.region, &data->perfmon_collection,
                        new_pair.second.second, acker));
                    break;
                default: unreachable();
                }
            }
        }
    }
    /* Go through our existing ongoings and delete the ones that don't correspond to any
    of the new contracts */
    for (const auto &old_pair : ongoings) {
        if (dont_delete.count(old_pair.first) == 0) {
            ack_map.delete_key(std::make_pair(server_id, old_pair.second->contract_id));
            to_delete_out->insert(old_pair.first);
        }
    }
}

void follower_t::send_ack(const contract_id_t &cid, const contract_ack_t &ack) {
    ack_map.set_key_no_equals(std::make_pair(server_id, cid), ack);
}

} /* namespace table_raft */

