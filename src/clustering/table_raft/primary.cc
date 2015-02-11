// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/primary.hpp"

primary_t::primary_t(
        const server_id_t &sid,
        store_view_t *s,
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *pbcs,
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb) :
    server_id(sid),
    store(s),
    primary_bcards(pbcs),
    region(r),
    our_branch_id(generate_uuid()),
    latest_contract(make_counted<contract_info_t>(c, acb))
    { }

void primary_t::update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb) {
    ASSERT_NO_CORO_WAITING;

    guarantee(static_cast<bool>(c.primary));
    guarantee(c.primary->server == server_id);

    /* Mark the old contract as obsolete, and record the new one */
    latest_contract->obsolete.pulse();
    latest_contract = make_counted<contract_info_t>(c, acb);

    /* Has our branch ID been registered yet? */
    if (!branch_registered.is_pulsed() && c.branch_id == our_branch_id) {
        /* This contract just issued us our initial branch ID */
        branch_registered.pulse();
        /* Change `latest_ack` immediately so we don't keep sending the branch
        registration request */
        latest_ack = boost::make_optional(contract_ack_t(
            contract_ack_t::state_t::primary_in_progress));
    }

    /* If we have a running broadcaster, then go back to acking `primary_in_progress`
    until the new write condition takes affect */
    if (our_broadcaster.get_ready_signal()->is_pulsed()) {
        guarantee(static_cast<bool>(latest_ack));
        guarantee(latest_ack->state == contract_ack_t::state_t::primary_in_progress ||
            latest_ack->state == contract_ack_t::state_t::primary_ready);
        latest_ack = boost::make_optional(contract_ack_t(
            contract_ack_t::state_t::primary_in_progress));
        /* Start a coroutine to eventually ack `primary_ready` */
        coro_t::spawn_sometime(std::bind(
            &primary_t::sync_and_ack_contract, this, latest_contract, drainer.lock()));
    }

    if (static_cast<bool>(latest_ack)) {
        latest_contract->ack_cb(*latest_ack);
    }
}

void primary_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        /* Set our initial state to `primary_need_branch`, so that the leader will assign
        us a new branch ID. */
        {
            /* Read the initial version from disk */
            region_map_t<binary_blob_t> blobs;
            read_token_t token;
            store->new_read_token(&token);
            store->do_get_metainfo(order_token_t::ignore, token,
                keepalive.get_drain_signal(), &blobs);

            /* Send a request for the leader to register our branch */
            contract_ack_t ack(contract_ack_t::state_t::primary_need_branch);
            ack.version = boost::make_optional(to_version_map(blobs));
            ack.branch_id = boost::make_optional(our_branch_id);
            latest_ack = boost::make_optional(ack);
            latest_contract->ack_cb(ack);
        }

        /* Wait until we get our branch registered */
        wait_interruptible(&branch_registered, keepalive.get_drain_signal());

        /* RSI: Must flush the branch history to disk before we call the broadcaster_t
        constructor */

        /* Set up the `broadcaster_t`, `listener_t`, and `replier_t`, which do the
        actual important work */

        broadcaster_t broadcaster(
            mailbox_manager,
            rdb_context, /* RSI */
            branch_history_manager, /* RSI */
            store,
            parent_perfmon_collection, /* RSI */
            order_source, /* RSI */
            branch_id,
            keepalive.get_drain_signal());

        listener_t listener(
            base_path, /* RSI */
            io_backender, /* RSI */
            mailbox_manager,
            server_id,
            branch_history_manager, /* RSI */
            &broadcaster,
            backfill_stats_parent, /* RSI */
            keepalive.get_drain_signal(),
            order_source /* RSI */);

        replier_t replier(
            &listener,
            mailbox_manager,
            branch_history_manager /* RSI */);

        /* Put an entry in the minidir so the replicas can find us */
        primary_bcard_t bcard;
        bcard.broadcaster = broadcaster.get_business_card();
        bcard.replier = replier.get_business_card();
        bcard.peer = mailbox_manager->get_me();
        watchable_map_var_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t>
            ::entry_t minidir_entry(&primary_bcards,
                std::make_pair(server_id, our_branch_id.wait()), bcard);

        /* Pulse `our_broadcaster` so that `update_contract()` knows we set up a
        broadcaster */
        our_broadcaster.pulse(&broadcaster);

        /* Start the process of acking the current contract */
        if (!keepalive.get_drain_signal()->is_pulsed()) {
            coro_t::spawn_sometime(std::bind(&primary_t::sync_and_ack_contract, this,
                latest_contract, drainer.lock()));
        }

        /* Set up the `master_t` and put an entry in the directory so we can receive
        queries */
        master_t master(mailbox_manager, region, this);

        /* RSI(raft): Publish the `master_t` in the directory so we can receive
        queries */

    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}

