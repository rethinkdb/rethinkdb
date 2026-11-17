// Copyright 2024 RethinkDB, all rights reserved.
#include <stdint.h>

#include <string>

#include "unittest/gtest.hpp"

#include "clustering/administration/main/cache_size.hpp"

namespace unittest {

const uint64_t KB = 1024;

TEST(ParseMeminfoFileTest, PrefersMemAvailable) {
    std::string contents =
        "MemTotal:       16330092 kB\n"
        "MemFree:         2151452 kB\n"
        "MemAvailable:    9423100 kB\n"
        "Buffers:          470940 kB\n"
        "Cached:          6858424 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_TRUE(parse_meminfo_file(contents, &mem_avail));
    /* MemAvailable must be reported as-is; the other fields must not be
    added on top of it, since the kernel already accounts for them. */
    ASSERT_EQ(uint64_t(9423100) * KB, mem_avail);
}

TEST(ParseMeminfoFileTest, MemAvailableAfterOtherFields) {
    /* "MemAvailable" appears before "Cached" in real /proc/meminfo files, but
    we shouldn't depend on the exact line order, so make sure a file with a
    different ordering parses correctly as well. */
    std::string contents =
        "MemTotal:       16330092 kB\n"
        "MemFree:         2151452 kB\n"
        "Buffers:          470940 kB\n"
        "Cached:          6858424 kB\n"
        "MemAvailable:    9423100 kB\n"
        "SwapCached:            0 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_TRUE(parse_meminfo_file(contents, &mem_avail));
    ASSERT_EQ(uint64_t(9423100) * KB, mem_avail);
}

TEST(ParseMeminfoFileTest, FallsBackToMemFreePlusCachedPlusBuffers) {
    /* A kernel without "MemAvailable" support (Linux < 3.14). */
    std::string contents =
        "MemTotal:        8042532 kB\n"
        "MemFree:         1252388 kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3354160 kB\n"
        "SwapCached:       130948 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_TRUE(parse_meminfo_file(contents, &mem_avail));
    ASSERT_EQ((uint64_t(1252388) + 280312 + 3354160) * KB, mem_avail);
}

TEST(ParseMeminfoFileTest, FallbackRequiresBuffers) {
    std::string contents =
        "MemTotal:        8042532 kB\n"
        "MemFree:         1252388 kB\n"
        "Cached:          3354160 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, FallbackRequiresCached) {
    std::string contents =
        "MemTotal:        8042532 kB\n"
        "MemFree:         1252388 kB\n"
        "Buffers:          280312 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, FallbackRequiresMemFree) {
    std::string contents =
        "MemTotal:        8042532 kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3354160 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, DuplicateMemFreeFails) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB\n"
        "MemFree:         1100000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, DuplicateBuffersFails) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "Buffers:          280312 kB\n"
        "Buffers:          280313 kB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, DuplicateMemAvailableFails) {
    std::string contents =
        "MemAvailable:    5000000 kB\n"
        "MemFree:         1000000 kB\n"
        "MemAvailable:    5100000 kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, WrongUnitOnMemFreeFails) {
    std::string contents =
        "MemFree:         1000000 MB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, WrongUnitOnBuffersFails) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "Buffers:          280312 MB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, WrongUnitOnMemAvailableFails) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "MemAvailable:    5000000 mB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, EmptyContentsFails) {
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file("", &mem_avail));
}

TEST(ParseMeminfoFileTest, MalformedLineWithoutColonFails) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "Buffers 280312 kB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, NonNumericValueFails) {
    std::string contents =
        "MemFree:         lots kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_FALSE(parse_meminfo_file(contents, &mem_avail));
}

TEST(ParseMeminfoFileTest, IgnoresUnrelatedFields) {
    /* A realistic excerpt with fields we don't care about, including some
    with unusual names. */
    std::string contents =
        "MemTotal:       16330092 kB\n"
        "MemFree:         2151452 kB\n"
        "MemAvailable:    9423100 kB\n"
        "Buffers:          470940 kB\n"
        "Cached:          6858424 kB\n"
        "SwapCached:            0 kB\n"
        "Active:          6932528 kB\n"
        "Inactive:        4833472 kB\n"
        "Active(anon):    3231220 kB\n"
        "SwapTotal:       4194300 kB\n"
        "HugePages_Total:       0\n"
        "Hugepagesize:       2048 kB\n";
    uint64_t mem_avail = 0;
    ASSERT_TRUE(parse_meminfo_file(contents, &mem_avail));
    ASSERT_EQ(uint64_t(9423100) * KB, mem_avail);
}

TEST(ParseMeminfoFileTest, TrailingNewlineIsFine) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB\n"
        "\n";
    uint64_t mem_avail = 0;
    ASSERT_TRUE(parse_meminfo_file(contents, &mem_avail));
    ASSERT_EQ((uint64_t(1000000) + 280312 + 3000000) * KB, mem_avail);
}

TEST(ParseMeminfoFileTest, NoTrailingNewlineIsFine) {
    std::string contents =
        "MemFree:         1000000 kB\n"
        "Buffers:          280312 kB\n"
        "Cached:          3000000 kB";
    uint64_t mem_avail = 0;
    ASSERT_TRUE(parse_meminfo_file(contents, &mem_avail));
    ASSERT_EQ((uint64_t(1000000) + 280312 + 3000000) * KB, mem_avail);
}

}  // namespace unittest
