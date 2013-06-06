// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "containers/printf_buffer.hpp"
#include "utils.hpp"

namespace unittest {

TEST(PrintfBufferTest, Buffering) {
    std::string str(printf_buffer_t::STATIC_DATA_SIZE - 2, 'a');
    printf_buffer_t buf;
    const char *const buf_static_data = buf.c_str();

    buf.appendf("%s", str.c_str());

    ASSERT_EQ(str, std::string(buf.c_str()));
    ASSERT_EQ(buf.c_str(), buf_static_data);

    buf.appendf("i");
    ASSERT_EQ(str + "i", std::string(buf.c_str()));
    ASSERT_EQ(buf.c_str(), buf_static_data);

    buf.appendf("j");
    ASSERT_EQ(str + "ij", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("k");
    ASSERT_EQ(str + "ijk", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("l");
    ASSERT_EQ(str + "ijkl", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("m");
    ASSERT_EQ(str + "ijklm", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("n");
    ASSERT_EQ(str + "ijklmn", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("o");
    ASSERT_EQ(str + "ijklmno", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("p");
    ASSERT_EQ(str + "ijklmnop", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);

    buf.appendf("q");
    ASSERT_EQ(str + "ijklmnopq", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf_static_data);
}




}  // namespace unittest
