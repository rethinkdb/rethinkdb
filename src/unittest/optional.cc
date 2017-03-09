#include "containers/optional.hpp"

#include "unittest/gtest.hpp"

namespace unittest {

TEST(OptionalTest, Test) {
    optional<std::string> x("hello");
    optional<std::string> y;
    ASSERT_TRUE(x.has_value());
    ASSERT_FALSE(y.has_value());
    ASSERT_EQ("hello", *x);
    y = x;
    ASSERT_TRUE(y.has_value());
    ASSERT_EQ("hello", *y);
}


}  // namespace unittest
