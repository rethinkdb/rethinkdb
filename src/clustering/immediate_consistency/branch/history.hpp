#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"

template<class protocol_t>
bool version_is_ancestor(
        boost::shared_ptr<metadata_read_view_t<namespace_branch_metadata_t<protocol_t> > > namespace_metadata,
        version_t ancestor,
        version_t descendent,
        typename protocol_t::region_t relevant_region)
{
    if (region_is_empty(relevant_region)) {
        return true;
    } else if (ancestor.branch.is_nil()) {
        /* Everything is descended from the root pseudobranch. */
        return true;
    } else if (descendent.branch.is_nil()) {
        /* The root psuedobranch is descended from nothing but itself. */
        return false;
    } else if (ancestor.branch == descendent.branch) {
        return ancestor.timestamp <= descendent.timestamp;
    } else {
        rassert(namespace_metadata->get().branches.count(descendent.branch) != 0);
        branch_metadata_t<protocol_t> descendent_branch_metadata = namespace_metadata->get().branches[descendent.branch];
        rassert(region_is_superset(descendent_branch_metadata.region, relevant_region));
        rassert(descendent.timestamp >= descendent_branch_metadata.initial_timestamp);
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> > origin_pairs =
            descendent_branch_metadata.origin.mask(relevant_region).get_as_pairs();
        rassert(!origin_pairs.empty());
        for (int i = 0; i < (int)origin_pairs.size(); i++) {
            rassert(!region_is_empty(origin_pairs[i].first));
            rassert(version_is_ancestor(namespace_metadata, origin_pairs[i].second.earliest, origin_pairs[i].second.latest, origin_pairs[i].first));
            if (!version_is_ancestor(namespace_metadata, ancestor, origin_pairs[i].second.earliest, origin_pairs[i].first)) {
                return false;
            }
        }
        return true;
    }
}

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP__ */
