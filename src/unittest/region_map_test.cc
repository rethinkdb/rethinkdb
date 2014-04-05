// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "region/region_map.hpp"

namespace unittest {

hash_region_t<key_range_t> make_region(std::string low, std::string high) {
    return hash_region_t<key_range_t>(key_range_t(key_range_t::closed, store_key_t(low),
                                                  key_range_t::open, store_key_t(high)));
}

TEST(RegionMap, BasicSets) {
    region_map_t<int> rmap(make_region("a", "z"), 0);
    rmap.set(make_region("a", "n"), 1);
    rmap.set(make_region("n", "z"), 2);
    for (region_map_t<int>::iterator it  = rmap.begin();
         it != rmap.end();
         ++it) {
        if (region_is_superset(make_region("a", "n"), it->first)) {
            EXPECT_EQ(1, it->second);
        } else if (region_is_superset(make_region("n", "z"), it->first)) {
            EXPECT_EQ(2, it->second);
        } else {
            ADD_FAILURE() << "Got unexpected region from region map.";
        }
    }
}

TEST(RegionMap, Mask) {
    region_map_t<int> rmap(make_region("a", "z"), 0);
    rmap.set(make_region("a", "n"), 1);
    rmap.set(make_region("n", "z"), 2);

    region_map_t<int> masked_map = rmap.mask(make_region("g", "s"));

    EXPECT_TRUE(masked_map.get_domain() == make_region("g", "s"));

    for (region_map_t<int>::iterator it  = masked_map.begin();
         it != masked_map.end();
         ++it) {
        if (region_is_superset(make_region("a", "n"), it->first)) {
            EXPECT_EQ(1, it->second);
        } else if (region_is_superset(make_region("n", "z"), it->first)) {
            EXPECT_EQ(2, it->second);
        } else {
            ADD_FAILURE() << "Got unexpected region from region map.";
        }
    }
}

}  //namespace unittest
