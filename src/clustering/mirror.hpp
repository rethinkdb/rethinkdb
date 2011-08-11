#ifndef __CLUSTERING_MIRROR_HPP__
#define __CLUSTERING_MIRROR_HPP__

#include "clustering/branch_metadata.hpp"   // for `version_t`

struct resource_disrupted_exc_t : public std::exception {
    peer_failed_exc_t(const char *m) : msg(m) { }
    const char *what() const throw () {
        return m;
    }
private:
    const char *msg;
};

struct interrupted_exc_t : public std::exception {
    const char *what() const throw () {
        return "interrupted";
    }
};

template<class protocol_t>
void mirror(
    typename protocol_t::store_t *store,
    mailbox_cluster_t *cluster,
    metadata_view_t<branch_metadata_t<protocol_t> > *branch_metadata,
    mirror_id_t backfill_provider,
    signal_t *interruptor
    );

#include "clustering/mirror.tcc"

#endif /* __CLUSTERING_MIRROR_HPP__ */