bool primary_t::on_write(
        const write_t &request,
        fifo_enforcer_sink_t::exit_write_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        write_response_t *response_out,
        std::string *error_out) {
    guarantee(our_broadcaster.get_ready_signal()->is_pulsed());

    if (static_cast<bool>(latest_contract->contract.primary->hand_over)) {
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
    the write. This is mostly to reduce user confusion. */
    {
        ack_counter_t counter;
        our_broadcaster.wait()->get_readable_dispatchees()->apply_read(
            [&](const std::set<server_id_t> &servers) {
                for (const server_id_t &s : servers) {
                    counter.note_ack(s);
                }
            });
        if (!counter.is_safe()) {
            *error_out = "The primary replica isn't connected to a quorum of replicas. "
                "The write was not performed.";
            return false;
        }
    }

    class write_callback_t : public broadcaster_t::write_callback_t {
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
    } write_callback(response_out, error_out, latest_contract);

    /* It's important that we don't block between making a local copy of
    `latest_contract` (above) and calling `spawn_write()` (below) */

    our_broadcaster->wait()->spawn_write(
        request,
        order_token,
        &write_callback,
        local_ack_condition.get());

    exiter->exit();

    wait_interruptible(&write_callback.done, interruptor);
    return write_callback.ack_counter.is_safe();
}

bool primary_t::on_read(
        const read_t &request,
        fifo_enforcer_sink_t::exit_read_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out,
        std::string *error_out) {
    guarantee(our_broadcaster.get_ready_signal()->is_pulsed());
    try {
        our_broadcaster.wait()->read(
            request,
            response_out,
            exiter,
            order_token,
            interruptor);
        return true;
    } catch (const cannot_perform_query_exc_t &e) {
        *error_out = e.what();
        return false;
    }
}

bool primary_t::is_contract_ackable(
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

void primary_t::sync_and_ack_contract(
        counted_t<contract_info_t> contract,
        auto_drainer_t::lock_t keepalive) {
    try {
        wait_any_t interruptor(&contract->obsolete, keepalive.get_drain_signal());
        while (!interruptor.is_pulsed()) {
            /* Wait until it looks like the write could go through */
            our_broadcaster.wait()->get_readable_dispatchees()->run_until_satisfied(
                std::bind(&primary_t::is_contract_ackable, contract, ph::_1),
                &interruptor);
            /* Now try to actually put the write through */
            class safe_write_callback_t : public broadcaster_t::write_callback_t {
            public:
                safe_write_callback_t(counted_t<contract_info_t> c) : contract(c) { }
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
            our_broadcaster.wait()->spawn_write(write_t::make_sync(),
                order_token_t::ignore(), &write_callback);
            wait_interruptible(&write_callback.done, &interruptor);
            if (is_contract_ackable(contract, write_callback.servers)) {
                break;
            }
        }
        /* OK, time to ack the contract */
        contract->ack_cb(contract_ack_t(contract_ack_t::state_t::primary_ready));
    } catch (const interrupted_exc_t &) {
        /* Either the contract is obsolete or we are being destroyed. In either case,
        stop trying to ack the contract. */
    }
}

