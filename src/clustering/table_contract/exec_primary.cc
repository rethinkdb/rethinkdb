// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/exec_primary.hpp"

#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "store_view.hpp"

primary_execution_t::primary_execution_t(
        const execution_t::context_t *_context,
        const region_t &_region,
        store_view_t *_store,
        perfmon_collection_t *_perfmon_collection,
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &acb) :
    execution_t(_context, _region, _store, _perfmon_collection),
    latest_contract_home_thread(make_counted<contract_info_t>(c, acb)),
    latest_contract_store_thread(latest_contract_home_thread),
    our_primary(nullptr)
{
    guarantee(static_cast<bool>(c.primary));
    guarantee(c.primary->server == context->server_id);
    coro_t::spawn_sometime(std::bind(&primary_execution_t::run, this, drainer.lock()));
}

void primary_execution_t::update_contract(
        const contract_t &c,
        const std::function<void(const contract_ack_t &)> &acb) {
    assert_thread();
    ASSERT_NO_CORO_WAITING;

    guarantee(static_cast<bool>(c.primary));
    guarantee(c.primary->server == context->server_id);

    /* Mark the old contract as obsolete, and record the new one */
    latest_contract_home_thread->obsolete.pulse();
    latest_contract_home_thread = make_counted<contract_info_t>(c, acb);

    /* Has our branch ID been registered yet? */
    if (!branch_registered.is_pulsed() &&
            boost::make_optional(c.branch) == our_branch_id) {
        /* This contract just issued us our initial branch ID */
        branch_registered.pulse();
        /* Change `latest_ack` immediately so we don't keep sending the branch
        registration request */
        latest_ack = boost::make_optional(contract_ack_t(
            contract_ack_t::state_t::primary_in_progress));
    }

    /* If we were acking `primary_ready`, go back to acking `primary_in_progress` until
    we sync with the replicas according to the new contract. */
    if (static_cast<bool>(latest_ack) &&
            latest_ack->state == contract_ack_t::state_t::primary_ready) {
        latest_ack = boost::make_optional(contract_ack_t(
            contract_ack_t::state_t::primary_in_progress));
    }

    /* Deliver the new contract to the other thread. If the broadcaster exists, we'll
    also do sync with the replicas and ack `primary_in_ready`. */
    coro_t::spawn_sometime(std::bind(
        &primary_execution_t::update_contract_on_store_thread, this,
        latest_contract_home_thread,
        drainer.lock(),
        new new_mutex_in_line_t(&mutex)));

    if (static_cast<bool>(latest_ack)) {
        latest_contract_home_thread->ack_cb(*latest_ack);
    }
}

