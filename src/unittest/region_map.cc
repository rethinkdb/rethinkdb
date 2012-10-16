#include "unittest/gtest.hpp"

#include "mock/dummy_protocol.hpp"
#include "protocol_api.hpp"

namespace unittest {
using mock::dummy_protocol_t;
TEST(RegionMap, BasicSets) {
    region_map_t<dummy_protocol_t, int> rmap(dummy_protocol_t::region_t('a', 'z'), 0);
    rmap.set(dummy_protocol_t::region_t('a', 'm'), 1);
    rmap.set(dummy_protocol_t::region_t('n', 'z'), 2);
    for (region_map_t<dummy_protocol_t, int>::iterator it  = rmap.begin();
         it != rmap.end();
         it++) {
        if (region_is_superset(dummy_protocol_t::region_t('a', 'm'), it->first)) {
            EXPECT_EQ(1, it->second);
        } else if (region_is_superset(dummy_protocol_t::region_t('n', 'z'), it->first)) {
            EXPECT_EQ(2, it->second);
        } else {
            ADD_FAILURE() << "Got unexpected region from region map.";
        }
    }
}

TEST(RegionMap, Mask) {
    region_map_t<dummy_protocol_t, int> rmap(dummy_protocol_t::region_t('a', 'z'), 0);
    rmap.set(dummy_protocol_t::region_t('a', 'm'), 1);
    rmap.set(dummy_protocol_t::region_t('n', 'z'), 2);

    region_map_t<dummy_protocol_t, int> masked_map = rmap.mask(dummy_protocol_t::region_t('g', 'r'));

    EXPECT_TRUE(masked_map.get_domain() == dummy_protocol_t::region_t('g', 'r'));

    for (region_map_t<dummy_protocol_t, int>::iterator it  = masked_map.begin();
         it != masked_map.end();
         it++) {
        if (region_is_superset(dummy_protocol_t::region_t('a', 'm'), it->first)) {
            EXPECT_EQ(1, it->second);
        } else if (region_is_superset(dummy_protocol_t::region_t('n', 'z'), it->first)) {
            EXPECT_EQ(2, it->second);
        } else {
            ADD_FAILURE() << "Got unexpected region from region map.";
        }
    }
}
} //namespace unittest
