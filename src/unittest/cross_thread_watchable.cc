
#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "arch/timing.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "mock/unittest_utils.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

bool equals(int a, int b) { return a == b; }

void runCrossThreadWatchabletest() {
    boost::scoped_ptr<watchable_variable_t<int> > watchable;
    boost::scoped_ptr<cross_thread_watchable_variable_t<int> > ctw;
    {
        on_thread_t thread_switcher(0);
        watchable.reset(new watchable_variable_t<int>(0));

        ctw.reset(new cross_thread_watchable_variable_t<int>(watchable->get_watchable(), 1));
    }

    int i, expected_value = 0;
    try {
        for (i = 1; i < 100; ++i) {
            {
                on_thread_t switcher(0);
                watchable->set_value(i);
                expected_value = i;
            }
            {
                on_thread_t switcher(1);
                signal_timer_t timer(5000);
                ctw->get_watchable()->run_until_satisfied(boost::bind(&equals, expected_value, _1), &timer);
            }
        }

        for (; i < 1000; ++i) {
            {
                on_thread_t switcher(0);
                int step = i + 25;
                for (; i < step; ++i) {
                    watchable->set_value(i);
                    expected_value = i;
                }
            }
            {
                on_thread_t switcher(1);
                signal_timer_t timer(5000);
                ctw->get_watchable()->run_until_satisfied(boost::bind(&equals, expected_value, _1), &timer);
            }
        }
    } catch (const interrupted_exc_t &) {
        printf("%d\n", i);
        ASSERT_TRUE(false); //Probable a better way to just error but whatever
    }
}

TEST(CrossThreadWatchable, CrossThreadWatchableTest) {
    mock::run_in_thread_pool(&runCrossThreadWatchabletest, 2);
}

} //namespace unittest
