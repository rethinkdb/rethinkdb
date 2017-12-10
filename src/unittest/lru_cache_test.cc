#include "unittest/gtest.hpp"

#include "containers/lru_cache.hpp"

namespace unittest {

TEST(LRUCacheTest, Basic) {
    lru_cache_t<std::string, std::string> cache(4);
    EXPECT_TRUE(cache.insert("4", "5"));
    EXPECT_TRUE(cache.insert("8", "10"));
    EXPECT_TRUE(cache.insert("0", "12"));
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(4, cache.max_size());
    std::string *p;
    bool res = cache.lookup("8", &p);
    ASSERT_TRUE(res);
    EXPECT_EQ("10", *p);
    // Usage ordering is now 4 0 8.
    EXPECT_EQ(false, cache.insert("4", "9"));
    EXPECT_TRUE(cache.insert("7", "77"));
    // Usage ordering is now 4 0 8 7
    EXPECT_EQ(4, cache.size());
    EXPECT_TRUE(cache.insert("6", ""));
    EXPECT_EQ(4, cache.size());
    // Usage ordering is now 0 8 7 6.  The key "4" was evicted.
    res = cache.lookup("4", &p);
    EXPECT_FALSE(res);
    res = cache.lookup("0", &p);
    ASSERT_TRUE(res);
    EXPECT_EQ("12", *p);
    // Usage ordering is now 8 7 6 0.
    EXPECT_TRUE(cache.insert("9", "99"));
    // Usage ordering is now 7 6 0 9.
    EXPECT_EQ(4, cache.size());
    res = cache.lookup("8", &p);
    EXPECT_FALSE(res);
    res = cache.lookup("4", &p);
    EXPECT_FALSE(res);
}

} // namespace unittest
