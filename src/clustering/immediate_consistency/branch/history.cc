#include "clustering/immediate_consistency/branch/history.hpp"

template<class protocol_t>
bool version_is_ancestor(
        branch_history_manager_t<protocol_t> *bhm,
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
        branch_birth_certificate_t<protocol_t> descendent_branch_metadata = bhm->get_branch(descendent.branch);

        rassert_unreviewed(region_is_superset(descendent_branch_metadata.region, relevant_region));
        rassert_unreviewed(descendent.timestamp >= descendent_branch_metadata.initial_timestamp);

        typedef region_map_t<protocol_t, version_range_t> version_map_t;
        version_map_t relevant_origin = descendent_branch_metadata.origin.mask(relevant_region);

        rassert_unreviewed(relevant_origin.begin() != relevant_origin.end());
        for (typename version_map_t::const_iterator it  = relevant_origin.begin();
                                                    it != relevant_origin.end();
                                                    it++) {
            rassert_unreviewed(!region_is_empty(it->first));
            rassert_unreviewed(version_is_ancestor(bhm, it->second.earliest, it->second.latest, it->first));
            if (!version_is_ancestor(bhm, ancestor, it->second.earliest, it->first)) {
                return false;
            }
        }
        return true;
    }
}

template <class protocol_t>
bool version_is_divergent(
        branch_history_manager_t<protocol_t> *bhm,
        version_t v1,
        version_t v2,
        typename protocol_t::region_t relevant_region) {
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
        mock::dummy_protocol_t::region_t relevant_region);

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
        memcached_protocol_t::region_t relevant_region);

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
        rdb_protocol_t::region_t relevant_region);

template region_map_t<mock::dummy_protocol_t, version_range_t> to_version_range_map<mock::dummy_protocol_t>(const region_map_t<mock::dummy_protocol_t, binary_blob_t> &blob_map);

template region_map_t<memcached_protocol_t, version_range_t> to_version_range_map<memcached_protocol_t>(const region_map_t<memcached_protocol_t, binary_blob_t> &blob_map);

template region_map_t<rdb_protocol_t, version_range_t> to_version_range_map<rdb_protocol_t>(const region_map_t<rdb_protocol_t, binary_blob_t> &blob_map);
