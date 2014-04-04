// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/generic/nonoverlapping_regions.hpp"

#include "region/region_json_adapter.hpp"

bool valid_regions_helper(const std::vector<region_t> &regionvec,
                          const std::set<region_t> &regionset) {
    // Disallow empty regions.
    if (regionset.find(region_t()) != regionset.end()) {
        return false;
    }

    // Require that the regions join to the universe.
    region_t should_be_universe;
    region_join_result_t res = region_join(regionvec, &should_be_universe);
    if (res != REGION_JOIN_OK || should_be_universe != region_t::universe()) {
        return false;
    }

    return true;
}

bool nonoverlapping_regions_t::set_regions(const std::vector<region_t> &regions) {
    std::set<region_t> regionset(regions.begin(), regions.end());

    // Disallow duplicate regions.
    if (regionset.size() != regions.size() || !valid_regions_helper(regions, regionset)) {
        return false;
    }

    regions_ = regionset;
    return true;
}

bool nonoverlapping_regions_t::valid_for_sharding() const {
    std::vector<region_t> regionvec(regions_.begin(), regions_.end());

    return valid_regions_helper(regionvec, regions_);
}

// TODO: Ugh, this is O(n)!  Oh well.
bool nonoverlapping_regions_t::add_region(const region_t &region) {
    for (std::set<region_t>::iterator it = regions_.begin();
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
json_adapter_if_t::json_adapter_map_t get_json_subfields(UNUSED nonoverlapping_regions_t *target) {
    return json_adapter_if_t::json_adapter_map_t();
}

cJSON *render_as_json(nonoverlapping_regions_t *target) {
    cJSON *res = cJSON_CreateArray();
    for (nonoverlapping_regions_t::iterator it = target->begin(); it != target->end(); ++it) {
        // TODO: An extra copy.
        region_t tmp = *it;
        cJSON_AddItemToArray(res, render_as_json(&tmp));
    }
    return res;
}

void apply_json_to(cJSON *change, nonoverlapping_regions_t *target) THROWS_ONLY(schema_mismatch_exc_t) {
    nonoverlapping_regions_t res;
    json_array_iterator_t it = get_array_it(change);
    std::vector<region_t> regions;
    for (cJSON *val; (val = it.next()); ) {
        region_t region;
        apply_json_to(val, &region);
        regions.push_back(region);
    }

    if (!res.set_regions(regions)) {
        throw schema_mismatch_exc_t("Sharding spec is invalid (regions overlap, empty regions exist, or they do not cover all keys).");
    }

    *target = res;
}
