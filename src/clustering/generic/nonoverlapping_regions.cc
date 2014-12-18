// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/generic/nonoverlapping_regions.hpp"

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

