// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/executor/exec_primary.hpp"

#include "clustering/administration/admin_op_exc.hpp"
#include "clustering/immediate_consistency/local_replicator.hpp"
#include "clustering/immediate_consistency/primary_dispatcher.hpp"
#include "clustering/immediate_consistency/remote_replicator_server.hpp"
#include "clustering/query_routing/direct_query_server.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/promise.hpp"
#include "store_view.hpp"

primary_execution_t::primary_execution_t(
        const execution_t::context_t *_context,
        execution_t::params_t *_params,
        const contract_id_t &contract_id,
        const table_raft_state_t &raft_state) :
    execution_t(_context, _params), our_dispatcher(nullptr)
{
    const contract_t &contract = raft_state.contracts.at(contract_id).second;
    guarantee(static_cast<bool>(contract.primary));
    guarantee(contract.primary->server == context->server_id);
    guarantee(raft_state.contracts.at(contract_id).first == region);
    latest_contract_home_thread = make_counted<contract_info_t>(
        contract_id, contract, raft_state.config.config.durability,
        raft_state.config.config.write_ack_config);
    latest_contract_store_thread = latest_contract_home_thread;
    begin_write_mutex_assertion.rethread(store->home_thread());
    coro_t::spawn_sometime(std::bind(&primary_execution_t::run, this, drainer.lock()));
}

primary_execution_t::~primary_execution_t() {
    drainer.drain();
    begin_write_mutex_assertion.rethread(home_thread());
}

