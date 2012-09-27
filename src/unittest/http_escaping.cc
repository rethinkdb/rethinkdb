#include "unittest/gtest.hpp"

#include "http/http.hpp"

namespace unittest {

TEST(HTTPEscaping, HTTPEscaping) {
    EXPECT_EQ("abc", percent_escaped_string("abc"));

    EXPECT_EQ("Hello%2C%20world%21", percent_escaped_string("Hello, world!"));

    std::string out;
    EXPECT_TRUE(percent_unescape_string("Hello%2C%20world%21", &out));
    EXPECT_EQ("Hello, world!", out);
    out.clear();

    EXPECT_FALSE(percent_unescape_string("%", &out));
    EXPECT_EQ("", out);
    EXPECT_FALSE(percent_unescape_string("%6y", &out));
    EXPECT_EQ("", out);
    EXPECT_FALSE(percent_unescape_string("!", &out));
    EXPECT_EQ("", out);
}

}//namespace unittest
