#include <queue>

#include "arch/io/disk.hpp"
#include "arch/timing.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/disk_backed_queue.hpp"
#include "mock/unittest_utils.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

const char *const DBQ_TEST_DIRECTORY = "test_disk_backed_queue";

void run_many_ints_test() {
    static const int NUM_ELTS_IN_QUEUE = 1000;
    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);
    disk_backed_queue_t<int> queue(io_backender.get(), DBQ_TEST_DIRECTORY, &get_global_perfmon_collection());
    std::queue<int> ref_queue;

    for (int i = 0; i < NUM_ELTS_IN_QUEUE; ++i) {
        queue.push(i);
        ref_queue.push(i);
    }

    for (int i = 0; i < NUM_ELTS_IN_QUEUE; ++i) {
        EXPECT_FALSE(queue.empty());
        int x;
        queue.pop(&x);
        EXPECT_EQ(ref_queue.front(), x);
        ref_queue.pop();
    }
}

TEST(DiskBackedQueue, ManyInts) {
    mock::run_in_thread_pool(&run_many_ints_test, 2);
}

void run_big_values_test() {
    static const int NUM_BIG_ELTS_IN_QUEUE = 100;
    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    disk_backed_queue_t<std::string> queue(io_backender.get(), DBQ_TEST_DIRECTORY, &get_global_perfmon_collection());
    std::queue<std::string> ref_queue;

    std::string val;
    val.resize(MEGABYTE, 'a');
    for (int i = 0; i < NUM_BIG_ELTS_IN_QUEUE; ++i) {
        queue.push(val);
        ref_queue.push(val);
    }

    for (int i = 0; i < NUM_BIG_ELTS_IN_QUEUE; ++i) {
        EXPECT_FALSE(queue.empty());
        std::string x;
        queue.pop(&x);
        EXPECT_EQ(ref_queue.front(), x);
        ref_queue.pop();
    }
}

TEST(DiskBackedQueue, BigVals) {
    mock::run_in_thread_pool(&run_big_values_test, 2);
}

static void randomly_delay(int, signal_t *) {
    nap(randint(100));
}

void run_concurrent_test() {
    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    disk_backed_queue_wrapper_t<int> queue(io_backender.get(), DBQ_TEST_DIRECTORY, &get_global_perfmon_collection());
    boost_function_callback_t<int> callback(&randomly_delay);
    coro_pool_t<int> coro_pool(10, &queue, &callback);
    for (int i = 0; i < 1000; i++) {
        queue.push(i);
        nap(randint(10));
    }
    nap(1000);
}

TEST(DiskBackedQueue, Concurrent) {
    mock::run_in_thread_pool(&run_concurrent_test, 1);
}

} //namespace unittest
