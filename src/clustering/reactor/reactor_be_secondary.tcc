#ifndef __CLUSTERING_REACTOR_REACTOR_BE_SECONDARY_TCC__
#define __CLUSTERING_REACTOR_REACTOR_BE_SECONDARY_TCC__

template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const blueprint_t<protocol_t> &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        /* Tell everyone that we're backfilling so that we can get up to
         * date. */
        directory_entry_t directory_entry(this, region);
        while (true) {
            boost::optional<clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster =
                find_broadcaster_in_directory(region);
            if (!broadcaster) {
                /* TODO?: Backfill from most up-to-date peer? */
                backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);
                typename reactor_business_card_t<protocol_t>::secondary_without_primary_t activity(store->get_metadata(interruptor), backfiller.get_business_card());
                directory_entry.set(activity);
                broadcaster = wait_for_broadcaster_to_appear_in_directory(region, interruptor);
            }

            /* We need to save this to a local variable because there may be a
             * race condition should the broadcaster go down. */
            boost::optional<broadcaster_business_card_t<protocol_t> > broadcaster_business_card = broadcaster.get()->get_value();
            if (!broadcaster_business_card) {
                /* The broadcaster went down immediately after we found it so
                 * we need to go through the loop again. */
                continue;
            }
            branch_id_t branch_id = broadcaster_business_card.get().branch_id;

            try {
                /* TODO we need to find a backfiller for listener_t */

                /* We have found a broadcaster (a master to track) so now we
                 * need to backfill to get up to date. */
                directory_entry.set(typename reactor_business_card_t<protocol_t>::secondary_backfilling_t());

                /* This causes backfilling to happen. Once this constructor returns we are up to date. */
                listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, interruptor);

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
