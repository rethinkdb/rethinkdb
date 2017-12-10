// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <algorithm>

#include "arch/timing.hpp"
#include "concurrency/pmap.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"
#include "utils.hpp"

namespace unittest {

const int waits = 10;
const int simultaneous = 2;
const int repeat = 10;

int wait_array[simultaneous][waits] =
    { { 1, 1, 2, 3, 5, 13, 20, 30, 40, 8 },
      { 5, 3, 2, 40, 30, 20, 8, 13, 1, 1 } };

#ifdef _WIN32
// Assuming a 15ms sleep resolution
const int max_error_ms = 16;
const int max_average_error_ms = 11;
#else
const int max_error_ms = 5;
const int max_average_error_ms = 2;
#endif

void walk_wait_times(int i, uint64_t *mse) {
    uint64_t se = 0;
    for (int j = 0; j < waits; ++j) {
        int expected_ms = wait_array[i][j];
        ticks_t t1 = get_ticks();
        nap(expected_ms);
        ticks_t t2 = get_ticks();
        int64_t actual_ns = t2.nanos - t1.nanos;
        int64_t error_ns = actual_ns - expected_ms * MILLION;

        EXPECT_LT(llabs(error_ns), max_error_ms * MILLION)
            << "failed to nap for " << expected_ms << "ms";

        se += error_ns * error_ns;
    }
    *mse += se / waits;
}

TPTEST(TimerTest, TestApproximateWaitTimes) {
    uint64_t mse_each[simultaneous] = {0};
    for (int i = 0; i < repeat; i++){
        pmap(simultaneous, [&](int j){ walk_wait_times(j, &mse_each[j]); });
    }
    int64_t mse = 0;
    for (int i = 0; i < simultaneous; i++) {
        mse += mse_each[i] / repeat;
    }
    mse /= simultaneous;
    EXPECT_LT(sqrt(mse) / MILLION, max_average_error_ms)
        << "Average timer error too high";
}

TPTEST(TimerTest, TestRepeatingTimer) {
    int64_t first_ticks = get_ticks().nanos;
    int count = 0;
    repeating_timer_t timer(30, [&]() {
        ++count;
        int64_t ticks = get_ticks().nanos;
        int64_t diff = ticks - first_ticks;
        EXPECT_LT(std::abs(diff - 30 * MILLION * count), max_error_ms * MILLION);
    });
    nap(100);
}

TPTEST(TimerTest, TestChangeInterval) {
    ticks_t first_ticks = get_ticks();
    int count = 0;
    int64_t expected[] = { 5, 10, 20, 40, 65 };
    int64_t naps[] = {0,  0,  0,  25, 0};
    int64_t ms[] = { 10, 20, 30, 10, 50};
    scoped_ptr_t<repeating_timer_t> timer;
    timer = make_scoped<repeating_timer_t>(10, [&]() {
        coro_t::spawn_now_dangerously([&]() {
            ASSERT_LT(count, 5);
            ticks_t ticks = get_ticks();
            int64_t diff = ticks.nanos - first_ticks.nanos;
            EXPECT_LT(std::abs(diff - expected[count] * MILLION),
                      max_error_ms * MILLION);
            nap(naps[count]);
            timer->change_interval(ms[count]);
            ++count;
        });
    });
    timer->change_interval(5);
    nap(70);
}


}  // namespace unittest
