// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/history.hpp"

#include <stack>

template <class protocol_t>
bool version_is_ancestor(
        branch_history_manager_t<protocol_t> *bhm,
        const version_t ancestor,
        version_t descendent,
        typename protocol_t::region_t relevant_region) {
    typedef region_map_t<protocol_t, version_range_t> version_map_t;
    // A stack of version maps and iterators pointing an the next element in the map to traverse.
    std::stack<std::pair<version_map_t *, typename version_map_t::const_iterator> > origin_stack;

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
            branch_birth_certificate_t<protocol_t> descendent_branch_metadata = bhm->get_branch(descendent.branch);

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

        typename version_map_t::const_iterator it = origin_stack.top().second;
        descendent = it->second.earliest;
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


template <class protocol_t>
bool version_is_divergent(
        branch_history_manager_t<protocol_t> *bhm,
        version_t v1,
        version_t v2,
        const typename protocol_t::region_t &relevant_region) {
    return !version_is_ancestor(bhm, v1, v2, relevant_region) &&
           !version_is_ancestor(bhm, v2, v1, relevant_region);
}


template <class protocol_t>
region_map_t<protocol_t, version_range_t> to_version_range_map(const region_map_t<protocol_t, binary_blob_t> &blob_map) {
    return region_map_transform<protocol_t, binary_blob_t, version_range_t>(blob_map,
                                                                            &binary_blob_t::get<version_range_t>);
}





#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"
#include "rdb_protocol/protocol.hpp"


template
bool version_is_ancestor<mock::dummy_protocol_t>(
        branch_history_manager_t<mock::dummy_protocol_t> *bhm,
        version_t ancestor,
        version_t descendent,
        mock::dummy_protocol_t::region_t relevant_region);

template
bool version_is_divergent<mock::dummy_protocol_t>(
        branch_history_manager_t<mock::dummy_protocol_t> *bhm,
        version_t v1,
        version_t v2,
        const mock::dummy_protocol_t::region_t &relevant_region);

template
bool version_is_ancestor<memcached_protocol_t>(
        branch_history_manager_t<memcached_protocol_t> *bhm,
        version_t ancestor,
        version_t descendent,
        memcached_protocol_t::region_t relevant_region);

template
bool version_is_divergent<memcached_protocol_t>(
        branch_history_manager_t<memcached_protocol_t> *bhm,
        version_t v1,
        version_t v2,
        const memcached_protocol_t::region_t &relevant_region);

template
bool version_is_ancestor<rdb_protocol_t>(
        branch_history_manager_t<rdb_protocol_t> *bhm,
        version_t ancestor,
        version_t descendent,
        rdb_protocol_t::region_t relevant_region);

template
bool version_is_divergent<rdb_protocol_t>(
        branch_history_manager_t<rdb_protocol_t> *bhm,
        version_t v1,
        version_t v2,
        const rdb_protocol_t::region_t &relevant_region);

template region_map_t<mock::dummy_protocol_t, version_range_t> to_version_range_map<mock::dummy_protocol_t>(const region_map_t<mock::dummy_protocol_t, binary_blob_t> &blob_map);

template region_map_t<memcached_protocol_t, version_range_t> to_version_range_map<memcached_protocol_t>(const region_map_t<memcached_protocol_t, binary_blob_t> &blob_map);

template region_map_t<rdb_protocol_t, version_range_t> to_version_range_map<rdb_protocol_t>(const region_map_t<rdb_protocol_t, binary_blob_t> &blob_map);
