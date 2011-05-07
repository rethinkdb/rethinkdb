#include "replication/delete_queue.hpp"

#include "unittest/gtest.hpp"

namespace unittest {

TEST(DeleteQueueTest, Offsets) {
    ASSERT_EQ(12, sizeof(replication::delete_queue::t_and_o));
    ASSERT_EQ(0, offsetof(replication::delete_queue::t_and_o, timestamp));
    ASSERT_EQ(4, offsetof(replication::delete_queue::t_and_o, offset));

    ASSERT_EQ(4, sizeof(replication::delete_queue_block_t));
}

}  // namespace unittest
