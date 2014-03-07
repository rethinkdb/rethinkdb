/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <algorithm>

#include <boost/heap/priority_queue.hpp>

#include "common_heap_tests.hpp"
#include "stable_heap_tests.hpp"
#include "merge_heap_tests.hpp"

template <bool stable>
void run_common_priority_queue_tests(void)
{
    typedef boost::heap::priority_queue<int, boost::heap::stable<stable> > pri_queue;
    BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<pri_queue>));

    run_concept_check<pri_queue>();
    run_common_heap_tests<pri_queue>();
    run_iterator_heap_tests<pri_queue>();
    run_copyable_heap_tests<pri_queue>();
    run_moveable_heap_tests<pri_queue>();
    run_merge_tests<pri_queue>();

    if (stable) {
        typedef boost::heap::priority_queue<q_tester, boost::heap::stable<stable> > stable_pri_queue;
        run_stable_heap_tests<stable_pri_queue>();
    }
}

BOOST_AUTO_TEST_CASE( std_pri_queue_test )
{
    run_common_priority_queue_tests<false>();
    run_common_priority_queue_tests<true>();
}

BOOST_AUTO_TEST_CASE( std_pri_queue_leak_test )
{
    typedef boost::heap::priority_queue<boost::shared_ptr<int> > pri_queue;
    run_leak_check_test<pri_queue>();
}