void primary_execution_t::run(auto_drainer_t::lock_t keepalive) {
    assert_thread();
    order_source_t order_source(store->home_thread());
    cross_thread_signal_t interruptor_store_thread(
        keepalive.get_drain_signal(), store->home_thread());

    try {
        /* Create the `primary_dispatcher_t`, establishing our branch in the process. */
        on_thread_t thread_switcher_1(store->home_thread());

        region_map_t<binary_blob_t> blobs;
        read_token_t token;
        store->new_read_token(&token);
        store->do_get_metainfo(
            order_source.check_in("primary_t").with_read_mode(), &token,
            &interruptor_store_thread, &blobs);

        primary_dispatcher_t primary_dispatcher(
            perfmon_collection,
            to_version_map(blobs));

        on_thread_t thread_switcher_2(home_thread());

        /* Send a request for the coordinator to register our branch */
        {
            our_branch_id = boost::make_optional(primary_dispatcher.get_branch_id());
            contract_ack_t ack(contract_ack_t::state_t::primary_need_branch);
            ack.branch = boost::make_optional(*our_branch_id);
            context->branch_history_manager->export_branch_history(
                branch_bc.origin, &ack.branch_history);
            ack.branch_history.branches.insert(std::make_pair(
                *our_branch_id,
                primary_dispatcher.get_branch_birth_certificate()));
            latest_ack = boost::make_optional(ack);
            latest_contract_home_thread->ack_cb(ack);
        }

        /* Wait until we get our branch registered */
        wait_interruptible(&branch_registered, keepalive.get_drain_signal());

        /* Acquire the mutex so that instances of `update_contract_on_store_thread()`
        will be blocked out during the transitional period when we're setting up the
        local replica. At the same time as we acquire the mutex, make a local copy of
        `latest_contract_on_home_thread`. `sync_contract_with_replicas()` won't have
        happened for this contract because `our_dispatcher` was `nullptr` when
        `update_contract_on_store_thread()` ran for it. So we have to manually call
        `sync_contract_with_replicas()` for it. */
        counted_t<contract_info_t> pre_replica_contract = latest_contract_home_thread;
        scoped_ptr_t<new_mutex_in_line_t> pre_replica_mutex_in_line(
            new new_mutex_in_line_t(&mutex));
        wait_interruptible(
            pre_replica_mutex_in_line->acq_signal(), keepalive.get_drain_signal());

        /* Set up the `local_replicator_t`, `remote_replicator_server_t`, and
        `primary_query_server_t`. */
        on_thread_t thread_switcher_3(store->home_thread());

        local_replicator_t local_replicator(
            server_id,
            &primary_dispatcher,
            store,
            context->branch_history_manager);

        remote_replicator_server_t remote_replicator_server(
            context->mailbox_manager,
            &primary_dispatcher);

        our_dispatcher = &primary_dispatcher;

        primary_query_server_t primary_query_server(
            context->mailbox_manager,
            region,
            this);

        on_thread_t thread_switcher_4(home_thread());

        /* OK, now we have to make sure that `sync_contract_with_replicas()` gets called
        for `pre_broadcaster_contract` by spawning an extra copy of
        `update_contract_on_store_thread()` manually. A copy was already spawned for the
        contract when it first came in, but that copy saw `our_broadcaster` as `nullptr`
        so it didn't call `sync_contract_with_replicas()`. */
        coro_t::spawn_sometime(std::bind(
            &primary_execution_t::update_contract_on_store_thread, this,
            pre_replica_contract,
            keepalive,
            pre_replica_mutex_in_line.release()));

        /* Put an entry in the minidir so the replicas can find us */
        contract_execution_bcard_t ce_bcard;
        ce_bcard.remote_replicator_server = remote_replicator_server.get_bcard();
        ce_bcard.replica = local_replicator.get_replica_bcard();
        ce_bcard.peer = context->mailbox_manager->get_me();
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t>::entry_t minidir_entry(
                context->local_contract_execution_bcards,
                std::make_pair(context->server_id, our_branch_id),
                ce_bcard);

        /* Put an entry in the global directory so clients can find us */
        table_query_bcard_t tq_bcard;
        tq_bcard.region = region;
        tq_bcard.primary = boost::make_optional(primary_query_server.get_bcard());
        watchable_map_var_t<uuid_u, table_query_bcard_t>::entry_t directory_entry(
            context->local_table_query_bcards, generate_uuid(), tq_bcard);

        /* Wait until we are no longer the primary, or it's time to shut down */
        keepalive.get_drain_signal()->wait_lazily_unordered();

    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}

bool primary_execution_t::on_write(
        const write_t &request,
        fifo_enforcer_sink_t::exit_write_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        write_response_t *response_out,
        std::string *error_out) {
    store->assert_thread();
    guarantee(our_dispatcher != nullptr);

    if (static_cast<bool>(latest_contract_store_thread->contract.primary->hand_over)) {
        *error_out = "The primary replica is currently changing from one replica to "
            "another. The write was not performed. This error should go away in a "
            "couple of seconds.";
        return false;
    }

    /* RSI(raft): Right now we ack the write when it's safe (i.e. won't be lost by
    failover or othe reconfiguration). We should let the user control when the write is
    acked, by adjusting both the durability and the ack thresholds. But the initial test
    should still check for the stronger of the user's condition and the safety condition;
    for example, if the user has three replicas and they set the ack threshold to one,
    and only the primary replica is available, we should reject the write because it's
    impossible for it to be safe, even though the user's condition is satisfiable. */

    /* Make sure we have contact with a quorum of replicas. If not, don't even attempt
    the write. This is because the user would get confused if their write returned an
    error about "not enough acks" and then got applied anyway. */
    {
        ack_counter_t counter(latest_contract_store_thread);
        our_dispatcher->get_readable_dispatchees()->apply_read(
            [&](const std::set<server_id_t> *servers) {
                for (const server_id_t &s : *servers) {
                    counter.note_ack(s);
                }
            });
        if (!counter.is_safe()) {
            *error_out = "The primary replica isn't connected to a quorum of replicas. "
                "The write was not performed.";
            return false;
        }
    }

    /* `write_callback_t` waits until the query is safe to ack, then pulses `done`. */
    class write_callback_t : public primary_dispatcher_t::write_callback_t {
    public:
        write_callback_t(write_response_t *_r_out, std::string *_e_out,
                counted_t<contract_info_t> contract) :
            ack_counter(contract), response_out(_r_out), error_out(_e_out) { }
        write_durability_t get_default_write_durability() {
            /* This only applies to writes that don't specify the durability */
            return write_durability_t::HARD;
        }
        void on_ack(const server_id_t &server, write_response_t &&resp) {
            if (!done.is_pulsed()) {
                ack_counter.note_ack(server);
                if (ack_counter.is_safe()) {
                    *response_out = std::move(resp);
                    done.pulse();
                }
            }
        }
        void on_end() {
            if (!done.is_pulsed()) {
                *error_out = "The primary replica lost contact with the secondary "
                    "replicas. The write may or may not have been performed.";
                done.pulse();
            }
        }
        ack_counter_t ack_counter;
        cond_t done;
        write_response_t *response_out;
        std::string *error_out;
    } write_callback(response_out, error_out, latest_contract_store_thread);

    /* It's important that we don't block between making a local copy of
    `latest_contract_store_thread` (above) and calling `spawn_write()` (below) */

    our_dispatcher->spawn_write(request, order_token, &write_callback);

    /* This will allow other calls to `on_write()` to happen. */
    exiter->end();

    wait_interruptible(&write_callback.done, interruptor);
    return write_callback.ack_counter.is_safe();
}

bool primary_execution_t::on_read(
        const read_t &request,
        fifo_enforcer_sink_t::exit_read_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out,
        std::string *error_out) {
    store->assert_thread();
    guarantee(our_dispatcher != nullptr);
    try {
        our_dispatcher->read(
            request,
            exiter,
            order_token,
            interruptor,
            response_out);
        return true;
    } catch (const cannot_perform_query_exc_t &e) {
        *error_out = e.what();
        return false;
    }
}

void primary_execution_t::update_contract_on_store_thread(
        counted_t<contract_info_t> contract,
        auto_drainer_t::lock_t keepalive,
        new_mutex_in_line_t *sync_mutex_in_line_ptr) {
    scoped_ptr_t<new_mutex_in_line_t> sync_mutex_in_line(sync_mutex_in_line_ptr);
    assert_thread();

    try {
        wait_any_t interruptor_home_thread(
            &contract->obsolete, keepalive.get_drain_signal());

        /* Wait until we hold `sync_mutex`, so that we don't reorder contracts if several
        new contracts come in quick succession */
        wait_interruptible(sync_mutex_in_line->acq_signal(), &interruptor_home_thread);

        bool should_ack;
        {
            /* Go to the store's thread */
            cross_thread_signal_t interruptor_store_thread(
                &interruptor_home_thread, store->home_thread());
            on_thread_t thread_switcher(store->home_thread());

            /* Deliver the latest contract */
            latest_contract_store_thread = contract;

            /* If we have a broadcaster, then try to sync with replicas so we can ack the
            contract */
            if (our_dispatcher != nullptr) {
                should_ack = true;
                sync_contract_with_replicas(contract, &interruptor_store_thread);
            } else {
                should_ack = false;
            }
        }

        if (should_ack) {
            /* OK, time to ack the contract */
            latest_ack = boost::make_optional(
                contract_ack_t(contract_ack_t::state_t::primary_ready));
            contract->ack_cb(*latest_ack);
        }

    } catch (const interrupted_exc_t &) {
        /* Either the contract is obsolete or we are being destroyed. In either case,
        stop trying to ack the contract. */
    }
}

void primary_execution_t::sync_contract_with_replicas(
        counted_t<contract_info_t> contract,
        signal_t *interruptor) {
    store->assert_thread();
    guarantee(our_dispatcher != nullptr);
    while (!interruptor->is_pulsed()) {
        /* Wait until it looks like the write could go through */
        our_dispatcher->get_readable_dispatchees()->run_until_satisfied(
            [&](const std::set<server_id_t> &servers) {
                return is_contract_ackable(contract, servers);
            },
            interruptor);
        /* Now try to actually put the write through */
        class safe_write_callback_t : public primary_dispatcher_t::write_callback_t {
        public:
            write_durability_t get_default_write_durability() {
                return write_durability_t::HARD;
            }
            void on_ack(const server_id_t &server, write_response_t &&) {
                servers.insert(server);
                if (is_contract_ackable(contract, servers)) {
                    done.pulse_if_not_already_pulsed();
                }
            }
            void on_end() {
                done.pulse_if_not_already_pulsed();
            }
            counted_t<contract_info_t> contract;
            std::set<server_id_t> servers;
            cond_t done;
        } write_callback;
        write_callback.contract = contract;
        our_dispatcher->spawn_write(write_t::make_sync(region),
            order_token_t::ignore, &write_callback);
        wait_interruptible(&write_callback.done, interruptor);
        if (is_contract_ackable(contract, write_callback.servers)) {
            break;
        }
    }
}

bool primary_execution_t::is_contract_ackable(
        counted_t<contract_info_t> contract, const std::set<server_id_t> &servers) {
    /* If it's a regular contract, we can ack it as soon as we send a sync to a quorum of
    replicas. If it's a hand-over contract, we can ack it as soon as we send a sync to
    the new primary. */
    if (!static_cast<bool>(contract->contract.primary->hand_over)) {
        ack_counter_t ack_counter(contract);
        for (const server_id_t &s : servers) {
            ack_counter.note_ack(s);
        }
        return ack_counter.is_safe();
    } else {
        return servers.count(*contract->contract.primary->hand_over) == 1;
    }
}

