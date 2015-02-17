// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_

#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/semilattice/view.hpp"

template <class> class clone_ptr_t;
template <class> class watchable_t;

/* `backfillee()` contacts the given backfiller and requests a backfill from it.
It takes responsibility for updating the metainfo. */

void backfillee(
        mailbox_manager_t *mailbox_manager,

        branch_history_manager_t *branch_history_manager,

        store_view_t *svs,

        /* The region to backfill. Keys outside of this region will be left
        as they were. */
        region_t region,

        /* The backfiller to backfill from. */
        const backfiller_business_card_t &backfiller_metadata,

        signal_t *interruptor,

        /* If this is non-null, `backfillee()` will periodically update it with the
        latest progress fraction in the range [0, 1]. */
        double *progress_out)
    THROWS_ONLY(interrupted_exc_t);

/* Convenience function for extracting the backfiller's peer_id_t from the
 * backfiller metadata. Returns a nil ID if no peer id exists. */
peer_id_t extract_backfiller_peer_id(
        const boost::optional<boost::optional<backfiller_business_card_t> >
        &backfiller_metadata);


#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLEE_HPP_ */
