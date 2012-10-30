// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "buffer_cache/buffer_cache.hpp"
#include "serializer/config.hpp"


namespace unittest {

TEST(SizeofTest, Sizes) {
    EXPECT_GT(1000, sizeof(standard_serializer_t));
    EXPECT_GT(1000, sizeof(cache_t));
    EXPECT_GT(1000, sizeof(buf_lock_t));
    EXPECT_GT(1000, sizeof(transaction_t));

}



}  // namespace unittest
