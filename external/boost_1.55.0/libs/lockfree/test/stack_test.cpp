//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


#include <boost/thread.hpp>
#include <boost/lockfree/stack.hpp>

#define BOOST_TEST_MAIN
#ifdef BOOST_LOCKFREE_INCLUDE_TESTS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#include "test_helpers.hpp"

BOOST_AUTO_TEST_CASE( simple_stack_test )
{
    boost::lockfree::stack<long> stk(128);

    stk.push(1);
    stk.push(2);
    long out;
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.pop(out));
}

BOOST_AUTO_TEST_CASE( unsafe_stack_test )
{
    boost::lockfree::stack<long> stk(128);

    stk.unsynchronized_push(1);
    stk.unsynchronized_push(2);
    long out;
    BOOST_REQUIRE(stk.unsynchronized_pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.unsynchronized_pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.unsynchronized_pop(out));
}

BOOST_AUTO_TEST_CASE( ranged_push_test )
{
    boost::lockfree::stack<long> stk(128);

    long data[2] = {1, 2};

    BOOST_REQUIRE_EQUAL(stk.push(data, data + 2), data + 2);

    long out;
    BOOST_REQUIRE(stk.unsynchronized_pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.unsynchronized_pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.unsynchronized_pop(out));
}

BOOST_AUTO_TEST_CASE( ranged_unsynchronized_push_test )
{
    boost::lockfree::stack<long> stk(128);

    long data[2] = {1, 2};

    BOOST_REQUIRE_EQUAL(stk.unsynchronized_push(data, data + 2), data + 2);

    long out;
    BOOST_REQUIRE(stk.unsynchronized_pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.unsynchronized_pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.unsynchronized_pop(out));
}

BOOST_AUTO_TEST_CASE( fixed_size_stack_test )
{
    boost::lockfree::stack<long, boost::lockfree::capacity<128> > stk;

    stk.push(1);
    stk.push(2);
    long out;
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.pop(out));
    BOOST_REQUIRE(stk.empty());
}

BOOST_AUTO_TEST_CASE( fixed_size_stack_test_exhausted )
{
    boost::lockfree::stack<long, boost::lockfree::capacity<2> > stk;

    stk.push(1);
    stk.push(2);
    BOOST_REQUIRE(!stk.push(3));
    long out;
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.pop(out));
    BOOST_REQUIRE(stk.empty());
}

BOOST_AUTO_TEST_CASE( bounded_stack_test_exhausted )
{
    boost::lockfree::stack<long> stk(2);

    stk.bounded_push(1);
    stk.bounded_push(2);
    BOOST_REQUIRE(!stk.bounded_push(3));
    long out;
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 2);
    BOOST_REQUIRE(stk.pop(out)); BOOST_REQUIRE_EQUAL(out, 1);
    BOOST_REQUIRE(!stk.pop(out));
    BOOST_REQUIRE(stk.empty());
}

BOOST_AUTO_TEST_CASE( stack_consume_one_test )
{
    boost::lockfree::stack<int> f(64);

    BOOST_WARN(f.is_lock_free());
    BOOST_REQUIRE(f.empty());

    f.push(1);
    f.push(2);

#ifdef BOOST_NO_CXX11_LAMBDAS
    bool success1 = f.consume_one(test_equal(2));
    bool success2 = f.consume_one(test_equal(1));
#else
    bool success1 = f.consume_one([] (int i) {
        BOOST_REQUIRE_EQUAL(i, 2);
    });

    bool success2 = f.consume_one([] (int i) {
        BOOST_REQUIRE_EQUAL(i, 1);
    });
#endif

    BOOST_REQUIRE(success1);
    BOOST_REQUIRE(success2);

    BOOST_REQUIRE(f.empty());
}

BOOST_AUTO_TEST_CASE( stack_consume_all_test )
{
    boost::lockfree::stack<int> f(64);

    BOOST_WARN(f.is_lock_free());
    BOOST_REQUIRE(f.empty());

    f.push(1);
    f.push(2);

#ifdef BOOST_NO_CXX11_LAMBDAS
    size_t consumed = f.consume_all(dummy_functor());
#else
    size_t consumed = f.consume_all([] (int i) {
    });
#endif

    BOOST_REQUIRE_EQUAL(consumed, 2u);

    BOOST_REQUIRE(f.empty());
}


BOOST_AUTO_TEST_CASE( reserve_test )
{
    typedef boost::lockfree::stack< void* > memory_stack;

    memory_stack ms(1);
    ms.reserve(1);
    ms.reserve_unsafe(1);
}