void primary_execution_t::update_contract_or_raft_state(
        const contract_id_t &contract_id,
        const table_raft_state_t &raft_state) {
    assert_thread();
    ASSERT_NO_CORO_WAITING;

    /* Has our branch ID been registered yet? */
    if (!branch_registered.is_pulsed()) {
        bool branch_matches = true;
        raft_state.current_branches.visit(region,
        [&](const region_t &, const branch_id_t &b) {
            if (boost::make_optional(b) != our_branch_id) {
                branch_matches = false;
            }
        });
        if (branch_matches) {
            rassert(!branch_registered.is_pulsed());
            /* This raft state update just confirmed our branch ID */
            branch_registered.pulse();
            /* Change `latest_ack` immediately so we don't keep sending the branch
            registration request */
            latest_ack = boost::make_optional(contract_ack_t(
                contract_ack_t::state_t::primary_in_progress));
            params->send_ack(contract_id, *latest_ack);
        }
    }

    const contract_t &contract = raft_state.contracts.at(contract_id).second;
    guarantee(static_cast<bool>(contract.primary));
    guarantee(contract.primary->server == context->server_id);
    guarantee(raft_state.contracts.at(contract_id).first == region);

    counted_t<contract_info_t> new_contract = make_counted<contract_info_t>(
        contract_id,
        contract,
        raft_state.config.config.durability,
        raft_state.config.config.write_ack_config);

    /* Exit early if there aren't actually any changes. This is for performance reasons.
    */
    if (new_contract->equivalent(*latest_contract_home_thread)) {
        return;
    }

    /* Mark the old contract as obsolete, and record the new one */
    latest_contract_home_thread->obsolete.pulse();
    latest_contract_home_thread = new_contract;

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
        new new_mutex_in_line_t(&update_contract_mutex)));

    /* Send an ack for the new contract */
    if (static_cast<bool>(latest_ack)) {
        params->send_ack(contract_id, *latest_ack);
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

        read_token_t token;
        store->new_read_token(&token);
        region_map_t<version_t> initial_version = to_version_map(store->get_metainfo(
            order_source.check_in("primary_t").with_read_mode(),
            &token, region, &interruptor_store_thread));

        perfmon_collection_t perfmon_collection;
        perfmon_membership_t perfmon_membership(params->get_parent_perfmon_collection(),
                                                &perfmon_collection,
                                                params->get_perfmon_name());

        primary_dispatcher_t primary_dispatcher(&perfmon_collection, initial_version);

        direct_query_server_t direct_query_server(
            context->mailbox_manager,
            store);

        on_thread_t thread_switcher_2(home_thread());

        /* Put an entry in the global directory so clients can find us for outdated reads
        */
        table_query_bcard_t tq_bcard_direct;
        tq_bcard_direct.region = region;
        tq_bcard_direct.primary = boost::none;
        tq_bcard_direct.direct = boost::make_optional(direct_query_server.get_bcard());
        watchable_map_var_t<uuid_u, table_query_bcard_t>::entry_t directory_entry_direct(
            context->local_table_query_bcards, generate_uuid(), tq_bcard_direct);

        /* Send a request for the coordinator to register our branch */
        {
            our_branch_id = boost::make_optional(primary_dispatcher.get_branch_id());
            contract_ack_t ack(contract_ack_t::state_t::primary_need_branch);
            ack.branch = boost::make_optional(*our_branch_id);
            context->branch_history_manager->export_branch_history(
                primary_dispatcher.get_branch_birth_certificate().origin,
                &ack.branch_history);
            ack.branch_history.branches.insert(std::make_pair(
                *our_branch_id,
                primary_dispatcher.get_branch_birth_certificate()));
            latest_ack = boost::make_optional(ack);
            params->send_ack(latest_contract_home_thread->contract_id, ack);
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
            new new_mutex_in_line_t(&update_contract_mutex));
        wait_interruptible(
            pre_replica_mutex_in_line->acq_signal(), keepalive.get_drain_signal());

        /* Set up the `local_replicator_t`, `remote_replicator_server_t`, and
        `primary_query_server_t`. */
        on_thread_t thread_switcher_3(store->home_thread());

        local_replicator_t local_replicator(
            context->mailbox_manager,
            context->server_id,
            &primary_dispatcher,
            store,
            context->branch_history_manager,
            &interruptor_store_thread);

        remote_replicator_server_t remote_replicator_server(
            context->mailbox_manager,
            &primary_dispatcher);

        auto_drainer_t primary_dispatcher_drainer;
        assignment_sentry_t<auto_drainer_t *> our_dispatcher_drainer_assign(
            &our_dispatcher_drainer, &primary_dispatcher_drainer);
        assignment_sentry_t<primary_dispatcher_t *> our_dispatcher_assign(
            &our_dispatcher, &primary_dispatcher);

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

        /* The `local_replicator_t` constructor set the metainfo to `*our_branch_id`, so
        it's now safe to call `enable_gc()`. */
        params->enable_gc(*our_branch_id);

        /* Put an entry in the minidir so the replicas can find us */
        contract_execution_bcard_t ce_bcard;
        ce_bcard.remote_replicator_server = remote_replicator_server.get_bcard();
        ce_bcard.replica = local_replicator.get_replica_bcard();
        ce_bcard.peer = context->mailbox_manager->get_me();
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>,
            contract_execution_bcard_t>::entry_t minidir_entry(
                context->local_contract_execution_bcards,
                std::make_pair(context->server_id, *our_branch_id),
                ce_bcard);

        /* Put an entry in the global directory so clients can find us for up-to-date
        writes and reads */
        table_query_bcard_t tq_bcard_primary;
        tq_bcard_primary.region = region;
        tq_bcard_primary.primary =
            boost::make_optional(primary_query_server.get_bcard());
        tq_bcard_primary.direct = boost::none;
        watchable_map_var_t<uuid_u, table_query_bcard_t>::entry_t
            directory_entry_primary(
                context->local_table_query_bcards, generate_uuid(), tq_bcard_primary);

        /* Wait until we are no longer the primary, or it's time to shut down */
        keepalive.get_drain_signal()->wait_lazily_unordered();

    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}

