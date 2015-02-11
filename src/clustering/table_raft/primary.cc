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
    original_branch_id(c.branch_id),
{
    update_contract(c, acb);
}

void primary_t::update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb) {
    ASSERT_NO_CORO_WAITING;

    guarantee(static_cast<bool>(c.primary));
    guarantee(c.primary->server == server_id);

    /* Did this contract assign us a branch ID? */
    if (!our_branch_id.get_ready_signal()->is_pulsed() &&
            c.branch_id != original_branch_id) {
        /* This contract just issued us our initial branch ID */
        our_branch_id.pulse(c.branch_id);
        /* Change `last_ack` immediately so we don't re-send another branch ID request */
        last_ack = boost::make_optional(contract_ack_t(
            contract_ack_t::state_t::primary_in_progress));
    }

    /* Record the new ack condition */
    ack_condition = make_counted<ack_condition_t>();
    if (static_cast<bool>(c.primary->hand_over)) {
        ack_condition->hand_over = true;
    } else {
        ack_condition->hand_over = false;
        voters = c.voters;
        temp_voters = c.temp_voters;
    }

    /* If we have a running broadcaster, then go back to acking `primary_in_progress`
    until the new write condition takes affect */
    if (our_broadcaster.get_ready_signal()->is_pulsed()) {
        guarantee(last_ack->state == contract_ack_t::state_t::primary_in_progress ||
            last_ack->state == contract_ack_t::state_t::primary_ready);
        last_ack = boost::make_optional(contract_ack_t(
            contract_ack_t::state_t::primary_in_progress));

        /* Start a dummy write to force a sync to disk */
        class dummy_write_callback_t : public broadcaster_t::write_callback_t {
        public:
            void on_ack(const server_id_t &, write_response_t &&) { }
            void on_end() { }
        } write_callback;
        state_timestamp_t timestamp = our_broadcaster.wait()->spawn_write(
            write_t(
                sync_t(),
                DURABILITY_REQUIREMENT_HARD,
                profile_bool_t::DONT_PROFILE, 
                ql::configured_limits_t::configured_limits_t()),
            order_token_t::ignore(),
            &write_callback);
        coro_t::spawn_sometime(std::bind(
            &primary_t::wait_for_sync, this, timestamp, acb));
    }

    ack_cb(last_ack);
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

            /* Store the initial version in `our_branch_version` so that
            `update_contract()` will be able to re-send the request if the contract
            changes before we are issued a branch ID */
            our_branch_version.pulse(to_version_map(blobs));

            /* Send a request for the leader to issue us a new branch ID */
            contract_ack_t ack(contract_ack_t::state_t::primary_need_branch);
            ack.version = boost::make_optional(our_branch_version.wait());
            send_ack(ack);
        }

        /* Wait until we get our branch ID assigned */
        wait_interruptible(our_branch_id.get_ready_signal(),
            keepalive.get_drain_signal());

        /* RSI: Must flush the branch history to disk before we call the broadcaster_t
        constructor */

        /* Set up our `broadcaster_t` */
        broadcaster_t broadcaster(
            mailbox_manager,
            rdb_context, /* RSI */
            branch_history_manager, /* RSI */
            store,
            parent_perfmon_collection, /* RSI */
            order_source, /* RSI */
            branch_id,
            keepalive.get_drain_signal());

        /* Set up the `listener_t` */
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

    } catch (const interrupted_exc_t &) {
        /* do nothing */
    }
}

void primary_t::send_ack(const contract_ack_t &ca) {
    ack_cb(ca);
    last_ack = boost::make_optional(ca);
}

bool primary_t::on_write(
        const write_t &request,
        fifo_enforcer_sink_t::exit_write_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        write_response_t *response_out,
        std::string *error_out) {
    guarantee(our_broadcaster->get_ready_signal()->is_pulsed());

    class write_callback_t : public broadcaster_t::write_callback_t {
    public:
        write_callback_t(write_response_t *_r_out, std::string *_e_out) :
            response_out(_r_out), error_out(_e_out), voter_acks(0), temp_voter_acks(0)
            { }
        void on_ack(const server_id_t &server, write_response_t &&resp) {
            if (!done.is_pulsed()) {
                voter_acks += ack_condition->voters.count(server);
                if (static_cast<bool>(ack_condition->temp_voters)) {
                    temp_voter_acks += ack_condition->temp_voters->count(server);
                }
                /* RSI(raft): Currently we wait for more than half of the voters. In
                theory we could wait for some other threshold, such as only one voter, or
                all the voters. We should let the user tune this. */
                if (voter_acks * 2 > ack_condition->voters.size() &&
                        (!static_cast<bool>(ack_condition->temp_voters) ||
                            temp_voter_acks * 2 > ack_condition->temp_voters->size())) {
                    *response_out = std::move(resp);
                    ok = true;
                    done.pulse();
                }
            }
        }
        void on_end() {
            if (!done.is_pulsed()) {
                *error_out = "The primary replica lost contact with the secondary "
                    "replicas while performing the write. It may or may not have been "
                    "perforrmed.";
                ok = false;
                done.pulse();
            }
        }
        cond_t done;
        bool ok;
        write_response_t *response_out;
        std::string *error_out;
        counted_t<ack_condition_t> ack_condition;
        size_t voter_acks, temp_voter_acks;
    } write_callback(response_out, error_out);

    {
        /* We have to copy `ack_condition` and spawn the write at the same time for
        proper synchronization with `update_contract()` */
        ASSERT_NO_CORO_WAITING;

        if (ack_condition->hand_over) {
            *error_out = "Writes are temporarily disabled while the primary replica "
                "changes. This error should go away in a couple of seconds.";
            return false;
        }
        /* RSI(raft): If the broadcaster doesn't currently have enough readable replicas
        to satisfy the ack condition, then don't even start the write. */

        write_callback.ack_condition = ack_condition;
        our_broadcaster->wait()->spawn_write(
            request,
            /* RSI(raft): Make write durability user-controllable */
            write_durability_t::HARD,
            order_token,
            &write_callback,
            local_ack_condition.get());
        exiter->exit();
    }

    wait_interruptible(&write_callback.done, interruptor);
    return write_callback.ok;
}

bool primary_t::on_read(
        const read_t &request,
        fifo_enforcer_sink_t::exit_read_t *exiter,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out,
        std::string *error_out) {
    
}

