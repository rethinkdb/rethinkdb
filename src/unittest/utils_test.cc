// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "utils.hpp"
#include "arch/address.hpp"
#include "arch/runtime/runtime.hpp"
#include "mock/unittest_utils.hpp"

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

    ASSERT_EQ(0, strtou64_strict(test1, &end, 10));
    ASSERT_EQ(1024, strtou64_strict(test2, &end, 10));
    ASSERT_EQ(0, strtou64_strict(test3, &end, 10));
    ASSERT_EQ(123, strtou64_strict(test4, &end, 10));
    ASSERT_FALSE(strncmp("lskdjf", end, 6));

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
    struct timespec time = {1335301122, 1234};
    std::string formatted = format_time(time);
    EXPECT_EQ("2012-04-24T13:58:42.000001234", formatted);
    struct timespec parsed = parse_time(formatted);
    EXPECT_EQ(time.tv_sec, parsed.tv_sec);
    EXPECT_EQ(time.tv_nsec, parsed.tv_nsec);
}

TEST(UtilsTest, SizedStrcmp)
{
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

void run_ip_address_test() {
    std::vector<ip_address_t> ips = ip_address_t::from_hostname("111.112.113.114");
    EXPECT_EQ("111.112.113.114", ips[0].as_dotted_decimal());
}

TEST(UtilsTest, IPAddress)
{
    // Since ip_address_t may block, it requires a thread_pool_t to exist
    mock::run_in_thread_pool(&run_ip_address_test);
}

}  // namespace unittest
