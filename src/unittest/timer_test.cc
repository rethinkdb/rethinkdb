// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/timing.hpp"
#include "concurrency/pmap.hpp"
#include "unittest/unittest_utils.hpp"
#include "utils.hpp"

namespace unittest {

int wait_array[2][10] = { { 1, 1, 2, 3, 5, 13, 20, 30, 40, 8 },
                          { 5, 3, 2, 40, 30, 20, 8, 13, 1, 1 } };

void walk_wait_times(int i) {
    ticks_t t = get_ticks();
    for (int j = 0; j < 10; ++j) {
        nap(wait_array[i][j]);
        const ticks_t t2 = get_ticks();
        const int64_t diff = static_cast<int64_t>(t2) - static_cast<int64_t>(t);
        // Asserts that we're off by less than two milliseconds.
        ASSERT_LT(llabs(diff - wait_array[i][j] * MILLION), 2 * MILLION);
        t = t2;
    }

}

TPTEST(TimerTest, TestApproximateWaitTimes) {
    pmap(2, walk_wait_times);
}


}  // namespace unittest
