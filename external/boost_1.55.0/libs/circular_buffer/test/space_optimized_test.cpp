// Test of the space optimized adaptor of the circular buffer.

// Copyright (c) 2003-2008 Jan Gaspar

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"

#define CB_CONTAINER circular_buffer_space_optimized

#include "common.ipp"

typedef circular_buffer_space_optimized<MyInteger> cb_space_optimized;
typedef cb_space_optimized::capacity_type capacity_ctrl;

// min_capacity test (it is useful to use a debug tool)
void min_capacity_test() {

    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);

    cb_space_optimized cb1(capacity_ctrl(10, 10));
    cb_space_optimized cb2(capacity_ctrl(10, 5), 1);
    cb_space_optimized cb3(capacity_ctrl(20, 10), v.begin(), v.end());

    BOOST_CHECK(cb1.size() == 0);
    BOOST_CHECK(cb1.capacity().capacity() == 10);
    BOOST_CHECK(cb1.capacity().min_capacity() == 10);
    BOOST_CHECK(cb2[0] == 1);
    BOOST_CHECK(cb2.size() == 10);
    BOOST_CHECK(cb2.capacity() == 10);
    BOOST_CHECK(cb2.capacity().min_capacity() == 5);
    BOOST_CHECK(cb3[0] == 1);
    BOOST_CHECK(cb3.size() == 5);
    BOOST_CHECK(cb3.capacity() == 20);
    BOOST_CHECK(cb3.capacity().min_capacity() == 10);
    BOOST_CHECK(cb1.capacity().min_capacity() <= cb1.internal_capacity());
    BOOST_CHECK(cb2.capacity().min_capacity() <= cb2.internal_capacity());
    BOOST_CHECK(cb3.capacity().min_capacity() <= cb3.internal_capacity());

    cb2.erase(cb2.begin() + 2, cb2.end());

    BOOST_CHECK(cb2.size() == 2);
    BOOST_CHECK(cb2.capacity().min_capacity() <= cb2.internal_capacity());

    cb2.clear();
    cb3.clear();

    BOOST_CHECK(cb2.empty());
    BOOST_CHECK(cb3.empty());
    BOOST_CHECK(cb2.capacity().min_capacity() <= cb2.internal_capacity());
    BOOST_CHECK(cb3.capacity().min_capacity() <= cb3.internal_capacity());
}

void capacity_control_test() {

    circular_buffer_space_optimized<int>::capacity_type c1 = 10;
    circular_buffer_space_optimized<int>::capacity_type c2 =
    circular_buffer_space_optimized<int>::capacity_type(20, 5);
    circular_buffer_space_optimized<int>::capacity_type c3 = c2;

    BOOST_CHECK(c1.capacity() == 10);
    BOOST_CHECK(c1.min_capacity() == 0);
    BOOST_CHECK(c2.capacity() == 20);
    BOOST_CHECK(c2.min_capacity() == 5);
    BOOST_CHECK(c3.capacity() == 20);
    BOOST_CHECK(c3.min_capacity() == 5);

    c1 = c2;

    BOOST_CHECK(c1.capacity() == 20);
    BOOST_CHECK(c1.min_capacity() == 5);
}

void specific_constructors_test() {

    cb_space_optimized cb1;
    BOOST_CHECK(cb1.capacity() == 0);
    BOOST_CHECK(cb1.capacity().min_capacity() == 0);
    BOOST_CHECK(cb1.internal_capacity() == 0);
    BOOST_CHECK(cb1.size() == 0);

    cb1.push_back(1);
    cb1.push_back(2);
    cb1.push_back(3);

    BOOST_CHECK(cb1.size() == 0);
    BOOST_CHECK(cb1.capacity() == 0);

    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    cb_space_optimized cb2(v.begin(), v.end());

    BOOST_CHECK(cb2.capacity() == 3);
    BOOST_CHECK(cb2.capacity().min_capacity() == 0);
    BOOST_CHECK(cb2.size() == 3);
}

void shrink_to_fit_test() {

    cb_space_optimized cb(1000);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    BOOST_CHECK(cb.size() == 3);
    BOOST_CHECK(cb.capacity() == 1000);

    size_t internal_capacity = cb.internal_capacity();
    cb_space_optimized(cb).swap(cb);

    BOOST_CHECK(cb.size() == 3);
    BOOST_CHECK(cb.capacity() == 1000);
    BOOST_CHECK(internal_capacity >= cb.internal_capacity());
}

void iterator_invalidation_test() {

#if !defined(NDEBUG) && !defined(BOOST_CB_DISABLE_DEBUG)

    cb_space_optimized cb1(10, 1);
    cb1.push_back(2);
    cb1.push_back(3);
    cb1.push_back(4);
    cb_space_optimized::iterator it1 = cb1.end();
    cb_space_optimized::const_iterator it2 = cb1.begin();
    cb_space_optimized::iterator it3 = cb1.begin() + 6;

    cb1.set_capacity(10);
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));
    BOOST_CHECK(!it3.is_valid(&cb1));

    it1 = cb1.end();
    it2 = cb1.begin();
    it3 = cb1.begin() + 6;
    cb1.rset_capacity(10);
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));
    BOOST_CHECK(!it3.is_valid(&cb1));

    it1 = cb1.end();
    it2 = cb1.begin();
    it3 = cb1.begin() + 6;
    cb1.resize(10);
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));
    BOOST_CHECK(!it3.is_valid(&cb1));

    it1 = cb1.end();
    it2 = cb1.begin();
    it3 = cb1.begin() + 6;
    cb1.rresize(10);
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));
    BOOST_CHECK(!it3.is_valid(&cb1));

    {
        cb_space_optimized cb2(10, 1);
        cb2.push_back(2);
        cb2.push_back(3);
        cb2.push_back(4);
        it1 = cb2.end();
        it2 = cb2.begin();
        it3 = cb2.begin() + 6;
    }
    BOOST_CHECK(!it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));
    BOOST_CHECK(!it3.is_valid(&cb1));

#endif // #if !defined(NDEBUG) && !defined(BOOST_CB_DISABLE_DEBUG)
}

// test main
test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {

    test_suite* tests = BOOST_TEST_SUITE("Unit tests for the circular_buffer_space_optimized.");
    add_common_tests(tests);

    tests->add(BOOST_TEST_CASE(&min_capacity_test));
    tests->add(BOOST_TEST_CASE(&capacity_control_test));
    tests->add(BOOST_TEST_CASE(&specific_constructors_test));
    tests->add(BOOST_TEST_CASE(&shrink_to_fit_test));
    tests->add(BOOST_TEST_CASE(&iterator_invalidation_test));

    return tests;
}
