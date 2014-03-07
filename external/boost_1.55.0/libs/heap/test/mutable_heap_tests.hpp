/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// random uses boost.fusion, which clashes with boost.test
//#define USE_BOOST_RANDOM

#ifdef USE_BOOST_RANDOM
#include <boost/random.hpp>
#else
#include <cstdlib>
#endif

#include "common_heap_tests.hpp"


#define PUSH_WITH_HANDLES(HANDLES, Q, DATA)                 \
    std::vector<typename pri_queue::handle_type> HANDLES;   \
                                                            \
    for (unsigned int k = 0; k != data.size(); ++k)         \
        HANDLES.push_back(Q.push(DATA[k]));



template <typename pri_queue>
void pri_queue_test_update_decrease(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        *handles[i] = -1;
        data[i] = -1;
        q.update(handles[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_update_decrease_function(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        data[i] = -1;
        q.update(handles[i], -1);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_update_function_identity(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        q.update(handles[i], data[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_update_shuffled(void)
{
    pri_queue q;
    test_data data = make_test_data(test_size);
    PUSH_WITH_HANDLES(handles, q, data);

    test_data shuffled (data);
    std::random_shuffle(shuffled.begin(), shuffled.end());

    for (int i = 0; i != test_size; ++i)
        q.update(handles[i], shuffled[i]);

    check_q(q, data);
}


template <typename pri_queue>
void pri_queue_test_update_increase(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        data[i] = data.back()+1;
        *handles[i] = data[i];
        q.update(handles[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_update_increase_function(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        data[i] = data.back()+1;
        q.update(handles[i], data[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_decrease(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        *handles[i] = -1;
        data[i] = -1;
        q.decrease(handles[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_decrease_function(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        data[i] = -1;
        q.decrease(handles[i], -1);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_decrease_function_identity(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        q.decrease(handles[i], data[i]);

        check_q(q, data);
    }
}


template <typename pri_queue>
void pri_queue_test_increase(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        data[i] = data.back()+1;
        *handles[i] = data[i];
        q.increase(handles[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_increase_function(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        data[i] = data.back()+1;
        q.increase(handles[i], data[i]);

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_increase_function_identity(void)
{
    for (int i = 0; i != test_size; ++i) {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        q.increase(handles[i], data[i]);

        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_erase(void)
{
#ifdef USE_BOOST_RANDOM
    boost::mt19937 rng;
#endif

    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(test_size);
        PUSH_WITH_HANDLES(handles, q, data);

        for (int j = 0; j != i; ++j)
        {
#ifdef USE_BOOST_RANDOM
            boost::uniform_int<> range(0, data.size() - 1);
            boost::variate_generator<boost::mt19937&, boost::uniform_int<> > gen(rng, range);

            int index = gen();
#else
            int index = std::rand() % (data.size() - 1);
#endif
            q.erase(handles[index]);
            handles.erase(handles.begin() + index);
            data.erase(data.begin() + index);
        }

        std::sort(data.begin(), data.end());
        check_q(q, data);
    }
}


template <typename pri_queue>
void run_mutable_heap_update_tests(void)
{
    pri_queue_test_update_increase<pri_queue>();
    pri_queue_test_update_decrease<pri_queue>();

    pri_queue_test_update_shuffled<pri_queue>();
}

template <typename pri_queue>
void run_mutable_heap_update_function_tests(void)
{
    pri_queue_test_update_increase_function<pri_queue>();
    pri_queue_test_update_decrease_function<pri_queue>();
    pri_queue_test_update_function_identity<pri_queue>();
}


template <typename pri_queue>
void run_mutable_heap_decrease_tests(void)
{
    pri_queue_test_decrease<pri_queue>();
    pri_queue_test_decrease_function<pri_queue>();
    pri_queue_test_decrease_function_identity<pri_queue>();
}

template <typename pri_queue>
void run_mutable_heap_increase_tests(void)
{
    pri_queue_test_increase<pri_queue>();
    pri_queue_test_increase_function<pri_queue>();
    pri_queue_test_increase_function_identity<pri_queue>();
}

template <typename pri_queue>
void run_mutable_heap_erase_tests(void)
{
    pri_queue_test_erase<pri_queue>();
}

template <typename pri_queue>
void run_mutable_heap_test_handle_from_iterator(void)
{
    pri_queue que;

    que.push(3);
    que.push(1);
    que.push(4);

    que.update(pri_queue::s_handle_from_iterator(que.begin()),
               6);
}


template <typename pri_queue>
void run_mutable_heap_tests(void)
{
    run_mutable_heap_update_function_tests<pri_queue>();
    run_mutable_heap_update_tests<pri_queue>();
    run_mutable_heap_decrease_tests<pri_queue>();
    run_mutable_heap_increase_tests<pri_queue>();
    run_mutable_heap_erase_tests<pri_queue>();
    run_mutable_heap_test_handle_from_iterator<pri_queue>();
}

template <typename pri_queue>
void run_ordered_iterator_tests()
{
    pri_queue_test_ordered_iterators<pri_queue>();
}
