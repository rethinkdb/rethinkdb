// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/secondary.hpp"

#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"

namespace table_raft {

secondary_t::secondary_t(
        const server_id_t &sid,
        mailbox_manager_t *mm,
        store_view_t *s,
        branch_history_manager_t *bhm,
        const region_t &r,
        perfmon_collection_t *pms,
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb,
        watchable_map_t<std::pair<server_id_t, branch_id_t>, primary_bcard_t> *pbcs,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        backfill_throttler_t *_backfill_throttler) :
    server_id(sid), mailbox_manager(mm), store(s), branch_history_manager(bhm),
    region(r), perfmons(pms), primary_bcards(pbcs), base_path(_base_path),
    io_backender(_io_backender), backfill_throttler(_backfill_throttler),
    primary(static_cast<bool>(c.primary) ? c.primary->server : nil_uuid()),
    branch(c.branch), ack_cb(acb)
{
    guarantee(s->get_region() == region);
    coro_t::spawn_sometime(std::bind(&secondary_t::run, this, drainer.lock()));
}

void secondary_t::update_contract(
        const contract_t &c,
        const std::function<void(contract_ack_t)> &acb) {
    guarantee(primary ==
        (static_cast<bool>(c.primary) ? c.primary->server : nil_uuid()));
    guarantee(branch == c.branch);
    guarantee(c.replicas.count(server_id) == 1);
    ack_cb = acb;
    if (static_cast<bool>(last_ack)) {
        ack_cb(*last_ack);
    }
}

void secondary_t::run(auto_drainer_t::lock_t keepalive) {
    order_source_t order_source;
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        try {
            /* Set our initial state to `secondary_need_primary`. */
            {
                region_map_t<binary_blob_t> blobs;
                read_token_t token;
                store->new_read_token(&token);
                store->do_get_metainfo(
                    order_source.check_in("secondary_t").with_read_mode(), &token,
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
            guarantee(primary_bcard.broadcaster.branch_id == branch);

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
                base_path,
                io_backender,
                mailbox_manager,
                server_id,
                backfill_throttler,
                primary_bcard.broadcaster,
                branch_history_manager,
                store,
                primary_bcard.replier,
                perfmons,
                &stop_signal,
                &order_source,
                nullptr); /* RSI(raft): Hook up backfill progress again */

            replier_t replier(
                &listener,
                mailbox_manager,
                branch_history_manager);

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

