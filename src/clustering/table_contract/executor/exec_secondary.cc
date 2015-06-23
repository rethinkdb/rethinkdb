// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/executor/exec_secondary.hpp"

#include <utility>

#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "clustering/query_routing/direct_query_server.hpp"
#include "concurrency/cross_thread_signal.hpp"

secondary_execution_t::secondary_execution_t(
        const execution_t::context_t *_context,
        execution_t::params_t *_params,
        const contract_id_t &cid,
        const table_raft_state_t &raft_state,
        const branch_id_t &_branch) :
    execution_t(_context, _params)
{
    const contract_t &c = raft_state.contracts.at(cid).second;
    guarantee(c.replicas.count(context->server_id) == 1);
    guarantee(raft_state.contracts.at(cid).first == region);

    /* If the current contract doesn't have a primary, we won't attempt to connect to the
    primary. Also, if we don't have a branch ID yet or the current contract's region
    doesn't match the branch's region, we won't attempt to connect. This is because
    the only way that can happen is if the table was just resharded, so the primary
    is about to stop and restart with a different branch ID, and there's no point in
    trying to connect to the existing primary. In fact, we can't connect to the
    existing primary, since it's impossible to subscribe to a primary with a different
    region. */
    if (static_cast<bool>(c.primary) && !_branch.is_nil() &&
            raft_state.branch_history.branches.at(_branch).get_region() == region) {
        connect_to_primary = true;
        primary = c.primary->server;
    } else {
        connect_to_primary = false;
        primary = nil_uuid();
    }

    branch = _branch;
    contract_id = cid;
    coro_t::spawn_sometime(std::bind(&secondary_execution_t::run, this, drainer.lock()));
}

void secondary_execution_t::update_contract_or_raft_state(
        const contract_id_t &cid,
        const table_raft_state_t &raft_state) {
    assert_thread();
    const contract_t &c = raft_state.contracts.at(cid).second;
    guarantee(raft_state.contracts.at(cid).first == region);
    guarantee(c.replicas.count(context->server_id) == 1);
    guarantee(primary.is_nil() || primary == c.primary->server);
    if (contract_id != cid && static_cast<bool>(last_ack)) {
        params->send_ack(cid, *last_ack);
    }
    contract_id = cid;
}

