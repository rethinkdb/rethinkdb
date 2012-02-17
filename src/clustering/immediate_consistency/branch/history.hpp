#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "rpc/semilattice/view.hpp"

template<class protocol_t>
bool version_is_ancestor(
        const branch_history_t<protocol_t> &branch_history,
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
        rassert(branch_history.branches.count(descendent.branch) != 0);

        typename std::map<branch_id_t, branch_birth_certificate_t<protocol_t> >::const_iterator it =
            branch_history.branches.find(descendent.branch);
        guarantee(it != branch_history.branches.end());
        branch_birth_certificate_t<protocol_t> descendent_branch_metadata = (*it).second;

        rassert(region_is_superset(descendent_branch_metadata.region, relevant_region));
        rassert(descendent.timestamp >= descendent_branch_metadata.initial_timestamp);

        typedef region_map_t<protocol_t, version_range_t> version_map_t;
        version_map_t relevant_origin = descendent_branch_metadata.origin.mask(relevant_region);

        rassert(relevant_origin.begin() != relevant_origin.end());
        for (typename version_map_t::const_iterator it  = relevant_origin.begin();
                                                    it != relevant_origin.end();
                                                    it++) {
            rassert(!region_is_empty(it->first));
            rassert(version_is_ancestor(branch_history, it->second.earliest, it->second.latest, it->first));
            if (!version_is_ancestor(branch_history, ancestor, it->second.earliest, it->first)) {
                return false;
            }
        }
        return true;
    }
}

template <class protocol_t>
bool version_is_divergent(
        const branch_history_t<protocol_t> &bh,
        version_t v1,
        version_t v2,
        typename protocol_t::region_t relevant_region) {
    return !version_is_ancestor(bh, v1, v2, relevant_region) &&
           !version_is_ancestor(bh, v2, v1, relevant_region);
}


#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP__ */
