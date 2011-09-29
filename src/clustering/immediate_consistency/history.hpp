#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_HISTORY_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_HISTORY_HPP__

#include "clustering/metadata.hpp"

template<class protocol_t>
bool version_is_ancestor(
        boost::shared_ptr<metadata_read_view_t<namespace_metadata_t<protocol_t> > > namespace_metadata,
        version_t ancestor,
        version_t descendent,
        typename protocol_t::region_t relevant_region) {
    crash("not implemented");
}

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_HISTORY_HPP__ */
