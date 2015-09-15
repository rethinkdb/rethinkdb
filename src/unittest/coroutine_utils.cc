// Copyright 2010-2015 RethinkDB, all rights reserved.

#include <stdexcept>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/timing.hpp"
#include "concurrency/cond_var.hpp"
#include "time.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

void run_in_coro(const std::function<void()> &fun) {
    run_in_thread_pool([&]() {
        // `run_in_thread_pool` already spawns a coroutine for us.
        ASSERT_NE(coro_t::self(), nullptr);
        fun();
    });
}

TEST(CoroutineUtilsTest, WithEnoughStackNoSpawn) {
    int res = 0;
    run_in_coro([&]() {
        // This should execute directly
        ASSERT_NO_CORO_WAITING;
        res = call_with_enough_stack<int>([] () {
            return 5;
        }, 1);
    });
    EXPECT_EQ(res, 5);
}

TEST(CoroutineUtilsTest, WithEnoughStackNonBlocking) {
    int res = 0;
    run_in_coro([&]() {
        ASSERT_FINITE_CORO_WAITING;
        // `COROUTINE_STACK_SIZE` forces a coroutine to be spawned
        res = call_with_enough_stack<int>([] () {
            return 5;
        }, COROUTINE_STACK_SIZE);
    });
    EXPECT_EQ(res, 5);
}

TEST(CoroutineUtilsTest, WithEnoughStackBlocking) {
    int res = 0;
    run_in_coro([&]() {
        // `COROUTINE_STACK_SIZE` forces a coroutine to be spawned
        res = call_with_enough_stack<int>([] () {
            nap(5);
            return 5;
        }, COROUTINE_STACK_SIZE);
    });
    EXPECT_EQ(res, 5);
}

TEST(CoroutineUtilsTest, WithEnoughStackNoCoro) {
    // call_with_enough_stack should still be usable if we are not in a coroutine
    // (though it doesn't do much in that case).
    int res = 0;
    run_in_thread_pool([&]() {
        struct test_message_t : public linux_thread_message_t {
            void on_thread_switch() {
                ASSERT_EQ(coro_t::self(), nullptr);
                *out = call_with_enough_stack<int>([] () {
                    return 5;
                }, 1);
                done_cond.pulse();
            }
            int *out;
            cond_t done_cond;
        };
        test_message_t thread_msg;
        thread_msg.out = &res;
        call_later_on_this_thread(&thread_msg);
        thread_msg.done_cond.wait();
    });
    EXPECT_EQ(res, 5);
}

TEST(CoroutineUtilsTest, WithEnoughStackException) {
    bool got_exception = false;
    run_in_coro([&]() {
        try {
            // `COROUTINE_STACK_SIZE` forces a coroutine to be spawned
            call_with_enough_stack([] () {
                throw std::runtime_error("This is a test exception");
            }, COROUTINE_STACK_SIZE);
        } catch (const std::exception &) {
            got_exception = true;
        }
    });
    EXPECT_TRUE(got_exception);
}

// This is not really a unit test, but a micro benchmark to test the overhead
// of call_with_enough_stack. No need to run this in debug mode.
#ifdef NDEBUG
NOINLINE int test_function() {
    return 1;
}
NOINLINE int test_function_blocking() {
    coro_t::yield();
    return 1;
}

TEST(CoroutineUtilsTest, WithEnoughStackBenchmark) {
    run_in_coro([&]() {
        const int NUM_REPETITIONS = 1000000;

        // Get the base line first
        printf("Getting base line timings.\n");
        int sum = 0;
        ticks_t start_ticks = get_ticks();
        for(int i = 0; i < NUM_REPETITIONS; ++i) {
            sum += test_function();
        }
        double dur_base = ticks_to_secs(get_ticks() - start_ticks);
        EXPECT_EQ(sum, NUM_REPETITIONS);

        sum = 0;
        start_ticks = get_ticks();
        for(int i = 0; i < NUM_REPETITIONS; ++i) {
            sum += test_function_blocking();
        }
        double dur_base_blocking = ticks_to_secs(get_ticks() - start_ticks);
        EXPECT_EQ(sum, NUM_REPETITIONS);

        {
            printf("Test 1: call_with_enough_stack without spawning: ");
            sum = 0;
            start_ticks = get_ticks();
            for(int i = 0; i < NUM_REPETITIONS; ++i) {
                // Use lambdas because that's what we're usually going to use in
                // our code.
                sum += call_with_enough_stack<int>([&] () {
                    return test_function();
                }, 1);
            }
            double dur = ticks_to_secs(get_ticks() - start_ticks);
            EXPECT_EQ(sum, NUM_REPETITIONS);
            printf("%f us overhead\n",
                   (dur - dur_base) / NUM_REPETITIONS * 1000000);
        }
        {
            printf("Test 2: call_with_enough_stack with spawning: ");
            sum = 0;
            start_ticks = get_ticks();
            for(int i = 0; i < NUM_REPETITIONS; ++i) {
                // Use lambdas because that's what we're usually going to use in
                // our code.
                sum += call_with_enough_stack<int>([&] () {
                    return test_function();
                }, COROUTINE_STACK_SIZE);
            }
            double dur = ticks_to_secs(get_ticks() - start_ticks);
            EXPECT_EQ(sum, NUM_REPETITIONS);
            printf("%f us overhead\n",
                   (dur - dur_base) / NUM_REPETITIONS * 1000000);
        }
        {
            printf("Test 3: call_with_enough_stack with blocking: ");
            sum = 0;
            start_ticks = get_ticks();
            for(int i = 0; i < NUM_REPETITIONS; ++i) {
                // Use lambdas because that's what we're usually going to use in
                // our code.
                sum += call_with_enough_stack<int>([&] () {
                    return test_function_blocking();
                }, COROUTINE_STACK_SIZE);
            }
            double dur = ticks_to_secs(get_ticks() - start_ticks);
            EXPECT_EQ(sum, NUM_REPETITIONS);
            printf("%f us overhead\n",
                   (dur - dur_base_blocking) / NUM_REPETITIONS * 1000000);
        }
    });
}
#endif

}   /* namespace unittest */