/* `write_callback_t` waits until the query is safe to ack, then pulses `done`. */
class primary_execution_t::write_callback_t : public primary_dispatcher_t::write_callback_t {
public:
    write_callback_t(write_response_t *_r_out,
                     write_durability_t _default_write_durability,
                     write_ack_config_t _write_ack_config,
                     contract_t *contract) :
        ack_counter(*contract),
        default_write_durability(_default_write_durability),
        write_ack_config(_write_ack_config),
        response_out(_r_out) { }

    promise_t<bool> result;
private:
    write_durability_t get_default_write_durability() {
        /* This only applies to writes that don't specify the durability */
        return default_write_durability;
    }
    void on_ack(const server_id_t &server, write_response_t &&resp) {
        if (!result.is_pulsed()) {
            switch (write_ack_config) {
                case write_ack_config_t::SINGLE:
                    break;
                case write_ack_config_t::MAJORITY:
                    ack_counter.note_ack(server);
                    if (!ack_counter.is_safe()) {
                        // We're not ready to safely ack the query yet.
                        return;
                    }
                    break;
            }
            *response_out = std::move(resp);
            result.pulse(true);
        }
    }
    void on_end() {
        if (!result.is_pulsed()) {
            result.pulse(false);
        }
    }
    ack_counter_t ack_counter;
    write_durability_t default_write_durability;
    write_ack_config_t write_ack_config;
    write_response_t *response_out;
};

bool primary_execution_t::on_write(
        const write_t &request,
        fifo_enforcer_sink_t::exit_write_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        write_response_t *response_out,
        admin_err_t *error_out) {
    store->assert_thread();
    guarantee(our_dispatcher != nullptr);

    /* `acq` asserts that `update_contract_on_store_thread()` doesn't run between when we
    take `contract_snapshot` and when we call `spawn_write()`. This is important because
    `update_contract_on_store_thread()` needs to be able to make sure that all writes
    that see the old contract are spawned before it calls
    `sync_contract_with_replicas()`. */
    mutex_assertion_t::acq_t begin_write_mutex_acq(&begin_write_mutex_assertion);

    /* The reason that the `begin_write_mutex_acq` holds, is because we don't block in
    here nor anywhere else where we acquire the mutex assertion. */
    DEBUG_ONLY(scoped_ptr_t<assert_finite_coro_waiting_t> finite_coro_waiting(
                   make_scoped<assert_finite_coro_waiting_t>(__FILE__, __LINE__)));

    counted_t<contract_info_t> contract_snapshot = latest_contract_store_thread;

    if (static_cast<bool>(contract_snapshot->contract.primary->hand_over)) {
        *error_out = admin_err_t{
            "The primary replica is currently changing from one replica to "
            "another. The write was not performed. This error should go away in a "
            "couple of seconds.",
            query_state_t::FAILED};
        return false;
    }

    /* Make sure that we have contact with a majority of replicas. It's bad practice to
    accept writes if we can't contact a majority of replicas because those writes might
    be lost during a failover. We do this even if the user set the ack threshold to
    "single". */
    if (!is_majority_available(contract_snapshot, our_dispatcher)) {
        *error_out = admin_err_t{
            "The primary replica isn't connected to a quorum of replicas. "
            "The write was not performed.",
            query_state_t::FAILED};
        return false;
    }

    write_callback_t write_callback(response_out,
                                    contract_snapshot->default_write_durability,
                                    contract_snapshot->write_ack_config,
                                    &contract_snapshot->contract);
    our_dispatcher->spawn_write(request, order_token, &write_callback);

    /* Now that we've called `spawn_write()`, our write is in the queue. So it's safe to
    release the `begin_write_mutex`; any calls to `update_contract_on_store_thread()`
    will enter the queue after us. */
    DEBUG_ONLY(finite_coro_waiting.reset());
    begin_write_mutex_acq.reset();

    /* This will allow other calls to `on_write()` to happen. */
    exiter->end();

    wait_interruptible(write_callback.result.get_ready_signal(), interruptor);

    bool res = write_callback.result.assert_get_value();
    if (!res) {
        *error_out = admin_err_t{
            "The primary replica lost contact with the secondary "
            "replicas. The write may or may not have been performed.",
            query_state_t::INDETERMINATE};
    }
    return res;
}

