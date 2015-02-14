// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/history.hpp"

#include <stack>

#include "containers/binary_blob.hpp"

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(version_t, branch, timestamp);


bool version_is_ancestor(
        branch_history_manager_t *bhm,
        const version_t ancestor,
        version_t descendent,
        region_t relevant_region) {
    typedef region_map_t<version_t> version_map_t;
    // A stack of version maps and iterators pointing an the next element in the map to traverse.
    std::stack<std::pair<version_map_t *, version_map_t::const_iterator> > origin_stack;

    // We break from this for loop when the version is shown not to be an ancestor.
    for (;;) {
        if (region_is_empty(relevant_region)) {
            // do nothing, continue to next part of tree.
        } else if (ancestor.branch.is_nil()) {
            /* Everything is descended from the root pseudobranch. */
            // do nothing, continue to next part of tree.
        } else if (descendent.branch.is_nil()) {
            /* The root psuedobranch is descended from nothing but itself. */
            // fail!
            break;
        } else if (ancestor.branch == descendent.branch) {
            if (ancestor.timestamp <= descendent.timestamp) {
                // do nothing, continue to next part of tree.
            } else {
                // fail!
                break;
            }
        } else {
            branch_birth_certificate_t descendent_branch_metadata = bhm->get_branch(descendent.branch);

            rassert(region_is_superset(descendent_branch_metadata.region, relevant_region));
            guarantee(descendent.timestamp >= descendent_branch_metadata.initial_timestamp);

            version_map_t *relevant_origin = new version_map_t(descendent_branch_metadata.origin.mask(relevant_region));
            guarantee(relevant_origin->begin() != relevant_origin->end());

            origin_stack.push(std::make_pair(relevant_origin, relevant_origin->begin()));
        }

        if (origin_stack.empty()) {
            // We've navigated the entire tree and succeed in seeing version_is_ancestor for all
            // children.  Yay.
            return true;
        }

        version_map_t::const_iterator it = origin_stack.top().second;
        descendent = it->second;
        relevant_region = it->first;

        ++it;
        if (it == origin_stack.top().first->end()) {
            delete origin_stack.top().first;
            origin_stack.pop();
        } else {
            origin_stack.top().second = it;
        }
    }

    // We failed.  We need to clean up some memory.
    while (!origin_stack.empty()) {
        delete origin_stack.top().first;
        origin_stack.pop();
    }

    return false;
}


bool version_is_divergent(
        branch_history_manager_t *bhm,
        version_t v1,
        version_t v2,
        const region_t &relevant_region) {
    return !version_is_ancestor(bhm, v1, v2, relevant_region) &&
           !version_is_ancestor(bhm, v2, v1, relevant_region);
}


region_map_t<version_t> to_version_map(const region_map_t<binary_blob_t> &blob_map) {
    return region_map_transform<binary_blob_t, version_t>(
        blob_map, &binary_blob_t::get<version_t>);
}



RDB_IMPL_SERIALIZABLE_3_SINCE_v1_13(branch_birth_certificate_t,
                        region, initial_timestamp, origin);
RDB_IMPL_EQUALITY_COMPARABLE_3(branch_birth_certificate_t,
                               region, initial_timestamp, origin);

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(branch_history_t, branches);
RDB_IMPL_EQUALITY_COMPARABLE_1(branch_history_t, branches);
