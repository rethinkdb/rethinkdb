#include "unittest/gtest.hpp"

#include <string>

#include "containers/lru_cache.hpp"

namespace unittest {

TEST(LRUCacheTest, Simple) {
    lru_cache_t<std::string, int> cache(10);
    cache["4"] = 5;
    cache["8"] = 10;
    cache["0"] = 12;
    EXPECT_EQ(10, cache.max_size()) << "cache max size is not correct";
    EXPECT_EQ(3, cache.size()) << "cache current size is not correct";
    EXPECT_EQ(10, cache["8"]) << "cache [] is not correct";
    cache["4"] = 18;
    EXPECT_EQ(cache["4"], 18) << "cache update is wrong";

    auto it = cache.begin();
    EXPECT_EQ("4", it->first);
    EXPECT_EQ(18, it->second);
    ++it;
    EXPECT_EQ("8", it->first);
    EXPECT_EQ(10, it->second);
    ++it;
    EXPECT_EQ("0", it->first);
    EXPECT_EQ(12, it->second);
    ++it;
    EXPECT_EQ(cache.end(), it) << "Wrong number of iterations?";
}

TEST(LRUCacheTest, Eviction) {
    lru_cache_t<std::string, int> cache(10);
    char c[2] = {0};
    for (c[0] = 'a'; c[0] < 'u'; c[0]++) cache[c] = c[0] - 'a';
    EXPECT_EQ(10, cache.size());
    EXPECT_EQ("t", cache.begin()->first);
    EXPECT_EQ("k", cache.rbegin()->first);
    cache["e"] = 42;
    cache["o"] = 88;
    cache["i"] = 18;
    EXPECT_EQ(10, cache.size());

    auto it = cache.begin();
    EXPECT_EQ("i", it->first);
    EXPECT_EQ(18, it->second);
    ++it;
    EXPECT_EQ("o", it->first);
    EXPECT_EQ(88, it->second);
    ++it;
    EXPECT_EQ("e", it->first);
    EXPECT_EQ(42, it->second);
    ++it;
    EXPECT_EQ("t", it->first);
    EXPECT_EQ(19, it->second);
    EXPECT_EQ("m", cache.rbegin()->first);
}

TEST(LRUCacheTest, Find) {
    lru_cache_t<std::string, int> cache(10);
    char c[2] = {0};
    for (c[0] = 'a'; c[0] < 'u'; c[0]++) cache[c] = c[0] - 'a';
    EXPECT_EQ("t", cache.begin()->first);
    EXPECT_EQ(cache.end(), cache.find("d"));
    EXPECT_EQ("t", cache.begin()->first);
    EXPECT_EQ("n", cache.find("n")->first);
    EXPECT_EQ("n", cache.begin()->first);
    EXPECT_EQ("k", cache.rbegin()->first);
}

} // namespace unittest
