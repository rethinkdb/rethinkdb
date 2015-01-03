// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/reactor/reactor.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/direct_reader.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "config/args.hpp"
#include "store_view.hpp"
#include "time.hpp"

bool reactor_t::find_broadcaster_in_directory(
        const region_t &region,
        const blueprint_t &bp,
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > > *broadcaster_out) {
    int num_broadcasters = 0;

    typedef reactor_business_card_t rb_t;

    for (auto it = bp.peers_roles.begin(); it != bp.peers_roles.end(); ++it) {
        directory->read_key(it->first,
                [&](const cow_ptr_t<reactor_business_card_t> *bcard) {
            if (bcard == nullptr) {
                /* This peer isn't connected or lacks a reactor. That's OK; we just look
                elsewhere for the broadcaster. */
                return;
            }
            for (auto a_it = (*bcard)->activities.begin();
                    a_it != (*bcard)->activities.end();
                    ++a_it) {
                if (a_it->second.region == region) {
                    if (boost::get<rb_t::primary_t>(&a_it->second.activity)) {
                        ++num_broadcasters;
                        *broadcaster_out =
                            get_directory_entry_view<rb_t::primary_t>(
                                it->first, a_it->first)->
                            subview(&reactor_t::
                                extract_broadcaster_from_reactor_business_card_primary);
                    }
                }
            }
        });
    }

    return (num_broadcasters == 1);
}

boost::optional<boost::optional<replier_business_card_t> > extract_replier_from_reactor_business_card_primary(
        const boost::optional<boost::optional<reactor_business_card_t::primary_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<replier_business_card_t> >();
    }
    if (!bcard.get()) {
        return boost::optional<boost::optional<replier_business_card_t> >(
            boost::optional<replier_business_card_t>());
    }
    if (!bcard.get().get().replier) {
        return boost::optional<boost::optional<replier_business_card_t> >(
            boost::optional<replier_business_card_t>());
    }
    return boost::optional<boost::optional<replier_business_card_t> >(
        boost::optional<replier_business_card_t>(bcard.get().get().replier.get()));
}

boost::optional<boost::optional<replier_business_card_t> > extract_replier_from_reactor_business_card_secondary(
        const boost::optional<boost::optional<reactor_business_card_t::secondary_up_to_date_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<replier_business_card_t> >();
    }
    if (!bcard.get()) {
        return boost::optional<boost::optional<replier_business_card_t> >(
            boost::optional<replier_business_card_t>());
    }
    return boost::optional<boost::optional<replier_business_card_t> >(
        boost::optional<replier_business_card_t>(bcard.get().get().replier));
}

bool reactor_t::find_replier_in_directory(
        const region_t &region,
        const branch_id_t &b_id,
        const blueprint_t &bp,
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t> > > > *replier_out,
        peer_id_t *peer_id_out,
        reactor_activity_id_t *activity_id_out) {
    std::vector<clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t> > > > > backfill_candidates;
    std::vector<peer_id_t> peer_ids;
    std::vector<reactor_activity_id_t> activity_ids;

    typedef reactor_business_card_t rb_t;

    for (auto it = bp.peers_roles.begin(); it != bp.peers_roles.end(); ++it) {
        directory->read_key(it->first,
                [&](const cow_ptr_t<reactor_business_card_t> *bcard) {
            if (bcard == nullptr) {
                /* This peer isn't connected or lacks a reactor. That's OK; we just look
                elsewhere for the replier. */
                return;
            }
            for (auto a_it = (*bcard)->activities.begin();
                    a_it != (*bcard)->activities.end();
                    ++a_it) {
                if (a_it->second.region == region) {
                    if (const rb_t::primary_t *primary = boost::get<rb_t::primary_t>(&a_it->second.activity)) {
                        if (primary->replier && primary->broadcaster.branch_id == b_id) {
                            backfill_candidates.push_back(get_directory_entry_view<rb_t::primary_t>(it->first, a_it->first)->
                                subview(&extract_replier_from_reactor_business_card_primary));
                            peer_ids.push_back(it->first);
                            activity_ids.push_back(a_it->first);
                        }
                    } else if (const rb_t::secondary_up_to_date_t *secondary = boost::get<rb_t::secondary_up_to_date_t>(&a_it->second.activity)) {
                        if (secondary->branch_id == b_id) {
                            backfill_candidates.push_back(get_directory_entry_view<rb_t::secondary_up_to_date_t>(it->first, a_it->first)->
                                subview(&extract_replier_from_reactor_business_card_secondary));
                                peer_ids.push_back(it->first);
                                activity_ids.push_back(a_it->first);
                        }
                    }
                }
            }
        });
    }

    if (backfill_candidates.empty()) {
        return false;
    } else {
        int selection = randint(backfill_candidates.size());
        *replier_out = backfill_candidates[selection];
        *peer_id_out = peer_ids[selection];
        *activity_id_out = activity_ids[selection];
        return true;
    }
}

