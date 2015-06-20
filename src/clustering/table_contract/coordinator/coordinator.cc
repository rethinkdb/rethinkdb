// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/coordinator/coordinator.hpp"

#include "clustering/generic/raft_core.tcc"
#include "clustering/table_contract/coordinator/calculate_contracts.hpp"
#include "clustering/table_contract/coordinator/calculate_misc.hpp"
#include "clustering/table_contract/coordinator/check_ready.hpp"
#include "logger.hpp"

contract_coordinator_t::contract_coordinator_t(
        raft_member_t<table_raft_state_t> *_raft,
        watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t> *_acks,
        watchable_map_t<std::pair<server_id_t, server_id_t>, empty_value_t>
            *_connections_map) :
    raft(_raft),
    acks(_acks),
    connections_map(_connections_map),
    config_pumper(std::bind(&contract_coordinator_t::pump_configs, this, ph::_1)),
    contract_pumper(std::bind(&contract_coordinator_t::pump_contracts, this, ph::_1)),
    ack_subs(acks,
             std::bind(&contract_coordinator_t::on_ack_change, this, ph::_1, ph::_2),
             initial_call_t::YES),
    connections_map_subs(connections_map,
        std::bind(&pump_coro_t::notify, &contract_pumper), initial_call_t::NO)
{
    raft->assert_thread();
    /* Do an initial round of pumping, in case there are any changes the previous
    coordinator didn't take care of */
    contract_pumper.notify();
    config_pumper.notify();
}

boost::optional<raft_log_index_t> contract_coordinator_t::change_config(
        const std::function<void(table_config_and_shards_t *)> &changer,
        signal_t *interruptor) {
    assert_thread();
    scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t> change_token;
    raft_log_index_t log_index;
    {
        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);
        table_raft_state_t::change_t::set_table_config_t change;
        bool is_noop;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
            change.new_config = state->state.config;
            changer(&change.new_config);
            log_index = state->log_index;
            is_noop = (change.new_config == state->state.config);
        });
        if (is_noop) {
            return boost::make_optional(log_index);
        }
        change_token = raft->propose_change(
            &change_lock, table_raft_state_t::change_t(change));
        log_index += 1;
    }
    if (!change_token.has()) {
        return boost::none;
    }
    contract_pumper.notify();
    config_pumper.notify();
    wait_interruptible(change_token->get_ready_signal(), interruptor);
    if (!change_token->wait()) {
        return boost::none;
    }
    return boost::make_optional(log_index);
}

bool contract_coordinator_t::check_all_replicas_ready(signal_t *interruptor) {
    assert_thread();
    /* The bulk of the work is in the `::check_all_replicas_ready()` function defined in
    `check_ready.hpp`. But we also make sure to commit the state that `check_ready` saw
    before we reply, to ensure that we are actually still the Raft leader. */
    scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t> change_token;
    {
        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);
        bool ok;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
            ok = ::check_all_replicas_ready(state->state, acks);
        });
        if (!ok) {
            return false;
        }
        change_token = raft->propose_noop(&change_lock);
    }
    if (!change_token.has()) {
        return false;
    }
    wait_interruptible(change_token->get_ready_signal(), interruptor);
    if (!change_token->wait()) {
        return false;
    }
    return true;
}

void contract_coordinator_t::on_ack_change(
        const std::pair<server_id_t, contract_id_t> &key,
        const contract_ack_t *ack) {
    if (ack != nullptr) {
        acks_by_contract[key.second][key.first] = *ack;
    } else {
        auto it = acks_by_contract.find(key.second);
        if (it != acks_by_contract.end()) {
            auto jt = it->second.find(key.first);
            if (jt != it->second.end()) {
                it->second.erase(jt);
                if (it->second.empty()) {
                    acks_by_contract.erase(it);
                }
            }
        }
    }

    contract_pumper.notify();
}

