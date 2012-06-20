#include "unittest/gtest.hpp"

#include "containers/printf_buffer.hpp"
#include "utils.hpp"

namespace unittest {

TEST(PrintfBufferTest, Buffering) {
    std::string str = "abcdefgh";
    printf_buffer_t<10> buf;
    buf.appendf(str.c_str());

    ASSERT_EQ(str, std::string(buf.c_str()));
    ASSERT_EQ(buf.c_str(), buf.data_);

    buf.appendf("i");
    ASSERT_EQ(str + "i", std::string(buf.c_str()));
    ASSERT_EQ(buf.c_str(), buf.data_);

    buf.appendf("j");
    ASSERT_EQ(str + "ij", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);

    buf.appendf("k");
    ASSERT_EQ(str + "ijk", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);

    buf.appendf("l");
    ASSERT_EQ(str + "ijkl", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);

    buf.appendf("m");
    ASSERT_EQ(str + "ijklm", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);

    buf.appendf("n");
    ASSERT_EQ(str + "ijklmn", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);

    buf.appendf("o");
    ASSERT_EQ(str + "ijklmno", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);

    buf.appendf("p");
    ASSERT_NE(buf.c_str(), buf.data_);
    ASSERT_EQ(str + "ijklmnop", std::string(buf.c_str()));

    buf.appendf("q");
    ASSERT_EQ(str + "ijklmnopq", std::string(buf.c_str()));
    ASSERT_NE(buf.c_str(), buf.data_);
}




}  // namespace unittest
