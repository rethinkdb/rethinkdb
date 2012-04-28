#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_

#include "clustering/immediate_consistency/branch/metadata.hpp"

template <class protocol_t>
bool version_is_ancestor(
        const branch_history_t<protocol_t> &branch_history,
        version_t ancestor,
        version_t descendent,
        typename protocol_t::region_t relevant_region);

template <class protocol_t>
bool version_is_divergent(
        const branch_history_t<protocol_t> &bh,
        version_t v1,
        version_t v2,
        typename protocol_t::region_t relevant_region);


#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_ */
