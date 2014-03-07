/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "common_heap_tests.hpp"
#include <boost/heap/heap_merge.hpp>

#define GENERATE_TEST_DATA(INDEX)                           \
    test_data data = make_test_data(test_size, 0, 1);       \
    std::random_shuffle(data.begin(), data.end());          \
                                                            \
    test_data data_q (data.begin(), data.begin() + INDEX);  \
    test_data data_r (data.begin() + INDEX, data.end());    \
                                                            \
    std::stable_sort(data.begin(), data.end());


template <typename pri_queue>
struct pri_queue_test_merge
{
    static void run(void)
    {
        for (int i = 0; i != test_size; ++i) {
            pri_queue q, r;
            GENERATE_TEST_DATA(i);

            fill_q(q, data_q);
            fill_q(r, data_r);

            q.merge(r);

            BOOST_REQUIRE(r.empty());
            check_q(q, data);
        }
    }
};


template <typename pri_queue1, typename pri_queue2>
struct pri_queue_test_heap_merge
{
    static void run (void)
    {
        for (int i = 0; i != test_size; ++i) {
            pri_queue1 q;
            pri_queue2 r;
            GENERATE_TEST_DATA(i);

            fill_q(q, data_q);
            fill_q(r, data_r);

            boost::heap::heap_merge(q, r);

            BOOST_REQUIRE(r.empty());
            check_q(q, data);
        }
    }
};



template <typename pri_queue>
void run_merge_tests(void)
{
    boost::mpl::if_c<pri_queue::is_mergable,
                     pri_queue_test_merge<pri_queue>,
                     dummy_run
                    >::type::run();

    pri_queue_test_heap_merge<pri_queue, pri_queue>::run();
}
