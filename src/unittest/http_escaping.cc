#include "unittest/gtest.hpp"

#include "http/http.hpp"

namespace unittest {

TEST(HTTPEscaping, HTTPEscaping) {
    EXPECT_EQ("abc", percent_escaped_string("abc"));

    EXPECT_EQ("Hello%2C%20world%21", percent_escaped_string("Hello, world!"));

    EXPECT_EQ("Hello, world!", percent_unescaped_string("Hello%2C%20world%21"));

    EXPECT_THROW(percent_unescaped_string("%"), std::runtime_error);

    EXPECT_THROW(percent_unescaped_string("%6y"), std::runtime_error);

    EXPECT_THROW(percent_unescaped_string("!"), std::runtime_error);
}

}//namespace unittest
