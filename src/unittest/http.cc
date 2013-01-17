// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <ctime>

#include "unittest/gtest.hpp"
#include "http/http.hpp"

namespace unittest {

TEST(Http, FormatTime) {

    std::string formatted_time = http_format_date(1356998463);

    EXPECT_EQ("Tue, 01 Jan 2013 00:01:03 GMT", formatted_time);

}

}
