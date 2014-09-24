#include "unittest/gtest.hpp"

#include "containers/lru_cache.hpp"

namespace unittest {

TEST(LRUCacheTest, Simple) {
    lru_cache_t<int, int> cache(10);
    cache[4] = 5;
    cache[8] = 10;
    cache[0] = 12;
    EXPECT_EQ(10, cache.max_size()) << "cache max size is not correct";
    EXPECT_EQ(3, cache.size()) << "cache current size is not correct";
    EXPECT_EQ(10, cache[8]) << "cache [] is not correct";
    cache[4] = 18;
    EXPECT_EQ(cache[4], 18) << "cache update is wrong";
    
    auto it = cache.begin();
    EXPECT_EQ(4, it->first);
    EXPECT_EQ(18, it->second);
    ++it;
    EXPECT_EQ(8, it->first);
    EXPECT_EQ(10, it->second);
    ++it;
    EXPECT_EQ(0, it->first);
    EXPECT_EQ(12, it->second);
    ++it;
    EXPECT_EQ(cache.end(), it) << "Wrong number of iterations?";
}

TEST(LRUCacheTest, Eviction) {
    lru_cache_t<int, int> cache(10);
    for (int i = 0; i < 20; i++) cache[i] = i;
    EXPECT_EQ(10, cache.size());
    EXPECT_EQ(19, cache.begin()->first);
    EXPECT_EQ(10, cache.rbegin()->first);
    cache[4] = 42;
    cache[14] = 88;
    cache[8] = 18;
    EXPECT_EQ(10, cache.size());
    
    auto it = cache.begin();
    EXPECT_EQ(8, it->first);
    EXPECT_EQ(18, it->second);
    ++it;
    EXPECT_EQ(14, it->first);
    EXPECT_EQ(88, it->second);
    ++it;
    EXPECT_EQ(4, it->first);
    EXPECT_EQ(42, it->second);
    ++it;
    EXPECT_EQ(19, it->first);
    EXPECT_EQ(19, it->second);
    EXPECT_EQ(12, cache.rbegin()->first);
}

TEST(LRUCacheTest, Find) {
    lru_cache_t<int, int> cache(10);
    for (int i = 0; i < 20; i++) cache[i] = i;
    EXPECT_EQ(19, cache.begin()->first);
    EXPECT_EQ(cache.end(), cache.find(3));
    EXPECT_EQ(19, cache.begin()->first);
    EXPECT_EQ(13, cache.find(13)->first);
    EXPECT_EQ(13, cache.begin()->first);
    EXPECT_EQ(10, cache.rbegin()->first);
}

} // namespace unittest
