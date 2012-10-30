// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "hash_region.hpp"
#include "btree/keys.hpp"
#include "memcached/region.hpp"

namespace unittest {

void assert_equal(const key_range_t& expected, const key_range_t& value) {
    ASSERT_EQ(key_to_unescaped_str(expected.left), key_to_unescaped_str(value.left));
    ASSERT_EQ(expected.right.unbounded, value.right.unbounded);
    ASSERT_EQ(key_to_unescaped_str(expected.right.key), key_to_unescaped_str(value.right.key));
}

void assert_empty(const hash_region_t<key_range_t>& r) {
    ASSERT_EQ(0, r.beg);
    ASSERT_EQ(0, r.end);
    assert_equal(key_range_t::empty(), r.inner);
}

TEST(HashRegionTest, RegionJoinHashwise) {

    key_range_t kr(key_range_t::closed, store_key_t("Alpha"), key_range_t::open, store_key_t("Beta"));

    std::vector<hash_region_t<key_range_t> > vec;
    vec.push_back(hash_region_t<key_range_t>(0, 10, kr));
    vec.push_back(hash_region_t<key_range_t>(20, 30, kr));
    vec.push_back(hash_region_t<key_range_t>(10, 20, kr));

    hash_region_t<key_range_t> r;
    region_join_result_t res = region_join(vec, &r);

    ASSERT_EQ(REGION_JOIN_OK, res);
    ASSERT_EQ(0, r.beg);
    ASSERT_EQ(30, r.end);
    assert_equal(kr, r.inner);
}

TEST(HashRegionTest, RegionJoinEmpty) {

    std::vector<hash_region_t<key_range_t> > vec;

    hash_region_t<key_range_t> r;
    region_join_result_t res = region_join(vec, &r);

    ASSERT_EQ(REGION_JOIN_OK, res);
    assert_empty(r);
}

TEST(HashRegionTest, RegionJoinEmpties) {

    key_range_t kr(key_range_t::closed, store_key_t("Alpha"), key_range_t::open, store_key_t("Beta"));

    std::vector<hash_region_t<key_range_t> > vec;
    vec.push_back(hash_region_t<key_range_t>());
    vec.push_back(hash_region_t<key_range_t>());
    vec.push_back(hash_region_t<key_range_t>());
    vec.push_back(hash_region_t<key_range_t>());

    hash_region_t<key_range_t> r;
    region_join_result_t res = region_join(vec, &r);

    ASSERT_EQ(REGION_JOIN_OK, res);
    assert_empty(r);
}

TEST(HashRegionTest, RegionJoin2D) {

    key_range_t kr(key_range_t::closed, store_key_t("Alpha"), key_range_t::open, store_key_t("Beta"));
    key_range_t kr2(key_range_t::closed, store_key_t("Beta"), key_range_t::open, store_key_t("Chi"));
    key_range_t kr3(key_range_t::closed, store_key_t("Alpha"), key_range_t::none, store_key_t());
    key_range_t kr4(key_range_t::closed, store_key_t("Chi"), key_range_t::none, store_key_t());

    std::vector<hash_region_t<key_range_t> > vec;
    vec.push_back(hash_region_t<key_range_t>(1, 5, kr2));
    vec.push_back(hash_region_t<key_range_t>(5, 10, kr2));
    vec.push_back(hash_region_t<key_range_t>(5, 10, kr));
    vec.push_back(hash_region_t<key_range_t>(1, 5, kr));
    vec.push_back(hash_region_t<key_range_t>(1, 10, kr4));

    hash_region_t<key_range_t> r;
    region_join_result_t res = region_join(vec, &r);

    ASSERT_EQ(REGION_JOIN_OK, res);
    ASSERT_EQ(1, r.beg);
    ASSERT_EQ(10, r.end);
    assert_equal(kr3, r.inner);
}

TEST(HashRegionTest, SpecialNonSquareRegionJoin) {

    key_range_t zt(key_range_t::none, store_key_t(), key_range_t::open, store_key_t("t"));
    key_range_t tinf(key_range_t::closed, store_key_t("t"), key_range_t::none, store_key_t());

    uint64_t quarter = HASH_REGION_HASH_SIZE / 4;
    uint64_t half = quarter * 2;
    uint64_t whole = HASH_REGION_HASH_SIZE;

    std::vector<hash_region_t<key_range_t> > vec;
    vec.push_back(hash_region_t<key_range_t>(0, quarter, zt));
    vec.push_back(hash_region_t<key_range_t>(0, quarter, tinf));
    vec.push_back(hash_region_t<key_range_t>(half, whole, zt));
    vec.push_back(hash_region_t<key_range_t>(half, whole, tinf));
    vec.push_back(hash_region_t<key_range_t>(quarter, half, zt));
    vec.push_back(hash_region_t<key_range_t>(quarter, half, tinf));

    hash_region_t<key_range_t> r;
    region_join_result_t res = region_join(vec, &r);

    ASSERT_EQ(REGION_JOIN_OK, res);
    ASSERT_EQ(0, r.beg);
    ASSERT_EQ(whole, r.end);
    assert_equal(key_range_t::universe(), r.inner);
}





}  // namespace unittest

