#ifndef __CLUSTERING_REACTOR_REACTOR_BE_SECONDARY_TCC__
#define __CLUSTERING_REACTOR_REACTOR_BE_SECONDARY_TCC__

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
