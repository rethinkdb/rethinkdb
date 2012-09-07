#include "clustering/reactor/reactor.hpp"

#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/direct_reader.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"

template <class protocol_t>
bool reactor_t<protocol_t>::find_broadcaster_in_directory(const typename protocol_t::region_t &region, const blueprint_t<protocol_t> &bp, const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &_reactor_directory,
                                                          clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > *broadcaster_out) {
    /* This helps us detect if we have multiple broadcasters. */
    bool found_broadcaster = false;

    typedef reactor_business_card_t<protocol_t> rb_t;
    typedef std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<rb_t> > > reactor_directory_t;

    for (typename blueprint_t<protocol_t>::role_map_t::const_iterator it  = bp.peers_roles.begin();
                                                                      it != bp.peers_roles.end();
                                                                      it++) {
        typename reactor_directory_t::const_iterator p_it = _reactor_directory.find(it->first);
        if (p_it != _reactor_directory.end() && p_it->second) {
            for (typename rb_t::activity_map_t::const_iterator a_it  = p_it->second->internal.activities.begin();
                                                               a_it != p_it->second->internal.activities.end();
                                                               a_it++) {
                if (a_it->second.region == region) {
                    if (boost::get<typename rb_t::primary_t>(&a_it->second.activity)) {
                        if (!found_broadcaster) {
                            //This is the first viable broadcaster we've found
                            //so we set the output variable.
                            *broadcaster_out = get_directory_entry_view<typename rb_t::primary_t>(it->first, a_it->first)->
                                subview(&reactor_t<protocol_t>::extract_broadcaster_from_reactor_business_card_primary);

                            found_broadcaster = true;
                        } else {
                            return false;
                        }
                    }
                }
            }
        } else {
            //this peer isn't connected or lacks a reactor, that's fine we can
            //just look else where for the broadcaster
        }
    }

    return found_broadcaster;
}

template<class protocol_t>
boost::optional<boost::optional<replier_business_card_t<protocol_t> > > extract_replier_from_reactor_business_card_primary(
        const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::primary_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >();
    }
    if (!bcard.get()) {
        return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >(
            boost::optional<replier_business_card_t<protocol_t> >());
    }
    if (!bcard.get().get().replier) {
        return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >(
            boost::optional<replier_business_card_t<protocol_t> >());
    }
    return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >(
        boost::optional<replier_business_card_t<protocol_t> >(bcard.get().get().replier.get()));
}

template<class protocol_t>
boost::optional<boost::optional<replier_business_card_t<protocol_t> > > extract_replier_from_reactor_business_card_secondary(
        const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t> > &bcard) {
    if (!bcard) {
        return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >();
    }
    if (!bcard.get()) {
        return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >(
            boost::optional<replier_business_card_t<protocol_t> >());
    }
    return boost::optional<boost::optional<replier_business_card_t<protocol_t> > >(
        boost::optional<replier_business_card_t<protocol_t> >(bcard.get().get().replier));
}

