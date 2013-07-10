#include "containers/auth_key.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

TEST(AuthKeyTest, TestEquals) {
    auth_key_t x;
    bool x_success = x.assign_value("hello");
    ASSERT_TRUE(x_success);

    auth_key_t y;
    bool y_success = y.assign_value("hello");
    ASSERT_TRUE(y_success);

    auth_key_t z;
    bool z_success = z.assign_value("hellO");
    ASSERT_TRUE(z_success);

    ASSERT_TRUE(timing_sensitive_equals(x, y));
    ASSERT_FALSE(timing_sensitive_equals(x, z));
    ASSERT_TRUE(x == y);
    ASSERT_FALSE(x == z);
}



}  // namespace unittest
