/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#define BOOST_TEST_MAIN
#ifdef BOOST_HEAP_INCLUDE_TESTS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif


#include <algorithm>

#include <boost/heap/skew_heap.hpp>

#include "common_heap_tests.hpp"
#include "stable_heap_tests.hpp"
#include "mutable_heap_tests.hpp"
#include "merge_heap_tests.hpp"

template <bool stable, bool constant_time_size, bool store_parent_pointer>
void run_skew_heap_test(void)
{
    typedef boost::heap::skew_heap<int, boost::heap::stable<stable>,
                                        boost::heap::compare<std::less<int> >,
                                        boost::heap::allocator<std::allocator<int> >,
                                        boost::heap::constant_time_size<constant_time_size>,
                                        boost::heap::store_parent_pointer<store_parent_pointer>
                                  > pri_queue;

    BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<pri_queue>));
    BOOST_CONCEPT_ASSERT((boost::heap::MergablePriorityQueue<pri_queue>));

    run_common_heap_tests<pri_queue>();
    run_iterator_heap_tests<pri_queue>();
    run_copyable_heap_tests<pri_queue>();
    run_moveable_heap_tests<pri_queue>();

    run_merge_tests<pri_queue>();

    pri_queue_test_ordered_iterators<pri_queue>();

    if (stable) {
        typedef boost::heap::skew_heap<q_tester, boost::heap::stable<stable>,
                                       boost::heap::constant_time_size<constant_time_size>,
                                       boost::heap::store_parent_pointer<store_parent_pointer>
                                      > stable_pri_queue;
        run_stable_heap_tests<stable_pri_queue>();
    }
}

template <bool stable, bool constant_time_size>
void run_skew_heap_mutable_test(void)
{
    typedef boost::heap::skew_heap<int, boost::heap::stable<stable>, boost::heap::mutable_<true>,
                                   boost::heap::compare<std::less<int> >,
                                   boost::heap::allocator<std::allocator<int> >,
                                   boost::heap::constant_time_size<constant_time_size>
                                  > pri_queue;

    BOOST_CONCEPT_ASSERT((boost::heap::MutablePriorityQueue<pri_queue>));
    BOOST_CONCEPT_ASSERT((boost::heap::MergablePriorityQueue<pri_queue>));

    run_common_heap_tests<pri_queue>();
    run_iterator_heap_tests<pri_queue>();
    run_copyable_heap_tests<pri_queue>();
    run_moveable_heap_tests<pri_queue>();

    run_merge_tests<pri_queue>();

    run_mutable_heap_tests<pri_queue >();

    run_ordered_iterator_tests<pri_queue>();

    if (stable) {
        typedef boost::heap::skew_heap<q_tester, boost::heap::stable<stable>, boost::heap::mutable_<true>,
                                               boost::heap::constant_time_size<constant_time_size>
                                              > stable_pri_queue;
        run_stable_heap_tests<stable_pri_queue>();
    }
}


BOOST_AUTO_TEST_CASE( skew_heap_test )
{
    run_skew_heap_test<false, false, true>();
    run_skew_heap_test<false, true, true>();
    run_skew_heap_test<true, false, true>();
    run_skew_heap_test<true, true, true>();

    run_skew_heap_test<false, false, false>();
    run_skew_heap_test<false, true, false>();
    run_skew_heap_test<true, false, false>();
    run_skew_heap_test<true, true, false>();

    RUN_EMPLACE_TEST(skew_heap);
}

BOOST_AUTO_TEST_CASE( skew_heap_mutable_test )
{
    run_skew_heap_mutable_test<false, false>();
    run_skew_heap_mutable_test<false, true>();
    run_skew_heap_mutable_test<true, false>();
    run_skew_heap_mutable_test<true, true>();
}

BOOST_AUTO_TEST_CASE( skew_heap_compare_lookup_test )
{
    typedef boost::heap::skew_heap<int,
                                   boost::heap::compare<less_with_T>,
                                   boost::heap::allocator<std::allocator<int> > > pri_queue;
    run_common_heap_tests<pri_queue>();
}


BOOST_AUTO_TEST_CASE( skew_heap_leak_test )
{
    typedef boost::heap::skew_heap<boost::shared_ptr<int> > pri_queue;
    run_leak_check_test<pri_queue>();
}
