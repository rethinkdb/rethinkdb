// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/reactor/reactor.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/immediate_consistency/branch/backfiller.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "config/args.hpp"
#include "store_view.hpp"


/* Returns true if every peer listed as a primary replica for this shard in the
 * blueprint has activity primary_t and every peer listed as a secondary has
 * activity secondary_up_to_date_t. */
bool reactor_t::is_safe_for_us_to_be_nothing(
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        const blueprint_t &blueprint,
        const region_t &region)
{
    /* Iterator through the peers the blueprint claims we should be able to
     * see. */
    for (std::map<peer_id_t, std::map<region_t, blueprint_role_t> >::const_iterator p_it = blueprint.peers_roles.begin();
         p_it != blueprint.peers_roles.end();
         ++p_it) {
        std::map<region_t, blueprint_role_t>::const_iterator r_it =
            p_it->second.find(drop_cpu_sharding(region));
        guarantee(r_it != p_it->second.end(), "Invalid blueprint issued, different "
            "peers have different sharding schemes.\n");
        bool ok;
        directory->read_key(p_it->first,
                [&](const cow_ptr_t<reactor_business_card_t> *bcard) {
            if (bcard == nullptr) {
                ok = false;
                return;
            }
            /* Whether or not we found a directory entry for this peer */
            bool found = false;
            for (auto it = (*bcard)->activities.begin();
                    it != (*bcard)->activities.end();
                    ++it) {
                if (it->second.region == region) {
                    if (r_it->second == blueprint_role_primary) {
                        if (!boost::get<reactor_business_card_t::primary_t>(&it->second.activity)) {
                            ok = false;
                            return;
                        }
                    } else if (r_it->second == blueprint_role_secondary) {
                        if (!boost::get<reactor_business_card_t::secondary_up_to_date_t>(&it->second.activity)) {
                            ok = false;
                            return;
                        }
                    }
                    found = true;
                    break;
                }
            }
            ok = found;
        });
        if (!ok) {
            return false;
        }
    }

    return true;
}

void reactor_t::be_nothing(region_t region,
        store_view_t *svs, const clone_ptr_t<watchable_t<blueprint_t> > &blueprint,
        signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);
        bool have_data;

        {
            cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());

            on_thread_t th(svs->home_thread());

            order_source_t order_source;  // TODO: order_token_t::ignore

            read_token_t read_token;
            svs->new_read_token(&read_token);
            region_map_t<binary_blob_t> metainfo_blob;
            svs->do_get_metainfo(order_source.check_in("be_nothing").with_read_mode(),
                &read_token, &ct_interruptor, &metainfo_blob);
            region_map_t<version_range_t> metainfo = to_version_range_map(metainfo_blob);

            /* If we don't have any data at all for this region, we can skip to the last
            step. This is important because if we broadcast a `nothing_when_safe_t` or a
            `nothing_when_done_erasing_t`, the user will see it in the
            `rethinkdb.table_status` pseudo-table. */
            have_data = false;
            for (const auto &pair : metainfo) {
                if (pair.second != version_range_t(version_t::zero())) {
                    have_data = true;
                    break;
                }
            }

            /* It's safe to skip the `nothing_when_safe_t` state because when
            `reactor_t::is_safe_for_us_to_be_primary()` is looking for a backfiller, it
            will proceed as long as it can see *some* state for every other node, even if
            that state is a `nothing_t`. It won't consider us as a backfill source, but
            that's OK because we don't have any data so we would never be a better
            backfill source than the primary replica itself. */
            if (have_data) {
                /* We offer backfills while waiting for it to be safe to shutdown in case
                another peer needs a copy of the data. */
                backfiller_t backfiller(mailbox_manager, branch_history_manager, svs);

                on_thread_t th2(this->home_thread());
                branch_history_t branch_history;
                branch_history_manager->export_branch_history(metainfo, &branch_history);

                /* Tell the other peers that we are looking to shutdown and
                 * offering backfilling until we do. */
                reactor_business_card_t::nothing_when_safe_t
                    activity(metainfo, backfiller.get_business_card(), branch_history);
                directory_echo_version_t version_to_wait_on =
                    directory_entry.set(activity);

                /* Make sure everyone sees that we're trying to erase our data,
                 * it's important to do this to avoid the following race condition:
                 *
                 * Peer 1 and Peer 2 both are secondary replicas.
                 * Peer 1 gets a blueprint saying its role is nothing and peer 2's role
                 * is secondary,
                 * Peer 2 gets a blueprint saying its role is nothing and peer 1's role
                 * is secondary,
                 *
                 * since each one sees that the other is a secondary replica, they both
                 * think it's safe to shutdown and thus destroy their data leading to
                 * data loss.
                 *
                 * The below line makes sure that they will sync their new roles
                 * with one another before they begin destroying data.
                 *
                 * This makes it possible for either to proceed with deleting the
                 * data, but never both, it's also possible that neither proceeds
                 * which is okay as well.
                 */
                wait_for_directory_acks(version_to_wait_on, interruptor);

                /* Make sure we don't go down and delete the data on our server before
                before every who needs a copy has it. */
                run_until_satisfied_2(
                    directory_echo_mirror.get_internal(),
                    blueprint,
                    boost::bind(&reactor_t::is_safe_for_us_to_be_nothing,
                        this, _1, _2, region),
                    interruptor,
                    REACTOR_RUN_UNTIL_SATISFIED_NAP);
            }
        }

        /* It's OK to skip the deletion phase if we don't have any valid data for this
        range. The `nothing_when_done_erasing_t` state is only for showing the user; the
        other reactor code treats it the same as `nothing_t`. */
        if (have_data) {
            /* We now know that it's safe to shutdown so we tell the other peers
             * that we are beginning the process of erasing data. */
            directory_entry.set(reactor_business_card_t::nothing_when_done_erasing_t());

            cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
            on_thread_t th(svs->home_thread());

            /* This actually erases the data, and also updates the metainfo as it erases
            each chunk to indicate we don't have any data for that region. */
            svs->reset_data(binary_blob_t(version_range_t(version_t::zero())),
                            region, write_durability_t::HARD, &ct_interruptor);
        }

        /* Tell the other peers that we are officially nothing for this region,
         * end of story. */
        directory_entry.set(reactor_business_card_t::nothing_t());

        interruptor->wait_lazily_unordered();
    } catch (const interrupted_exc_t &) {
        /* ignore */
    }
}

