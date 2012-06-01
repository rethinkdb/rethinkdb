#include <queue>

#include "unittest/gtest.hpp"

#include "containers/disk_backed_queue.hpp"
#include "arch/runtime/starter.hpp"

#define NUM_ELTS_IN_QUEUE 1000

namespace unittest {

void run_many_ints_test() {
    disk_backed_write_queue_t<int> queue("test");
    std::queue<int> ref_queue;

    for (int i = 0; i < NUM_ELTS_IN_QUEUE; ++i) {
        queue.push(i);
        ref_queue.push(i);
    }

    for (int i = 0; i < NUM_ELTS_IN_QUEUE; ++i) {
        EXPECT_FALSE(queue.empty());
        EXPECT_EQ(queue.pop(), ref_queue.front());
        ref_queue.pop();
    }
}

TEST(DiskBackedQueue, ManyInts) {
    run_in_thread_pool(&run_many_ints_test, 2);
}

#define NUM_BIG_ELTS_IN_QUEUE 100

void run_big_values_test() {
    disk_backed_write_queue_t<std::string> queue("test");
    std::queue<std::string> ref_queue;

    std::string val;
    val.resize(MEGABYTE, 'a');
    for (int i = 0; i < NUM_BIG_ELTS_IN_QUEUE; ++i) {
        queue.push(val);
        ref_queue.push(val);
    }

    for (int i = 0; i < NUM_BIG_ELTS_IN_QUEUE; ++i) {
        EXPECT_FALSE(queue.empty());
        EXPECT_EQ(queue.pop(), ref_queue.front());
        ref_queue.pop();
    }
}

TEST(DiskBackedQueue, BigVals) {
    run_in_thread_pool(&run_big_values_test, 2);
}

} //namespace unittest
