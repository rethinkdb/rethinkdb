// Copyright 2010-2015 RethinkDB, all rights reserved.

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "concurrency/auto_drainer.hpp"
#include "config/args.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"
#include "utils.hpp"

namespace unittest {

TEST(CoroutinesTest, NotifyOrdered) {
    // Tests that both `spawn_later_ordered` as well as `yield_ordered` preserve
    // ordering (both of them internally use `notify_later_ordered`).
    int num_coros = 100;
    run_in_thread_pool([&]() {
        int c = 0;
        for (int i = 0; i < num_coros; ++i) {
            // Priorities should be ignored when using ordered notifications. Make sure
            // that's the case.
            int my_priority =
                i % (MESSAGE_SCHEDULER_MAX_PRIORITY - MESSAGE_SCHEDULER_MIN_PRIORITY)
                    + MESSAGE_SCHEDULER_MIN_PRIORITY;
            with_priority_t p(my_priority);
            ASSERT_TRUE(coro_t::self()->get_priority() == my_priority);
            coro_t::spawn_later_ordered([&c, i, my_priority]() {
                ASSERT_TRUE(coro_t::self()->get_priority() == my_priority);
                coro_t::yield_ordered();
                ASSERT_EQ(i, c);
                ++c;
            });
        }
        coro_t::yield_ordered();
        coro_t::yield_ordered();
        // As a side-effect, this also makes sure that `spawn_later_ordered` doesn't
        // introduce any additional wait cycles. It probably wouldn't be critical if it
        // did, but for now we assume that all coroutines spawned above have terminated
        // after the second `yield_ordered()`.
        ASSERT_EQ(c, num_coros);
    });
}

TEST(CoroutinesTest, OnThreadOrdering) {
    // Tests that `on_thread_t` preserves ordering
    int num_threads = 10;
    int num_coros = 1000; // 100 per thread
    run_in_thread_pool([&]() {
        auto_drainer_t drainer;
        std::vector<int> c(num_threads, 0);
        std::vector<int> return_c(num_threads, 0);
        for (int i = 0; i < num_coros; ++i) {
            int my_priority =
                i % (MESSAGE_SCHEDULER_MAX_PRIORITY - MESSAGE_SCHEDULER_MIN_PRIORITY)
                    + MESSAGE_SCHEDULER_MIN_PRIORITY;
            // Make sure that different priorities don't lead to reordering with ordered
            // operations.
            with_priority_t p(my_priority);
            ASSERT_TRUE(coro_t::self()->get_priority() == my_priority);
            auto_drainer_t::lock_t lock(&drainer);
            coro_t::spawn_later_ordered(
            [&c, &return_c, i, num_threads, my_priority, lock]() {
                ASSERT_TRUE(coro_t::self()->get_priority() == my_priority);
                threadnum_t my_thread(i % num_threads);
                {
                    on_thread_t t(my_thread);
                    ASSERT_EQ(get_thread_id(), my_thread);
                    ASSERT_TRUE(coro_t::self()->get_priority() == my_priority);
                    // We should arrive on the new thread in the right order
                    ASSERT_EQ(i / num_threads, c[my_thread.threadnum]);
                    ++c[i % num_threads];
                }
                // We should return to the old thread in the right order
                ASSERT_EQ(i / num_threads, return_c[my_thread.threadnum]);
                ++return_c[my_thread.threadnum];
            });
        }
        drainer.drain();
        for (int i = 0; i < num_threads; ++i) {
            ASSERT_EQ(c[i], num_coros / num_threads);
            ASSERT_EQ(return_c[i], num_coros / num_threads);
        }
    }, num_threads);
}

TEST(CoroutinesTest, NotifyNow) {
    // Test that `spawn_now_dangerously` doesn't block`
    run_in_thread_pool([&]() {
        // The adversary will try to get run before the `spawn_now_dangerously` coroutine.
        bool adversary_ran = false;
        bool check_ran = false;
        coro_t::spawn_later_ordered([&]() {
            adversary_ran = true;
        });
        {
            ASSERT_FINITE_CORO_WAITING;
            coro_t::spawn_now_dangerously([&]() {
                check_ran = true;
                ASSERT_FALSE(adversary_ran);
            });
            ASSERT_FALSE(adversary_ran);
            ASSERT_TRUE(check_ran);
        }
        coro_t::yield_ordered();
        ASSERT_TRUE(adversary_ran);
    });
}

}   /* namespace unittest */