bool primary_execution_t::sync_committed_read(const read_t &read_request,
                                              order_token_t order_token,
                                              signal_t *interruptor,
                                              admin_err_t *error_out) {
    write_response_t response;
    write_t request = write_t::make_sync(read_request.get_region(),
                                         read_request.profile);

    /* See the comments in `on_write` for an explanation about why we're acquiring
    `begin_write_mutex_assertion` here. */
    mutex_assertion_t::acq_t begin_write_mutex_acq(&begin_write_mutex_assertion);
    DEBUG_ONLY(scoped_ptr_t<assert_finite_coro_waiting_t> finite_coro_waiting(
                   make_scoped<assert_finite_coro_waiting_t>(__FILE__, __LINE__)));
    counted_t<contract_info_t> contract_snapshot = latest_contract_store_thread;

    if (static_cast<bool>(contract_snapshot->contract.primary->hand_over)) {
        *error_out = admin_err_t{
            "The primary replica is currently changing from one replica to "
            "another. The read could not be guaranteed as committed. This error "
            "should go away in a couple of seconds.",
            query_state_t::FAILED};
        return false;
    }

    write_callback_t write_callback(&response,
                                    write_durability_t::HARD,
                                    write_ack_config_t::MAJORITY,
                                    &contract_snapshot->contract);
    our_dispatcher->spawn_write(request, order_token, &write_callback);

    DEBUG_ONLY(finite_coro_waiting.reset());
    begin_write_mutex_acq.reset();

    wait_interruptible(write_callback.result.get_ready_signal(), interruptor);

    bool res = write_callback.result.assert_get_value();
    if (!res) {
        *error_out = admin_err_t{
            "The primary replica lost contact with the secondary "
            "replicas. The read could not be guaranteed as committed.",
            query_state_t::FAILED};
    }
    return res;
}

