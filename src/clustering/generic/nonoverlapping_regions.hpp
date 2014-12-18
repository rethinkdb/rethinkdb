// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_NONOVERLAPPING_REGIONS_HPP_
#define CLUSTERING_GENERIC_NONOVERLAPPING_REGIONS_HPP_

#include <set>
#include <vector>

#include "containers/archive/stl_types.hpp"
#include "region/region.hpp"
#include "rpc/serialize_macros.hpp"

class nonoverlapping_regions_t {
public:
    nonoverlapping_regions_t() { }

    // Returns true upon success, false if there's an overlap or the
    // regions don't join to the universal set.  This is like calling
    // add_region in a loop and then valid_for_sharding, only maybe without being O(n^2).
    MUST_USE bool set_regions(const std::vector<region_t> &regions);

    // We already know by the public interface that no regions
    // overlap.  This checks that their join is the universe, and that
    // there is no empty region.
    bool valid_for_sharding() const;

    // TODO: Use valid_for_sharding everywhere we use add_region.

    // Returns true upon success, false if there's an overlap.  This is O(n)!
    MUST_USE bool add_region(const region_t &region);

    // Yes, both of these are const iterators.
    typedef std::set<region_t>::iterator iterator;
    iterator begin() const { return regions_.begin(); }
    iterator end() const { return regions_.end(); }

    bool empty() const { return regions_.empty(); }
    size_t size() const { return regions_.size(); }

    void remove_region(iterator it) {
        guarantee(it != regions_.end());
        regions_.erase(it);
    }

    bool operator==(const nonoverlapping_regions_t &other) const {
        return regions_ == other.regions_;
    }

    RDB_MAKE_ME_SERIALIZABLE_1(regions_);

private:
    std::set<region_t> regions_;
};

RDB_SERIALIZE_OUTSIDE(nonoverlapping_regions_t);

#endif  // CLUSTERING_GENERIC_NONOVERLAPPING_REGIONS_HPP_
