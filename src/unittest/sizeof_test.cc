// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "buffer_cache/alt.hpp"
#include "serializer/log/log_serializer.hpp"

namespace unittest {

TEST(SizeofTest, Sizes) {
    EXPECT_GT(1000u, sizeof(log_serializer_t));
    EXPECT_GT(1000u, sizeof(cache_t));
    EXPECT_GT(1000u, sizeof(buf_lock_t));
    EXPECT_GT(1000u, sizeof(txn_t));
}

TEST(SizeofTest, SerBuffer) {
    // These values depend on what sizeof(block_id_t) is.
    EXPECT_EQ(8u, sizeof(ls_buf_data_t));
    EXPECT_EQ(8u, sizeof(ser_buffer_t));
    EXPECT_EQ(8u, offsetof(ser_buffer_t, cache_data));
}

}  // namespace unittest