void secondary_execution_t::run(auto_drainer_t::lock_t keepalive) {
    assert_thread();
    order_source_t order_source(store->home_thread());
    bool enabled_gc = false;
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        try {
            /* Switch to the store thread so we can extract our metainfo and set up the
            `direct_query_server_t` to serve queries while we wait for the primary to
            be available */
            cross_thread_signal_t interruptor_on_store_thread(
                keepalive.get_drain_signal(), store->home_thread());
            on_thread_t thread_switcher_1(store->home_thread());

            contract_ack_t initial_ack(contract_ack_t::state_t::secondary_need_primary);
            read_token_t token;
            store->new_read_token(&token);
            initial_ack.version = boost::make_optional(to_version_map(
                store->get_metainfo(
                    order_source.check_in("secondary_execution_t").with_read_mode(),
                    &token, region, &interruptor_on_store_thread)));

            direct_query_server_t direct_query_server(context->mailbox_manager, store);

            /* Switch back to the home thread so we can send the initial ack */
            on_thread_t thread_switcher_2(home_thread());

            context->branch_history_manager->export_branch_history(
                *initial_ack.version, &initial_ack.branch_history);

            send_ack(initial_ack);

            /* Serve outdated reads while we wait for the primary */
            object_buffer_t<watchable_map_var_t<uuid_u, table_query_bcard_t>::entry_t>
                directory_entry;
            {
                table_query_bcard_t tq_bcard;
                tq_bcard.region = region;
                tq_bcard.direct = boost::make_optional(direct_query_server.get_bcard());
                directory_entry.create(
                    context->local_table_query_bcards, generate_uuid(), tq_bcard);
            }

            if (!connect_to_primary) {
                /* Instead of establishing a connection to the primary and doing a
                backfill, we'll just wait for a new contract. */
                keepalive.get_drain_signal()->wait_lazily_unordered();
                return;
            }

            /* Set up a subscription to look for the primary in the directory and also
            detect if we lose contact. Initially, `primary_bcard` and
            `primary_no_more_bcard` are both unpulsed, and `primary_disconnect_watcher`
            is empty. When we first see the primary, we pulse `primary_bcard` and create
            `primary_disconnected`. If the primary later disappears from the directory,
            we pulse `primary_no_more_bcard`. */
            promise_t<contract_execution_bcard_t> primary_bcard;
            object_buffer_t<disconnect_watcher_t> primary_disconnected;
            cond_t primary_no_more_bcard;

            object_buffer_t<watchable_map_t<
                std::pair<server_id_t, branch_id_t>,
                contract_execution_bcard_t>::key_subs_t>
                    primary_watcher;
            primary_watcher.create(
                context->remote_contract_execution_bcards,
                std::make_pair(primary, branch),
                [&](const contract_execution_bcard_t *bc) {
                    if (!primary_bcard.get_ready_signal()->is_pulsed()) {
                        if (bc != nullptr) {
                            primary_bcard.pulse(*bc);
                            primary_disconnected.create(
                                context->mailbox_manager, bc->peer);
                        }
                    } else if (!primary_no_more_bcard.is_pulsed()) {
                        if (bc == nullptr) {
                            primary_no_more_bcard.pulse();
                        }
                    }
                }, initial_call_t::YES);

            /* Wait until we see a primary */
            wait_interruptible(
                primary_bcard.get_ready_signal(), keepalive.get_drain_signal());

            /* Stop serving outdated reads, because we're going to do a backfill */
            directory_entry.reset();

            /* Let the coordinator know we found the primary. */
            send_ack(contract_ack_t(contract_ack_t::state_t::secondary_backfilling));

            /* Set up a signal that will get pulsed if we lose contact with the primary
            or we get interrupted */
            wait_any_t stop_signal(
                &primary_no_more_bcard,
                primary_disconnected.get(),
                keepalive.get_drain_signal());

            /* We have to construct and destroy the `listener_t` and the `replier_t` on
            the store's home thread. So we switcher there and switch back, and when the
            stack variables get destructed we do the reverse. */
            cross_thread_signal_t stop_signal_on_store_thread(
                &stop_signal, store->home_thread());
            on_thread_t thread_switcher_3(store->home_thread());

            /* Backfill and start streaming from the primary. */
            remote_replicator_client_t remote_replicator_client(
                context->backfill_throttler,
                backfill_config_t(),
                context->mailbox_manager,
                context->server_id,
                branch,
                primary_bcard.assert_get_value().remote_replicator_server,
                primary_bcard.assert_get_value().replica,
                store,
                context->branch_history_manager,
                &stop_signal_on_store_thread);

            on_thread_t thread_switcher_4(home_thread());

            /* Now that we've backfilled, it's safe to call `enable_gc()`. */
            if (!enabled_gc) {
                enabled_gc = true;
                params->enable_gc(branch);
            }

            /* Let the coordinator know we finished backfilling */
            send_ack(contract_ack_t(contract_ack_t::state_t::secondary_streaming));

            /* Resume serving outdated reads now that the backfill is over */
            {
                table_query_bcard_t tq_bcard;
                tq_bcard.region = region;
                tq_bcard.direct = boost::make_optional(direct_query_server.get_bcard());
                directory_entry.create(
                    context->local_table_query_bcards, generate_uuid(), tq_bcard);
            }

            /* Wait until we lose contact with the primary or we get interrupted */
            stop_signal.wait_lazily_unordered();

        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    }
}

void secondary_execution_t::send_ack(const contract_ack_t &ca) {
    assert_thread();
    params->send_ack(contract_id, ca);
    last_ack = boost::make_optional(ca);
}

