#include "serializer/log/data_block_manager.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

TEST(DBMTest, ReadAheadInterval) {
    int64_t offset;
    int64_t end_offset;

    std::vector<uint32_t> boundaries = { 0, 20, 40, 60, 80, 100 };

    unaligned_read_ahead_interval(20, 20, 100, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(0, offset);
    ASSERT_EQ(60, end_offset);

    boundaries = { 0, 20, 40, 80, 100 };
    unaligned_read_ahead_interval(20, 20, 100, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(0, offset);
    ASSERT_EQ(80, end_offset);

    boundaries = { 0, 20, 35, 45, 55, 65, 80, 100 };
    unaligned_read_ahead_interval(55, 7, 100, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(0, offset);
    ASSERT_EQ(65, end_offset);

    unaligned_read_ahead_interval(55, 3, 100, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(0, offset);
    ASSERT_EQ(65, end_offset);

    unaligned_read_ahead_interval(55, 3, 100, 1200, boundaries, &offset, &end_offset);
    ASSERT_EQ(0, offset);
    ASSERT_EQ(100, end_offset);

    unaligned_read_ahead_interval(55, 3, 1000, 1200, boundaries, &offset, &end_offset);
    ASSERT_EQ(0, offset);
    ASSERT_EQ(100, end_offset);

    unaligned_read_ahead_interval(65, 3, 100, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(65, offset);
    ASSERT_EQ(100, end_offset);

    unaligned_read_ahead_interval(65, 3, 120, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(65, offset);
    ASSERT_EQ(100, end_offset);

    unaligned_read_ahead_interval(65, 3, 170, 60, boundaries, &offset, &end_offset);
    ASSERT_EQ(65, offset);
    ASSERT_EQ(100, end_offset);
}



}  // namespace unittest