void reactor_t::be_secondary(region_t region, store_view_t *svs, const clone_ptr_t<watchable_t<blueprint_t> > &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        order_source_t order_source(svs->home_thread());  // TODO: order_token_t::ignore

        /* Tell everyone that we're backfilling so that we can get up to
         * date. */
        directory_entry_t directory_entry(this, region);
        while (true) {

            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > > broadcaster;
            clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t> > > > location_to_backfill_from;
            branch_id_t branch_id;
            peer_id_t peer_id;
            reactor_activity_id_t activity_id;

            {
                cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
                on_thread_t th(svs->home_thread());

                /* First we construct a backfiller which offers backfills to
                 * the rest of the cluster, this is necessary because if a new
                 * primary is coming up it may need data from us before it can
                 * become the primary.
                 * Also this is potentially a performance boost because it
                 * allows other secondary replicas to preemptively backfill before the
                 * primary replica is up. */
                backfiller_t backfiller(mailbox_manager, branch_history_manager, svs);

                /* Tell everyone in the cluster what state we're in. */
                read_token_t read_token;
                svs->new_read_token(&read_token);

                region_map_t<binary_blob_t> metainfo_blob;
                svs->do_get_metainfo(order_source.check_in("reactor_t::be_secondary").with_read_mode(), &read_token, &ct_interruptor, &metainfo_blob);

                direct_reader_t direct_reader(mailbox_manager, svs);

                on_thread_t th2(this->home_thread());

                branch_history_t branch_history;
                branch_history_manager->export_branch_history(to_version_range_map(metainfo_blob), &branch_history);

                reactor_business_card_t::secondary_without_primary_t
                    activity(to_version_range_map(metainfo_blob), backfiller.get_business_card(), direct_reader.get_business_card(), branch_history);

                directory_entry.set(activity);

                /* Wait until we can find a primary for our region. */
                run_until_satisfied_2(
                    directory_echo_mirror.get_internal(),
                    blueprint,
                    boost::bind(&reactor_t::find_broadcaster_in_directory, this, region, _2, _1, &broadcaster),
                    interruptor,
                    REACTOR_RUN_UNTIL_SATISFIED_NAP);

                /* We need to save this to a local variable because there may be a
                 * race condition should the broadcaster go down. */
                boost::optional<boost::optional<broadcaster_business_card_t> > broadcaster_business_card = broadcaster->get();
                if (!broadcaster_business_card || !*broadcaster_business_card) {
                    /* Either the peer went down immediately after we found it
                     * or the peer is still connected but the broadcaster on
                     * its server was destroyed. Either way we need to go
                     * through the loop again. */
                    continue;
                }
                branch_id = broadcaster_business_card.get().get().branch_id;

                run_until_satisfied_2(
                    directory_echo_mirror.get_internal(),
                    blueprint,
                    boost::bind(&reactor_t::find_replier_in_directory, this, region, branch_id, _2, _1, &location_to_backfill_from, &peer_id, &activity_id),
                    interruptor,
                    REACTOR_RUN_UNTIL_SATISFIED_NAP);

                /* Note, the backfiller goes out of scope here, that's because
                 * we're about to start backfilling from someone else and thus
                 * can't offer backfills. (This is fine because obviously
                 * there's someone else to backfill from).*/
            }

            try {
                /* We have found a broadcaster (a primary replica to track) so now we
                 * need to backfill to get up to date. */
                directory_entry.set(reactor_business_card_t::secondary_backfilling_t());

                cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
                cross_thread_watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > ct_broadcaster(broadcaster, svs->home_thread());
                cross_thread_watchable_variable_t<boost::optional<boost::optional<replier_business_card_t> > > ct_location_to_backfill_from(location_to_backfill_from, svs->home_thread());
                on_thread_t th(svs->home_thread());

                map_insertion_sentry_t<region_t, reactor_progress_report_t>
                    progress_tracker_on_svs_thread(
                        progress_map.get(),
                        region,
                        reactor_progress_report_t{false, current_microtime(), { }});
                progress_tracker_on_svs_thread.get_value()->backfills.push_back(
                    std::make_pair(peer_id, 0.0));

                // TODO: Don't use local stack variable for name.
                std::string region_name = strprintf("be_secondary_%p", &region);
                perfmon_collection_t region_perfmon_collection;
                perfmon_membership_t region_perfmon_membership(&regions_perfmon_collection, &region_perfmon_collection, region_name);

                /* This causes backfilling to happen. Once this constructor returns we are up to date. */
                listener_t listener(
                    base_path,
                    io_backender,
                    mailbox_manager,
                    server_id,
                    backfill_throttler,
                    ct_broadcaster.get_watchable(),
                    branch_history_manager,
                    svs,
                    ct_location_to_backfill_from.get_watchable(),
                    &region_perfmon_collection,
                    &ct_interruptor,
                    &order_source,
                    &progress_tracker_on_svs_thread.get_value()->backfills.back().second
                    );

                progress_tracker_on_svs_thread.get_value()->is_ready = true;

                /* This gives others access to our services, in particular once
                 * this constructor returns people can send us queries and use
                 * us for backfills. */
                replier_t replier(&listener, mailbox_manager, branch_history_manager);

                direct_reader_t direct_reader(mailbox_manager, svs);

                cross_thread_signal_t ct_broadcaster_lost_signal(listener.get_broadcaster_lost_signal(), this->home_thread());
                on_thread_t th2(this->home_thread());

                /* Make the directory reflect the new role that we are filling.
                 * (Being a secondary replica). */
                directory_entry.set(reactor_business_card_t::secondary_up_to_date_t(branch_id, replier.get_business_card(), direct_reader.get_business_card()));

                /* Wait for something to change. */
                wait_interruptible(&ct_broadcaster_lost_signal, interruptor);
            } catch (const listener_t::backfiller_lost_exc_t &) {
                /* We lost the replier which means we should retry, just
                 * going back to the top of the while loop accomplishes this.
                 * */
            } catch (const listener_t::broadcaster_lost_exc_t &) {
                /* We didn't find the broadcaster which means we should retry,
                 * same deal as above. */
            }
        }
    } catch (const interrupted_exc_t &) {
        /* ignore */
    }
}
