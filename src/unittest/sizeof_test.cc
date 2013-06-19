// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "buffer_cache/buffer_cache.hpp"
#include "serializer/config.hpp"


namespace unittest {

TEST(SizeofTest, Sizes) {
    EXPECT_GT(1000u, sizeof(standard_serializer_t));
    EXPECT_GT(1000u, sizeof(cache_t));
    EXPECT_GT(1000u, sizeof(buf_lock_t));
    EXPECT_GT(1000u, sizeof(transaction_t));
}

TEST(SizeofTest, SerializerBuffer) {
    EXPECT_EQ(12u, sizeof(ls_buf_data_t));
    EXPECT_EQ(12u, sizeof(serializer_buffer_t));
    EXPECT_EQ(12u, offsetof(serializer_buffer_t, cache_data));
}



}  // namespace unittest
