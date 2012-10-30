// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/generic/nonoverlapping_regions.hpp"

#include "protocol_api.hpp"

template <class protocol_t>
bool valid_regions_helper(const std::vector<typename protocol_t::region_t> &regionvec,
                          const std::set<typename protocol_t::region_t> &regionset) {
    // Disallow empty regions.
    if (regionset.find(typename protocol_t::region_t()) != regionset.end()) {
        return false;
    }

    // Require that the regions join to the universe.
    typename protocol_t::region_t should_be_universe;
    region_join_result_t res = region_join(regionvec, &should_be_universe);
    if (res != REGION_JOIN_OK || should_be_universe != protocol_t::region_t::universe()) {
        return false;
    }

    return true;
}

template <class protocol_t>
bool nonoverlapping_regions_t<protocol_t>::set_regions(const std::vector<typename protocol_t::region_t> &regions) {
    std::set<typename protocol_t::region_t> regionset(regions.begin(), regions.end());

    // Disallow duplicate regions.
    if (regionset.size() != regions.size() || !valid_regions_helper<protocol_t>(regions, regionset)) {
        return false;
    }

    regions_ = regionset;
    return true;
}

template <class protocol_t>
bool nonoverlapping_regions_t<protocol_t>::valid_for_sharding() const {
    std::vector<typename protocol_t::region_t> regionvec(regions_.begin(), regions_.end());

    return valid_regions_helper<protocol_t>(regionvec, regions_);
}

// TODO: Ugh, this is O(n)!  Oh well.
template <class protocol_t>
bool nonoverlapping_regions_t<protocol_t>::add_region(const typename protocol_t::region_t &region) {
    for (typename std::set<typename protocol_t::region_t>::iterator it = regions_.begin();
         it != regions_.end();
         ++it) {
        if (region_overlaps(region, *it)) {
            return false;
        }
    }

    regions_.insert(region);
    return true;
}


// TODO: See if any json adapter for std::set is being used.

// ctx-less json adapter concept for nonoverlapping_regions_t.
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(UNUSED nonoverlapping_regions_t<protocol_t> *target) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class protocol_t>
cJSON *render_as_json(nonoverlapping_regions_t<protocol_t> *target) {
    cJSON *res = cJSON_CreateArray();
    for (typename nonoverlapping_regions_t<protocol_t>::iterator it = target->begin(); it != target->end(); ++it) {
        // TODO: An extra copy.
        typename protocol_t::region_t tmp = *it;
        cJSON_AddItemToArray(res, render_as_json(&tmp));
    }
    return res;
}

template <class protocol_t>
void apply_json_to(cJSON *change, nonoverlapping_regions_t<protocol_t> *target) THROWS_ONLY(schema_mismatch_exc_t) {
    nonoverlapping_regions_t<protocol_t> res;
    json_array_iterator_t it = get_array_it(change);
    std::vector<typename protocol_t::region_t> regions;
    for (cJSON *val; (val = it.next()); ) {
        typename protocol_t::region_t region;
        apply_json_to(val, &region);
        regions.push_back(region);
    }

    if (!res.set_regions(regions)) {
        throw schema_mismatch_exc_t("Sharding spec is invalid (regions overlap, empty regions exist, or they do not cover all keys).");
    }

    *target = res;
}

template <class protocol_t>
void on_subfield_change(UNUSED nonoverlapping_regions_t<protocol_t> *target) { }



#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
template class nonoverlapping_regions_t<mock::dummy_protocol_t>;
template json_adapter_if_t::json_adapter_map_t get_json_subfields<mock::dummy_protocol_t>(nonoverlapping_regions_t<mock::dummy_protocol_t> *target);
template cJSON *render_as_json<mock::dummy_protocol_t>(nonoverlapping_regions_t<mock::dummy_protocol_t> *target);
template void apply_json_to<mock::dummy_protocol_t>(cJSON *change, nonoverlapping_regions_t<mock::dummy_protocol_t> *target);
template void on_subfield_change<mock::dummy_protocol_t>(nonoverlapping_regions_t<mock::dummy_protocol_t> *target);

#include "memcached/protocol.hpp"
#include "memcached/protocol_json_adapter.hpp"
template class nonoverlapping_regions_t<memcached_protocol_t>;
template json_adapter_if_t::json_adapter_map_t get_json_subfields<memcached_protocol_t>(nonoverlapping_regions_t<memcached_protocol_t> *target);
template cJSON *render_as_json<memcached_protocol_t>(nonoverlapping_regions_t<memcached_protocol_t> *target);
template void apply_json_to<memcached_protocol_t>(cJSON *change, nonoverlapping_regions_t<memcached_protocol_t> *target);
template void on_subfield_change<memcached_protocol_t>(nonoverlapping_regions_t<memcached_protocol_t> *target);

#include "rdb_protocol/protocol.hpp"
template class nonoverlapping_regions_t<rdb_protocol_t>;
template json_adapter_if_t::json_adapter_map_t get_json_subfields<rdb_protocol_t>(nonoverlapping_regions_t<rdb_protocol_t> *target);
template cJSON *render_as_json<rdb_protocol_t>(nonoverlapping_regions_t<rdb_protocol_t> *target);
template void apply_json_to<rdb_protocol_t>(cJSON *change, nonoverlapping_regions_t<rdb_protocol_t> *target);
template void on_subfield_change<rdb_protocol_t>(nonoverlapping_regions_t<rdb_protocol_t> *target);

