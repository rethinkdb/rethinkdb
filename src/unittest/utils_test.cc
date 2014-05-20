// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <ctime>

#include "arch/address.hpp"
#include "arch/runtime/runtime.hpp"
#include "btree/keys.hpp"
#include "stl_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

namespace unittest {

TEST(UtilsTest, BeginsWithMinus) {
    char test1[] = "    -foo";
    char test2[] = "-bar";
    char test3[] = "    baz";
    char test4[] = "qux-";
    char test5[] = "";
    ASSERT_TRUE(begins_with_minus(test1));
    ASSERT_TRUE(begins_with_minus(test2));
    ASSERT_FALSE(begins_with_minus(test3));
    ASSERT_FALSE(begins_with_minus(test4));
    ASSERT_FALSE(begins_with_minus(test5));
}

TEST(UtilsTest, StrtofooStrict) {
    char test1[] = "-1024";
    char test2[] = "1024";
    char test3[] = "102834728273347433844";
    char test4[] = "123lskdjf";

    const char *end;

    ASSERT_EQ(0u, strtou64_strict(test1, &end, 10));
    ASSERT_EQ(1024u, strtou64_strict(test2, &end, 10));
    ASSERT_EQ(0u, strtou64_strict(test3, &end, 10));
    ASSERT_EQ(123u, strtou64_strict(test4, &end, 10));
    ASSERT_EQ(0, strncmp("lskdjf", end, 6));

    bool success;
    uint64_t u_res;
    int64_t i_res;

    success = strtou64_strict(std::string(test1), 10, &u_res);
    ASSERT_TRUE(!success && u_res == 0);
    success = strtoi64_strict(std::string(test1), 10, &i_res);
    ASSERT_TRUE(success && i_res == -1024);
    success = strtou64_strict(std::string(test2), 10, &u_res);
    ASSERT_TRUE(success && u_res == 1024);
    success = strtoi64_strict(std::string(test2), 10, &i_res);
    ASSERT_TRUE(success && i_res == 1024);
    success = strtou64_strict(std::string(test3), 10, &u_res);
    ASSERT_TRUE(!success && u_res == 0);
    success = strtoi64_strict(std::string(test3), 10, &i_res);
    ASSERT_TRUE(!success && i_res == 0);
    success = strtou64_strict(std::string(test4), 10, &u_res);
    ASSERT_TRUE(!success && u_res == 0);
    success = strtoi64_strict(std::string(test4), 10, &i_res);
    ASSERT_TRUE(!success && i_res == 0);
}

TEST(UtilsTest, Time) {
    // Change the timezone for the duration of this test
    char *hastz = getenv("TZ");
    std::string oldtz(hastz ? hastz : "");
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();

    struct timespec time = {1335301122, 1234};
    std::string formatted = format_time(time);
    EXPECT_EQ("2012-04-24T13:58:42.000001234", formatted);
    struct timespec parsed = parse_time(formatted);
    EXPECT_EQ(time.tv_sec, parsed.tv_sec);
    EXPECT_EQ(time.tv_nsec, parsed.tv_nsec);

    if (hastz) {
      setenv("TZ", oldtz.c_str(), 1);
    } else {
      unsetenv("TZ");
    }
    tzset();
}

TEST(BtreeUtilsTest, SizedStrcmp) {
    uint8_t test1[] = "foobarbazn\nqux";
    uint8_t test2[] = "foobarbazn\nquxr";
    uint8_t test3[] = "hello world";

    ASSERT_GT(0, sized_strcmp(test1, 14, test2, 15));
    ASSERT_LT(0, sized_strcmp(test2, 15, test1, 14));
    ASSERT_EQ(0, sized_strcmp(test1, 10, test1, 10));
    ASSERT_EQ(0, sized_strcmp(test1, 14, test1, 14));
    ASSERT_EQ(0, sized_strcmp(test1, 0, test1, 0));
    ASSERT_NE(0, sized_strcmp(test3, 11, test1, 14));
}

/* This doesn't quite belong in `utils_test.cc`, but I don't want to create a
new file just for it. */

// Since ip_address_t may block, it requires a thread_pool_t to exist
TPTEST(UtilsTest, IPAddress) {
    std::set<ip_address_t> ips = hostname_to_ips("203.0.113.59");
    EXPECT_EQ(static_cast<size_t>(1), ips.size());
    EXPECT_EQ("203.0.113.59", ips.begin()->to_string());

    ips = hostname_to_ips("::DEAD:0:BEEF");
    EXPECT_EQ(static_cast<size_t>(1), ips.size());
    EXPECT_EQ("::dead:0:beef", ips.begin()->to_string());

    ips = hostname_to_ips("::FFFF:203.0.113.59");

    std::set<std::string> ip_strings;
    for (auto ip_it = ips.begin(); ip_it != ips.end(); ++ip_it) {
        ip_strings.insert(ip_it->to_string());
    }

    // Some platforms may give us the IPv4 and IPv6 address, so handle both cases
    EXPECT_TRUE(ip_strings.find("::ffff:203.0.113.59") != ip_strings.end());
    if (ip_strings.size() > 1) {
        EXPECT_EQ(static_cast<size_t>(2), ips.size());
        EXPECT_TRUE(ip_strings.find("203.0.113.59") != ip_strings.end());
    }
}

TEST(StlUtilsTest, SplitString) {
    EXPECT_EQ(make_vector<std::string>(""), split_string("", '.'));
    EXPECT_EQ(make_vector<std::string>("", ""), split_string(".", '.'));
    EXPECT_EQ(make_vector<std::string>("1", "", "3"), split_string("1..3", '.'));
    EXPECT_EQ(make_vector<std::string>("1", "3"), split_string("1.3", '.'));
    EXPECT_EQ(make_vector<std::string>("13"), split_string("13", '.'));
    EXPECT_EQ(make_vector<std::string>("1", " 2", " 3"), split_string("1, 2, 3", ','));
    EXPECT_EQ(make_vector<std::string>("1", " 2 3"), split_string("1, 2 3", ','));
}

}  // namespace unittest
