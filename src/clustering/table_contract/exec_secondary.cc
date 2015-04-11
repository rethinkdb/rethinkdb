// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/exec_secondary.hpp"

#include <utility>

#include "clustering/immediate_consistency/remote_replicator_client.hpp"
#include "concurrency/cross_thread_signal.hpp"

secondary_execution_t::secondary_execution_t(
        const execution_t::context_t *_context,
        const region_t &_region,
        store_view_t *_store,
        perfmon_collection_t *_perfmon_collection,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &acb) :
    execution_t(_context, _region, _store, _perfmon_collection),
    primary(static_cast<bool>(c.primary) ? c.primary->server : nil_uuid()),
    branch(c.branch), ack_cb(acb)
{
    guarantee(c.replicas.count(context->server_id) == 1);
    coro_t::spawn_sometime(std::bind(&secondary_execution_t::run, this, drainer.lock()));
}

void secondary_execution_t::update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &acb) {
    assert_thread();
    guarantee(primary ==
        (static_cast<bool>(c.primary) ? c.primary->server : nil_uuid()));
    guarantee(branch == c.branch);
    guarantee(c.replicas.count(context->server_id) == 1);
    ack_cb = acb;
    if (static_cast<bool>(last_ack)) {
        ack_cb(*last_ack);
    }
}

void secondary_execution_t::run(auto_drainer_t::lock_t keepalive) {
    assert_thread();
    order_source_t order_source(store->home_thread());
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        try {
            /* Set our initial state to `secondary_need_primary`. */
            contract_ack_t initial_ack(contract_ack_t::state_t::secondary_need_primary);
            {
                cross_thread_signal_t interruptor_on_store_thread(
                    keepalive.get_drain_signal(), store->home_thread());
                on_thread_t thread_switcher(store->home_thread());
                region_map_t<binary_blob_t> blobs;
                read_token_t token;
                store->new_read_token(&token);
                store->do_get_metainfo(
                    order_source.check_in("secondary_execution_t").with_read_mode(),
                    &token, &interruptor_on_store_thread, &blobs);
                initial_ack.version = boost::make_optional(to_version_map(blobs));
            }

            /* Note that in the initial ack, `failover_timeout_elapsed` will be `false`.
            This isn't quite right, because it's possible that hasn't been a primary for
            a while before we got to this point. Consider the following scenario: The
            primary fails. The failover timeout elapses. The coordinator sets the primary
            to nil. All the `secondary_execution_t`s are destroyed and recreated with
            `primary` set to `nil_uuid()`. But now they all report that the failover
            timeout has not elapsed yet, so the failover timeout has to elapse again
            before a new primary can be elected. This is annoying, but it doesn't really
            break anything. */
            initial_ack.failover_timeout_elapsed = false;

            send_ack(initial_ack);

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
            if (!primary.is_nil()) {
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
                    }, true);
            }

            /* Wait until we see a primary or the failover timeout elapses. */
            static const int failover_timeout_ms = 5000;
            signal_timer_t failover_timer;
            failover_timer.start(failover_timeout_ms);
            wait_any_t waiter(primary_bcard.get_ready_signal(), &failover_timer);
            wait_interruptible(&waiter, keepalive.get_drain_signal());

            if (!primary_bcard.is_pulsed()) {
                /* The failover timeout elapsed. Send a new contract ack with
                `failover_timeout_elapsed` set to `true`, then resume waiting for the
                primary. */
                initial_ack.failover_timeout_elapsed = true;
                send_ack(initial_ack);
                wait_interruptible(
                    primary_bcard.get_ready_signal(), keepalive.get_drain_signal());
            }

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
            on_thread_t thread_switcher_1(store->home_thread());

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

            on_thread_t thread_switcher_t(home_thread());

            /* Let the coordinator know we finished backfilling */
            send_ack(contract_ack_t(contract_ack_t::state_t::secondary_streaming));

            /* Wait until we lose contact with the primary or we get interrupted */
            stop_signal.wait_lazily_unordered();

        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    }
}

void secondary_execution_t::send_ack(const contract_ack_t &ca) {
    assert_thread();
    ack_cb(ca);
    last_ack = boost::make_optional(ca);
}

