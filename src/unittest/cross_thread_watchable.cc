// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "containers/scoped.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

bool equals(int a, int b) { return a == b; }

TPTEST(CrossThreadWatchable, CrossThreadWatchableTest) {
    scoped_ptr_t<watchable_variable_t<int> > watchable;
    scoped_ptr_t<cross_thread_watchable_variable_t<int> > ctw;
    {
        on_thread_t thread_switcher(threadnum_t(0));
        watchable.init(new watchable_variable_t<int>(0));

        ctw.init(new cross_thread_watchable_variable_t<int>(watchable->get_watchable(), threadnum_t(1)));
    }

    int i;
    int expected_value = 0;
    try {
        for (i = 1; i < 100; ++i) {
            {
                on_thread_t switcher(threadnum_t(0));
                watchable->set_value(i);
                expected_value = i;
            }
            {
                on_thread_t switcher(threadnum_t(1));
                signal_timer_t timer;
                timer.start(5000);
                ctw->get_watchable()->run_until_satisfied(
                        boost::bind(&equals, expected_value, _1), &timer);
            }
        }

        for (; i < 1000; ++i) {
            {
                on_thread_t switcher(threadnum_t(0));
                int step = i + 25;
                for (; i < step; ++i) {
                    watchable->set_value(i);
                    expected_value = i;
                }
            }
            {
                on_thread_t switcher(threadnum_t(1));
                signal_timer_t timer;
                timer.start(5000);
                ctw->get_watchable()->run_until_satisfied(
                        boost::bind(&equals, expected_value, _1), &timer);
            }
        }
    } catch (const interrupted_exc_t &) {
        printf("%d\n", i);
        ASSERT_TRUE(false); //Probable a better way to just error but whatever
    }
}

} //namespace unittest
