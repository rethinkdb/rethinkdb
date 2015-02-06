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

    /* Record the new write condition */
    if (static_cast<bool>(c.primary->hand_over)) {
        hand_over = true;
        voters.clear();
        temp_voters.clear();
    } else {
        hand_over = false;
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

