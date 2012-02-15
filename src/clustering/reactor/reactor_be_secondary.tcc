#ifndef __CLUSTERING_REACTOR_REACTOR_BE_SECONDARY_TCC__
#define __CLUSTERING_REACTOR_REACTOR_BE_SECONDARY_TCC__

template <class protocol_t>
bool reactor_t<protocol_t>::find_broadcaster_in_directory(const typename protocol_t::region_t &region, const blueprint_t<protocol_t> &bp, const std::map<peer_id_t, boost::optional<reactor_business_card_t<protocol_t> > > &reactor_directory, 
                                                          clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > *broadcaster_out) {
    /* This helps us detect if we have multiple broadcasters. */
    bool found_broadcaster = false;

    typedef reactor_business_card_t<protocol_t> rb_t;
    typedef std::map<peer_id_t, boost::optional<rb_t> > reactor_directory_t;

    for (typename blueprint<protocol_t>::role_map_t::const_iterator it  = bp.peers_roles.begin();
                                                                    it != bp.peers_roles.end();
                                                                    it++) {
        typename reactor_directory_t::const_iterator p_it = reactor_directory.find(it->first);
        if (p_it != reactor_directory.end() && p_it->second) {
            for (typename rb_t::activity_map_t::const_iterator a_it  = p_it->second->activities.begin();
                                                               a_it != p_it->second->activities.end();
                                                               a_it++) {
                if (a_it->second.first == region) {
                    if (boost::get<typename rb_t::primary_t>(&a_it->second.second)) {
                        if (!found_broadcaster) {
                            //This is the first viable broadcaster we've found
                            //so we set the output variable.
                            *broadcaster_out = get_directory_entry_view<typename rb_t::primary_t>(it->first, a_it->first)->
                                subview(optional_monad_lens<broadcaster_business_card_t<protocol_t>, typename rb_t::primary_t>(
                                        field_lens(&rb_t::primary_t::broadcaster)));

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

template <class protocol_t>
bool find_backfiller_in_directory(const typename protocol_t::region_t &region, const branch_id_t &b_id, const blueprint_t<protocol_t> &bp, const std::map<peer_id_t, boost::optional<reactor_business_card_t<protocol_t> > > &reactor_directory, 
                                  clone_ptr_t<directory_single_rview_t<boost::optional<backfiller_business_card_t<protocol_t> > > > *backfiller_out) {
    std::vector<clone_ptr_t<directory_single_rview_t<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfill_candidates;

    typedef reactor_business_card_t<protocol_t> rb_t;
    typedef std::map<peer_id_t, boost::optional<rb_t> > reactor_directory_t;

    for (typename blueprint<protocol_t>::role_map_t::const_iterator it  = bp.peers_roles.begin();
                                                                    it != bp.peers_roles.end();
                                                                    it++) {
        typename reactor_directory_t::const_iterator p_it = reactor_directory.find(it->first);
        if (p_it != reactor_directory.end() && p_it->second) {
            for (typename rb_t::activity_map_t::const_iterator a_it  = p_it->second->activities.begin();
                                                               a_it != p_it->second->activities.end();
                                                               a_it++) {
                if (a_it->second.first == region) {
                    if (typename rb_t::primary_t *primary = boost::get<typename rb_t::primary_t>(&a_it->second.second)) {
                        if (primary->backfiller && primary->broadcaster.branch_id == b_id) {
                            backfill_candidates.push_back(get_directory_entry_view<typename rb_t::primary_t>(it->first, a_it->first)->
                                    subview(compose_lens<boost::optional<backfiller_business_card_t<protocol_t> >, 
                                                         boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > >, 
                                                         boost::optional<rb_t> > (
                                                             optional_collapser_lens<backfiller_business_card_t<protocol_t> >(),
                                                             optional_monad_lens<backfiller_business_card_t<protocol_t>, typename rb_t::primary_t>(
                                                                 field_lens(&rb_t::primary_t::backfiller)
                                                             )
                                                         )
                                    )
                            );
                        }
                    } else if (typename rb_t::secondary_up_to_date_t *secondary = boost::get<typename rb_t::secondary_up_to_date_t>(&a_it->second.second)) {
                        if (secondary->branch_id == b_id) {
                            backfill_candidates.push_back(get_directory_entry_view<typename rb_t::secondary_up_to_date_t>(it->first, a_it->first)->
                                                             subview(optional_monad_lens<broadcaster_business_card_t<protocol_t>, typename rb_t::secondary_up_to_date_t>(
                                                                field_lens(&rb_t::secondary_up_to_date_t::broadcaster))));
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
        *backfiller_out = backfill_candidates[rand() % backfill_candidates.size()];
        return true;
    }
}


template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const blueprint_t<protocol_t> &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        /* Tell everyone that we're backfilling so that we can get up to
         * date. */
        directory_entry_t directory_entry(this, region);
        while (true) {
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster;
            branch_id_t branch_id;

            {
                /* First we construct a backfiller which offers backfills to
                 * the rest of the cluster, this is necessary because if a new
                 * primary is coming up it may need data from us before it can
                 * become the primary. 
                 * Also this is potentially a performance boost because it
                 * allows other secondaries to preemptively backfill before the
                 * primary is up. */
                backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);

                /* Tell everyone in the cluster what state we're in. */
                typename reactor_business_card_t<protocol_t>::secondary_without_primary_t activity(store->get_metadata(interruptor), backfiller.get_business_card());
                directory_entry.set(activity);

                /* Wait until we can find a primary for our region. */
                directory_echo_access.get_internal_view()->run_until_satisfied(boost::bind(&reactor_t<protocol_t>::find_broadcaster_in_directory, this, region, blueprint, _1, &broadcaster), interruptor);

                /* We need to save this to a local variable because there may be a
                 * race condition should the broadcaster go down. */
                boost::optional<broadcaster_business_card_t<protocol_t> > broadcaster_business_card = broadcaster.get()->get_value();
                if (!broadcaster_business_card) {
                    /* The broadcaster went down immediately after we found it so
                     * we need to go through the loop again. */
                    continue;
                }
                branch_id = broadcaster_business_card.get().branch_id;

                clone_ptr_t<directory_single_rview_t<boost::optional<backfiller_business_card_t<protocol_t> > > > backfiller;
                directory_echo_access.get_internal_view()->run_until_satisfied(boost::bind(&reactor_t<protocol_t>::find_backfiller_in_directory, this, region, branch_id, blueprint, _1, &backfiller), interruptor);

                /* Note, the backfiller goes out of scope here, that's because
                 * we're about to start backfilling from someone else and thus
                 * can't offer backfills. (This is fine because obviously
                 * there's someone else to backfill from).*/
            }

            try {

                /* We have found a broadcaster (a master to track) so now we
                 * need to backfill to get up to date. */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_backfilling_t());

                /* This causes backfilling to happen. Once this constructor returns we are up to date. */
                listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, backfiller, interruptor);

                /* This gives others access to our services, in particular once
                 * this constructor returns people can send us queries and use
                 * us for backfills. */
                replier_t<protocol_t> replier(&listener);

                /* Make the directory reflect the new role that we are filling.
                 * (Being a secondary). */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t(branch_id, replier.get_business_card()));

                /* Wait for something to change. */
                wait_interruptible(listener.get_outdated_signal(), interruptor);
            } catch (typename listener_t<protocol_t>::backfiller_lost_exc_t) {
                /* We lost the backfiller which means we should retry, just
                 * going back to the top of the while loop accomplishes this.
                 * */
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

#endif