template <class protocol_t>
bool reactor_t<protocol_t>::find_replier_in_directory(const typename protocol_t::region_t &region, const branch_id_t &b_id, const blueprint_t<protocol_t> &bp, const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &_reactor_directory,
                                                         clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > *replier_out, peer_id_t *peer_id_out, reactor_activity_id_t *activity_id_out) {
    std::vector<clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > > backfill_candidates;
    std::vector<peer_id_t> peer_ids;
    std::vector<reactor_activity_id_t> activity_ids;

    typedef reactor_business_card_t<protocol_t> rb_t;
    typedef std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<rb_t> > > reactor_directory_t;

    for (typename blueprint_t<protocol_t>::role_map_t::const_iterator it = bp.peers_roles.begin();
         it != bp.peers_roles.end();
         ++it) {
        typename reactor_directory_t::const_iterator p_it = _reactor_directory.find(it->first);
        if (p_it != _reactor_directory.end() && p_it->second) {
            for (typename rb_t::activity_map_t::const_iterator a_it = p_it->second->internal.activities.begin();
                 a_it != p_it->second->internal.activities.end();
                 ++a_it) {
                if (a_it->second.region == region) {
                    if (const typename rb_t::primary_t *primary = boost::get<typename rb_t::primary_t>(&a_it->second.activity)) {
                        if (primary->replier && primary->broadcaster.branch_id == b_id) {
                            backfill_candidates.push_back(get_directory_entry_view<typename rb_t::primary_t>(it->first, a_it->first)->
                                subview(&extract_replier_from_reactor_business_card_primary<protocol_t>));
                            peer_ids.push_back(it->first);
                            activity_ids.push_back(a_it->first);
                        }
                    } else if (const typename rb_t::secondary_up_to_date_t *secondary = boost::get<typename rb_t::secondary_up_to_date_t>(&a_it->second.activity)) {
                        if (secondary->branch_id == b_id) {
                            backfill_candidates.push_back(get_directory_entry_view<typename rb_t::secondary_up_to_date_t>(it->first, a_it->first)->
                                subview(&extract_replier_from_reactor_business_card_secondary<protocol_t>));
                                peer_ids.push_back(it->first);
                                activity_ids.push_back(a_it->first);
                        }
                    }
                }
            }
        } else {
            //this peer isn't connected or lacks a reactor, that's fine we can
            //just look else where for the broadcaster
        }
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

template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &blueprint, signal_t *interruptor) THROWS_NOTHING {
    debugf_print("be_secondary on region", region);
    try {
        order_source_t order_source(svs->home_thread());  // TODO: order_token_t::ignore

        /* Tell everyone that we're backfilling so that we can get up to
         * date. */
        directory_entry_t directory_entry(this, region);
        while (true) {
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster;
            clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > location_to_backfill_from;
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
                 * allows other secondaries to preemptively backfill before the
                 * primary is up. */
                backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history_manager, svs);

                /* Tell everyone in the cluster what state we're in. */
                scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> read_token;
                svs->new_read_token(&read_token);

                region_map_t<protocol_t, binary_blob_t> metainfo_blob;
                svs->do_get_metainfo(order_source.check_in("reactor_t::be_secondary").with_read_mode(), &read_token, &ct_interruptor, &metainfo_blob);

                on_thread_t th2(this->home_thread());

                typename reactor_business_card_t<protocol_t>::secondary_without_primary_t
                    activity(to_version_range_map(metainfo_blob), backfiller.get_business_card());

                directory_entry.set(activity);

                /* Wait until we can find a primary for our region. */
                run_until_satisfied_2(
                    reactor_directory,
                    blueprint,
                    boost::bind(&reactor_t<protocol_t>::find_broadcaster_in_directory, this, region, _2, _1, &broadcaster),
                    interruptor);

                /* We need to save this to a local variable because there may be a
                 * race condition should the broadcaster go down. */
                boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > broadcaster_business_card = broadcaster->get();
                if (!broadcaster_business_card || !*broadcaster_business_card) {
                    /* Either the peer went down immediately after we found it
                     * or the peer is still connected but the broadcaster on
                     * its machine was destroyed. Either way we need to go
                     * through the loop again. */
                    continue;
                }
                branch_id = broadcaster_business_card.get().get().branch_id;

                run_until_satisfied_2(
                    reactor_directory,
                    blueprint,
                    boost::bind(&reactor_t<protocol_t>::find_replier_in_directory, this, region, branch_id, _2, _1, &location_to_backfill_from, &peer_id, &activity_id),
                    interruptor);

                /* Note, the backfiller goes out of scope here, that's because
                 * we're about to start backfilling from someone else and thus
                 * can't offer backfills. (This is fine because obviously
                 * there's someone else to backfill from).*/
            }

            try {
                /* Generate a session id to do our backfill. */
                backfill_session_id_t backfill_session_id = generate_uuid();

                reactor_business_card_details::backfill_location_t backfill_location(backfill_session_id,
                                                                                     peer_id,
                                                                                     activity_id);

                /* We have found a broadcaster (a master to track) so now we
                 * need to backfill to get up to date. */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_backfilling_t(backfill_location));

                cross_thread_signal_t ct_interruptor(interruptor, svs->home_thread());
                cross_thread_watchable_variable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > ct_broadcaster(broadcaster, svs->home_thread());
                cross_thread_watchable_variable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > ct_location_to_backfill_from(location_to_backfill_from, svs->home_thread());
                on_thread_t th(svs->home_thread());

                // TODO: Don't use local stack variable for name.
                std::string region_name = strprintf("be_secondary_%p", &region);
                perfmon_collection_t region_perfmon_collection;
                perfmon_membership_t region_perfmon_membership(&regions_perfmon_collection, &region_perfmon_collection, region_name);

                /* This causes backfilling to happen. Once this constructor returns we are up to date. */
                listener_t<protocol_t> listener(io_backender, mailbox_manager, ct_broadcaster.get_watchable(), branch_history_manager, svs, ct_location_to_backfill_from.get_watchable(), backfill_session_id, &regions_perfmon_collection, &ct_interruptor, &order_source);

                /* This gives others access to our services, in particular once
                 * this constructor returns people can send us queries and use
                 * us for backfills. */
                replier_t<protocol_t> replier(&listener, mailbox_manager, branch_history_manager);

                direct_reader_t<protocol_t> direct_reader(mailbox_manager, svs);

                cross_thread_signal_t ct_broadcaster_lost_signal(listener.get_broadcaster_lost_signal(), this->home_thread());
                on_thread_t th2(this->home_thread());

                /* Make the directory reflect the new role that we are filling.
                 * (Being a secondary). */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t(branch_id, replier.get_business_card(), direct_reader.get_business_card()));

                /* Wait for something to change. */
                wait_interruptible(&ct_broadcaster_lost_signal, interruptor);
            } catch (typename listener_t<protocol_t>::backfiller_lost_exc_t) {
                /* We lost the replier which means we should retry, just
                 * going back to the top of the while loop accomplishes this.
                 * */
            } catch (typename listener_t<protocol_t>::broadcaster_lost_exc_t) {
                /* We didn't find the broadcaster which means we should retry,
                 * same deal as above. */
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"

template class reactor_t<mock::dummy_protocol_t>;
template class reactor_t<memcached_protocol_t>;
template class reactor_t<rdb_protocol_t>;

