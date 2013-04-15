#include <limits.h>

#include "unittest/gtest.hpp"
#include "clustering/administration/main/json_import.hpp"
#include "containers/bitset.hpp"
#include "http/json.hpp"
#include "stl_utils.hpp"

TEST(ImportTest, TestSplitBuf) {
    bitset_t bitset(static_cast<int>(UCHAR_MAX) + 1);
    separators_to_bitset("\n", &bitset);

    const char *buf = "a\nb\nc";

    const std::vector<std::string> splits = split_buf(bitset, buf, strlen(buf));
    const std::vector<std::string> expected = make_vector<std::string>("a", "b", "c");

    ASSERT_EQ(expected, splits);
}

TEST(ImportTest, TestNextJson) {
    // Test that next_json (as well as the csv_to_json_importer_t
    // constructor) doesn't skip any rows.
    std::vector<std::string> lines = make_vector<std::string>("#a, b",
                                                              "c, d",
                                                              "e, f");
    csv_to_json_importer_t importer("\n", lines);

    scoped_cJSON_t tmp;
    const bool res1 = importer.next_json(&tmp);
    ASSERT_TRUE(res1);
    tmp.reset(NULL);
    const bool res2 = importer.next_json(&tmp);
    ASSERT_TRUE(res2);
    tmp.reset(NULL);
    const bool res3 = importer.next_json(&tmp);
    ASSERT_FALSE(res3);
}
