#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "rpc/semilattice/view.hpp"

template <class> class multistore_ptr_t;

template<class protocol_t>
void backfillee(
        mailbox_manager_t *mailbox_manager,
        UNUSED boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
        multistore_ptr_t<protocol_t> *svs,
        typename protocol_t::region_t region,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_metadata,
        backfill_session_id_t backfill_session_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_ */
