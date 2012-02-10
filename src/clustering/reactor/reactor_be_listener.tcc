#ifndef __CLUSTERING_REACTOR_REACTOR_BE_LISTENER_TCC__
#define __CLUSTERING_REACTOR_REACTOR_BE_LISTENER_TCC__

template<class protocol_t>
void reactor_t<protocol_t>::be_listener(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const blueprint_t<protocol_t> &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);
        while (true) {
            /* Tell everyone else that we're waiting for a broadcaster to
             * appear so that we can backfill. */
            directory_entry.set(typename reactor_business_card_t<protocal_t>::listener_without_primary_t());

            /* TODO we need to find a backfiller for listener_t */

            /* TODO: Backfill from most up-to-date peer if broadcaster is not
            found? */

            /* This actually finds the broadcaster that we will backfill from. */

            //TODO implement the function wait_for_broadcaster_to_appear_in_directory
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster =
                wait_for_broadcaster_to_appear_in_directory(interruptor);

            /* We've found a broadcaster tell everyone else that we're about to
             * begin backfilling. */
            directory_entry.set(typename reactor_business_card_t<protocal_t>::listener_backfilling_t());

            try {
                /* Construct a listener to receive the backfill. */
                listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, interruptor);

                /* We've completed backfilling, tell everyone that we're an up
                 * to date listener. (the architect will probably react to this
                 * by giving us a different role to fill .*/
                directory_entry.set(typename reactor_business_card_t<protocal_t>::listener_up_to_date_t());

                /* Wait for something to change (probably us getting a new role
                 * to fill). */
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