void contract_coordinator_t::pump_contracts(signal_t *interruptor) {
    assert_thread();

    /* Wait a little while to give changes time to accumulate, because
    `calculate_all_contracts()` needs to examine every shard of the table even if
    nothing about them has changed, and that might be expensive. */
    nap(200, interruptor);

    /* Now we'll apply changes to Raft. We keep trying in a loop in case it
    doesn't work at first. */
    while (true) {
        /* Wait until the Raft member is likely to accept changes */
        raft->get_readiness_for_change()->run_until_satisfied(
            [](bool is_ready) { return is_ready; }, interruptor);

        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);

        /* Call `include_latest_notifications()` right before we read the Raft state.
        This is because any changes to the Raft state that happened up to this point will
        be included in this round of pumping, but any changes after this point must be
        run in their own round. */
        contract_pumper.include_latest_notifications();

        /* Calculate the proposed change */
        table_raft_state_t::change_t::new_contracts_t change;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *state) {
            calculate_all_contracts(
                state->state, acks_by_contract, connections_map,
                &change.remove_contracts, &change.add_contracts,
                &change.register_current_branches);
            calculate_branch_history(
                state->state, acks_by_contract,
                change.remove_contracts, change.add_contracts,
                change.register_current_branches,
                &change.remove_branches, &change.add_branches);
            calculate_server_names(
                state->state, change.remove_contracts, change.add_contracts,
                &change.remove_server_names, &change.add_server_names);
        });

        /* Apply the change, unless it's a no-op */
        bool change_ok;
        if (!change.remove_contracts.empty() ||
                !change.add_contracts.empty() ||
                !change.remove_branches.empty() ||
                !change.add_branches.branches.empty() ||
                !change.register_current_branches.empty()) {
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_change(
                    &change_lock, table_raft_state_t::change_t(change));

            /* `pump_configs()` sometimes makes changes in reaction to changes in
            contracts, so wake it up. */
            config_pumper.notify();

            change_ok = change_token.has();

            /* Note that we don't wait on the change token. We know we won't
            start a redundant change because we always compute the change by
            comparing to `get_latest_state()`. */
        } else {
            change_ok = true;
        }

        /* If the change failed, go back to the top of the loop and wait on
        `get_readiness_for_change()` again. */
        if (change_ok) {
            break;
        }
    }
}

void contract_coordinator_t::pump_configs(signal_t *interruptor) {
    assert_thread();

    nap(200, interruptor);

    while (true) {
        /* Wait until the Raft member is likely to accept config changes. This
        isn't actually necessary for changes to `member_ids`, but it's easier to
        just handle `member_ids` changes and Raft configuration changes at the
        same time. */
        raft->get_readiness_for_config_change()->run_until_satisfied(
            [](bool is_ready) { return is_ready; },
            interruptor);

        raft_member_t<table_raft_state_t>::change_lock_t change_lock(raft, interruptor);

        /* As in `pump_contracts()`, call `include_latest_notifications()` right before
        we read the Raft state */
        config_pumper.include_latest_notifications();

        /* Calculate changes to `table_raft_state_t::member_ids` */
        table_raft_state_t::change_t::new_member_ids_t member_ids_change;
        boost::optional<raft_config_t> config_change;
        raft->get_latest_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t *sc) {
            raft_config_t new_config;
            calculate_member_ids_and_raft_config(
                *sc,
                &member_ids_change.remove_member_ids,
                &member_ids_change.add_member_ids,
                &new_config);
            /* The config almost always won't be a joint consensus, because we
            waited until the `raft_member_t` was ready for config changes. But
            it's still possible so we have to handle that case somehow. Setting
            `config_change` won't change the config in a joint consensus because
            `propose_config_change()` will fail, but at least it will force us to
            go around the loop again. */
            if ((!sc->config.is_joint_consensus() &&
                        new_config != sc->config.config) ||
                    (sc->config.is_joint_consensus() &&
                        new_config != *sc->config.new_config)) {
                config_change = boost::make_optional(new_config);
            }
        });

        bool member_ids_ok;
        if (!member_ids_change.remove_member_ids.empty() ||
                !member_ids_change.add_member_ids.empty()) {
            /* Apply the `member_ids` change */
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_change(
                    &change_lock,
                    table_raft_state_t::change_t(member_ids_change));
            member_ids_ok = change_token.has();
        } else {
            /* The change is a no-op, so don't bother applying it */
            member_ids_ok = true;
        }

        bool config_ok;
        if (!member_ids_ok) {
            /* If the `member_ids` change didn't go through, don't attempt the
            config change. The config change would be unlikely to succeed, but
            more importantly, the config change isn't necessarily valid to apply
            unless it comes after the `member_ids` change. */
            config_ok = false;
        } else if (static_cast<bool>(config_change)) {
            /* Apply the config change */
            scoped_ptr_t<raft_member_t<table_raft_state_t>::change_token_t>
                change_token = raft->propose_config_change(&change_lock, *config_change);
            config_ok = change_token.has();
        } else {
            /* The config change is a no-op */
            config_ok = true;
        }

        /* If both changes succeeded, break out of the loop. Otherwise, go back
        to the top of the loop and wait for `get_readiness_for_config_change()`
        again. */
        if (member_ids_ok && config_ok) {
            break;
        }
    }
}