bool primary_execution_t::on_read(
        const read_t &request,
        fifo_enforcer_sink_t::exit_read_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out,
        admin_err_t *error_out) {
    store->assert_thread();
    guarantee(our_dispatcher != nullptr);

    counted_t<contract_info_t> contract_snapshot = latest_contract_store_thread;

    switch (request.read_mode) {
    case read_mode_t::SINGLE:
    case read_mode_t::MAJORITY: // Fallthrough intentional
        /* Make sure that we have contact with a majority of replicas, if we've lost
        this we may no longer be able to do up-to-date reads. */
        if (!is_majority_available(contract_snapshot, our_dispatcher)) {
            *error_out = admin_err_t{
                "The primary replica isn't connected to a quorum of replicas. "
                "The read was not performed, you can do an outdated read using "
                "`read_mode=\"outdated\"`.",
                query_state_t::FAILED};
            return false;
        }
        break;
    case read_mode_t::OUTDATED: // Fallthrough intentional
    case read_mode_t::DEBUG_DIRECT:
    default:
        // These read modes should not come through the `primary_exection_t`.
        unreachable();
    }

    try {
        our_dispatcher->read(
            request,
            exiter,
            order_token,
            interruptor,
            response_out);

        if (request.read_mode == read_mode_t::MAJORITY) {
            return sync_committed_read(request, order_token, interruptor, error_out);
        } else {
            return true;
        }
    } catch (const cannot_perform_query_exc_t &e) {
        *error_out = admin_err_t{e.what(), e.get_query_state()};
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

            /* We can only send a `primary_ready` response to the contract coordinator
            once we're sure that both all incoming writes will be executed under the new
            contract, and all previous writes are safe under the conditions of the new
            contract. We ensure the former condition by setting
            `latest_contract_store_thread`, so that incoming writes will pick up the new
            contract. We ensure the latter condition by running sync writes until we get
            one to succeed under the new contract. The sync writes must enter the
            dispatcher's queue after any writes that were run under the old contract, so
            that the sync writes can't succeed under the new contract until the old
            writes are also safe under the new contract. */

            /* Deliver the latest contract */
            {
                /* We acquire `begin_write_mutex_assertion` to assert that we're not
                interleaving with a call to `on_write()`. This way, we can be sure that
                any writes that saw the old contract will enter the primary dispatcher's
                queue before we call `sync_contract_with_replicas()`. */
                mutex_assertion_t::acq_t begin_write_mutex_acq(
                    &begin_write_mutex_assertion);
                ASSERT_FINITE_CORO_WAITING;
                latest_contract_store_thread = contract;
            }

            /* If we have a broadcaster, then try to sync with replicas so we can ack the
            contract. */
            if (our_dispatcher != nullptr) {
                /* We're going to hold a pointer to the `primary_dispatcher_t` that lives
                on the stack in `run()`, so we need to make sure we'll stop before
                `run()` stops. We do this by acquiring a lock on the `auto_drainer_t` on
                the stack in `run()`. */
                auto_drainer_t::lock_t keepalive2 = our_dispatcher_drainer->lock();
                wait_any_t combiner(
                    &interruptor_store_thread, keepalive2.get_drain_signal());
                should_ack = true;
                sync_contract_with_replicas(contract, &combiner);
            } else {
                should_ack = false;
            }
        }

        if (should_ack) {
            /* OK, time to ack the contract */
            latest_ack = boost::make_optional(
                contract_ack_t(contract_ack_t::state_t::primary_ready));
            params->send_ack(contract->contract_id, *latest_ack);
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
    while (true) {
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }

        /* Wait until it looks like the write could go through */
        our_dispatcher->get_ready_dispatchees()->run_until_satisfied(
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
        our_dispatcher->spawn_write(
            write_t::make_sync(region, profile_bool_t::DONT_PROFILE),
            order_token_t::ignore, &write_callback);
        wait_interruptible(&write_callback.done, interruptor);
        if (is_contract_ackable(contract, write_callback.servers)) {
            break;
        }
    }
}

bool primary_execution_t::is_contract_ackable(
        counted_t<contract_info_t> contract_info, const std::set<server_id_t> &servers) {
    /* If it's a regular contract, we can ack it as soon as we send a sync to a quorum of
    replicas. If it's a hand-over contract, we also need to ensure that we performed a
    sync with the new primary.

    If we return `true` from this function, we are going to send a `primary_ready` ack
    to the coordinator. The coordinator relies on this status for two things:
    * if the we are in a configuration with `temp_voters`, the coordinator is going to
     use the `primary_ready` ack as a confirmation that a majority of `voters` as well as
     a majority of `temp_voters` have up-to-date data. Thus it will be able to safely
     turn `temp_voters` into `voters` (this is critical for correctness).
    * if we currently have a `hand_over` primary set, the coordinator will wait for the
     `primary_ready` ack before it shuts down the current primary (us).
     Our stricter criteria for `hand_over` configurations is to make sure that when the
     time comes to start up a new primary, the `hand_over` primary is up to date and
     actually eligible to become a primary.
     (the additional criteria is not critical for correctness, but makes sure that the
     transition to the new primary goes through smoothly) */
    if (static_cast<bool>(contract_info->contract.primary->hand_over) &&
        servers.count(*contract_info->contract.primary->hand_over) != 1) {
        return false;
    }
    ack_counter_t ack_counter(contract_info->contract);
    for (const server_id_t &s : servers) {
        ack_counter.note_ack(s);
    }
    return ack_counter.is_safe();
}

bool primary_execution_t::is_majority_available(
        counted_t<contract_info_t> contract_info,
        primary_dispatcher_t *dispatcher) {
    ack_counter_t counter(contract_info->contract);
    dispatcher->get_ready_dispatchees()->apply_read(
        [&](const std::set<server_id_t> *servers) {
            for (const server_id_t &s : *servers) {
                counter.note_ack(s);
            }
        });
    return counter.is_safe();
}
