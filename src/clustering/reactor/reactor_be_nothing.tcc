#ifndef __CLUSTERING_REACTOR_REACTOR_BE_NOTHING_TCC__
#define __CLUSTERING_REACTOR_REACTOR_BE_NOTHING_TCC__

template<class protocol_t>
void reactor_t<protocol_t>::be_nothing(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const blueprint_t<protocol_t> &blueprint, signal_t *interruptor) THROWS_NOTHING {
    try {
        directory_entry_t directory_entry(this, region);

        {
            /* We offer backfills while waiting for it to be safe to shutdown
             * in case another peer needs a copy of the data */
            backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);

            /* Tell the other peers that we are looking to shutdown and
             * offering backfilling until we do. */
            typename reactor_business_card_t<protocol_t>::nothing_when_safe_t activity(store_view->get_metadata(interruptor), backfiller.get_business_card());
            directory_echo_version_t version_to_wait_on = directory_entry.set(activity);

            /* Make sure everyone sees that we're trying to erase our data,
             * it's important to do this to avoid the following race condition:
             *
             * Peer 1 and Peer 2 both are secondaries.
             * Peer 1 gets a blueprint saying its role is nothing and peer 2's is secondary,
             * Peer 2 gets a blueprint saying its role is nothing and peer 1's is secondary,
             *
             * since each one sees the other is secondary they both think it's
             * safe to shutdown and thus destroy their data leading to data
             * loss.
             *
             * The below line makes sure that they will sync their new roles
             * with one another before they begin destroying data.
             *
             * This makes it possible for either to proceed with deleting the
             * data, but never both, it's also possible that neither proceeds
             * which is okay as well. 
             */
            wait_for_directory_acks(version_to_wait_on, interruptor);

            /* TODO block until for each key: everything listed as a primary or secondary
            in the blueprint is listed as PRIMARY, SECONDARY, SECONDARY_LOST, or
            PRIMARY_SOON in the directory */
        }

        /* We now know that it's safe to shutdown so we tell the other peers
         * that we are beginning the process of erasing data. */
        directory_entry.set(typename reactor_business_card_t<protocol_t>::nothing_when_done_erasing_t());

        /* This actually erases the data. */
        store.reset_data(region, region_map_t<protocol_t>(region, version_range_t::zero()), store.new_write_token(), interruptor);

        /* Tell the other peers that we are officially nothing for this region,
         * end of story. */
        directory_entry.set(typename reactor_business_card_t<protocol_t>::nothing_t());

        interruptor->wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

#endif
