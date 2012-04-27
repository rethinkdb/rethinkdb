#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "rpc/semilattice/view.hpp"

/* TODO: What if the backfill chunks on the network get reordered in transit?
Even if the `protocol_t` can tolerate backfill chunks being reordered, it's
clearly not OK for the end-of-backfill message to be processed before the last
of the backfill chunks are. Consider using a FIFO enforcer or something. */

template<class protocol_t>
void on_receive_backfill_chunk(
        store_view_t<protocol_t> *store,
        signal_t *dont_go_until,
        typename protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor,
        UNUSED auto_drainer_t::lock_t keepalive)
    THROWS_NOTHING;

template<class protocol_t>
void backfillee(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
        store_view_t<protocol_t> *store,
        typename protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_ */
