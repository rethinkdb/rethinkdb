// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/exec_secondary.hpp"

#include <utility>

#include "clustering/immediate_consistency/listener.hpp"
#include "clustering/immediate_consistency/replier.hpp"
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
    order_source_t order_source;
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        try {
            /* Set our initial state to `secondary_need_primary`. */
            contract_ack_t initial_ack;
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
                initial_ack.state = contract_ack_t::state_t::secondary_need_primary;
                initial_ack.version = boost::make_optional(to_version_map(blobs));
            }
            send_ack(initial_ack);

            /* If the contract has no primary, we stop here. When a new contract is
            issued which has a primary, the `contract_executor_t` will destroy us and
            construct a new `secondary_execution_t`. */
            if (primary.is_nil()) {
                keepalive.get_drain_signal()->wait_lazily_unordered();
                return;
            }

            /* Wait until we see the primary. */
            contract_execution_bcard_t primary_bcard;
            context->remote_contract_execution_bcards->run_key_until_satisfied(
                std::make_pair(primary, branch),
                [&](const contract_execution_bcard_t *bc) {
                    if (bc != nullptr) {
                        primary_bcard = *bc;
                        return true;
                    } else {
                        return false;
                    }
                }, keepalive.get_drain_signal());
            guarantee(primary_bcard.broadcaster.branch_id == branch);

            /* Let the coordinator know we found the primary. */
            send_ack(contract_ack_t(contract_ack_t::state_t::secondary_backfilling));

            /* Set up a signal that will get pulsed if we lose contact with the primary
            or we get interrupted */
            cond_t no_more_bcard_signal;
            watchable_map_t<std::pair<server_id_t, branch_id_t>,
                            contract_execution_bcard_t>::key_subs_t subs(
                context->remote_contract_execution_bcards,
                std::make_pair(primary, branch),
                [&](const contract_execution_bcard_t *bc) {
                    if (bc == nullptr) {
                        no_more_bcard_signal.pulse();
                    }
                },
                true);
            disconnect_watcher_t disconnect_signal(
                context->mailbox_manager, primary_bcard.peer);
            wait_any_t stop_signal(
                &no_more_bcard_signal,
                &disconnect_signal,
                keepalive.get_drain_signal());

            /* We have to construct and destroy the `listener_t` and the `replier_t` on
            the store's home thread. So we switcher there and switch back, and when the
            stack variables get destructed we do the reverse. */
            cross_thread_signal_t stop_signal_on_store_thread(
                &stop_signal, store->home_thread());
            on_thread_t thread_switcher_1(store->home_thread());

            /* Backfill and start streaming from the primary. */
            listener_t listener(
                context->base_path,
                context->io_backender,
                context->mailbox_manager,
                context->server_id,
                context->backfill_throttler,
                primary_bcard.broadcaster,
                context->branch_history_manager,
                store,
                primary_bcard.replier,
                perfmon_collection,
                &stop_signal_on_store_thread,
                &order_source,
                nullptr); /* RSI(raft): Hook up backfill progress again */

            replier_t replier(
                &listener,
                context->mailbox_manager,
                context->branch_history_manager);

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

