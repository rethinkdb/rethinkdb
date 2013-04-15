#include <limits.h>

#include "unittest/gtest.hpp"
#include "clustering/administration/main/json_import.hpp"
#include "containers/bitset.hpp"
#include "stl_utils.hpp"

TEST(ImportTest, TestSplitBuf) {
    bitset_t bitset(static_cast<int>(UCHAR_MAX) + 1);
    separators_to_bitset("\n", &bitset);

    const char *buf = "a\nb\nc";

    const std::vector<std::string> splits = split_buf(bitset, buf, strlen(buf));
    const std::vector<std::string> expected = make_vector<std::string>("a", "b", "c");

    ASSERT_EQ(expected, splits);
}
