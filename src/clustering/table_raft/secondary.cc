// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/secondary.hpp"

namespace table_raft {

secondary_t::secondary_t(
        const server_id_t &sid,
        store_view_t *s,
        watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *pbcs,
        const region_t &r,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb) :
    server_id(sid), store(s), primary_bcards(pbcs), region(r),
    primary(static_cast<bool>(c.primary) ? c.primary->server : nil_uuid()),
    branch(c.branch),
    ack_cb(acb)
{
    guarantee(s->get_region() == region);
    coro_t::spawn_sometime(std::bind(&secondary_t::run, this, drainer.lock()));
}

void secondary_t::update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb) {
    guarantee(primary == static_cast<bool>(c.primary) ? c.primary->server : nil_uuid());
    guarantee(branch == c.branch);
    guarantee(c.replicas.count(server_id) == 1);
    ack_cb = acb;
    if (static_cast<bool>(last_ack)) {
        ack_cb(*last_ack);
    }
}

void secondary_t::run(auto_drainer_t::lock_t keepalive) {
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        try {
            /* Set our initial state to `secondary_need_primary`. */
            {
                region_map_t<binary_blob_t> blobs;
                read_token_t token;
                store->new_read_token(&token);
                store->do_get_metainfo(order_token_t::ignore, token,
                    keepalive.get_drain_signal(), &blobs);
                contract_ack_t ack(contract_ack_t::state_t::secondary_need_primary);
                ack.version = boost::make_optional(to_version_map(blobs));
                send_ack(ack);
            }

            /* If the contract has no primary, we stop here. */
            if (primary.is_nil()) {
                keepalive.get_drain_signal()->wait_lazily_unordered();
                return;
            }

            /* Wait until we see the primary. */
            primary_bcard_t primary_bcard;
            primary_bcards->run_key_until_satisfied(
                std::make_pair(primary, branch),
                [&](const primary_bcard_t *pbc) {
                    if (pbc != nullptr) {
                        primary_bcard = *pbc;
                        return true;
                    } else {
                        return false;
                    }
                }, keepalive.get_drain_signal());
            guarantee(primary_bcard.broadcaster.branch_id == branch_id);

            /* Let the leader know we found the primary. */
            send_ack(contract_ack_t(contract_ack_t::state_t::secondary_backfilling));

            /* Set up a signal that will get pulsed if we lose contact with the primary
            or we get interrupted */
            cond_t no_more_bcard_signal;
            watchable_map_t<std::pair<server_id_t, branch_id_t>,
                            primary_bcard_t>::key_subs_t subs(
                primary_bcards,
                std::make_pair(primary, branch),
                [&](const primary_bcard_t *pbc) {
                    if (pbc == nullptr) {
                        no_more_bcard_signal.pulse();
                    }
                },
                true);
            disconnect_watcher_t disconnect_signal(mailbox_manager, primary_bcard.peer);
            wait_any_t stop_signal(
                &no_more_bcard_signal,
                &disconnect_signal,
                keepalive.get_drain_signal());

            /* Backfill and start streaming from the primary. */
            listener_t listener(
                base_path, /* RSI */
                io_backender, /* RSI */
                mailbox_manager,
                server_id,
                backfill_throttler, /* RSI */
                primary_bcard.broadcaster,
                branch_history_manager, /* RSI */
                store,
                primary_bcard.replier,
                backfill_stats_parent, /* RSI */
                &stop_signal,
                order_source, /* RSI */
                nullptr); /* RSI(raft): Hook up backfill progress again */

            /* Let the leader know we finished backfilling */
            send_ack(contract_ack_t(contract_ack_t::state_t::secondary_streaming));

            /* Wait until we lose contact with the primary or we get interrupted */
            stop_signal.wait_lazily_unordered();

        } catch (const interrupted_exc_t &) {
            /* do nothing */
        }
    }
}

void secondary_t::send_ack(const contract_ack_t &ca) {
    ack_cb(ca);
    last_ack = boost::make_optional(ca);
}

} /* namespace table_raft */

