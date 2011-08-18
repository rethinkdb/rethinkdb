#ifndef __CLUSTERING_MIRROR_HPP__
#define __CLUSTERING_MIRROR_HPP__

template<class protocol_t>
void mirror(
    typename protocol_t::store_t *store,
    mailbox_cluster_t *cluster,
    metadata_readwrite_view_t<branch_metadata_t<protocol_t> > *branch_metadata,
    mirror_id_t backfill_provider,
    signal_t *interruptor
    );

#include "clustering/mirror.tcc"

#endif /* __CLUSTERING_MIRROR_HPP__ */
