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
    rmap.update(make_region("a", "n"), 1);
    rmap.update(make_region("n", "z"), 2);
    rmap.visit(rmap.get_domain(), [&](const region_t &reg, int val) {
        if (region_is_superset(make_region("a", "n"), reg)) {
            EXPECT_EQ(1, val);
        } else if (region_is_superset(make_region("n", "z"), reg)) {
            EXPECT_EQ(2, val);
        } else {
            ADD_FAILURE() << "Got unexpected region from region map.";
        }
    });
}

TEST(RegionMap, Mask) {
    region_map_t<int> rmap(make_region("a", "z"), 0);
    rmap.update(make_region("a", "n"), 1);
    rmap.update(make_region("n", "z"), 2);

    region_map_t<int> masked_map = rmap.mask(make_region("g", "s"));

    EXPECT_TRUE(masked_map.get_domain() == make_region("g", "s"));

    masked_map.visit(masked_map.get_domain(), [&](const region_t &reg, int val) {
        if (region_is_superset(make_region("g", "n"), reg)) {
            EXPECT_EQ(1, val);
        } else if (region_is_superset(make_region("n", "s"), reg)) {
            EXPECT_EQ(2, val);
        } else {
            ADD_FAILURE() << "Got unexpected region from region map.";
        }
    });
}

}  //namespace unittest
