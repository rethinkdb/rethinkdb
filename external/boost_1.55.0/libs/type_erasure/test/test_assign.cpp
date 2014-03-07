// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_assign.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_basic)
{
    typedef ::boost::mpl::vector<common<>, assignable<> > test_concept;
    any<test_concept> x(1);
    int* ip = any_cast<int*>(&x);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    // make sure that we're actually using assignment
    // of the underlying object, not copy and swap.
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), ip);
}

BOOST_AUTO_TEST_CASE(test_basic_relaxed)
{
    typedef ::boost::mpl::vector<common<>, assignable<>, relaxed > test_concept;
    any<test_concept> x(1);
    int* ip = any_cast<int*>(&x);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    // make sure that we're actually using assignment
    // of the underlying object, not copy and swap.
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), ip);
}

BOOST_AUTO_TEST_CASE(test_relaxed_no_copy)
{
    typedef ::boost::mpl::vector<
        destructible<>,
        typeid_<>,
        assignable<>,
        relaxed
    > test_concept;
    any<test_concept> x(1);
    int* ip = any_cast<int*>(&x);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    // make sure that we're actually using assignment
    // of the underlying object, not copy and swap.
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), ip);
}

BOOST_AUTO_TEST_CASE(test_relaxed_no_assign)
{
    typedef ::boost::mpl::vector<
        common<>,
        relaxed
    > test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
}

BOOST_AUTO_TEST_CASE(test_dynamic_fallback)
{
    typedef ::boost::mpl::vector<common<>, assignable<>, relaxed> test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2.0);
    x = y;
    BOOST_CHECK_EQUAL(any_cast<double>(x), 2.0);
}

BOOST_AUTO_TEST_CASE(test_dynamic_fail)
{
    typedef ::boost::mpl::vector<destructible<>, typeid_<>, assignable<>, relaxed> test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2.0);
    BOOST_CHECK_THROW(x = y, bad_function_call);
}

BOOST_AUTO_TEST_CASE(test_basic_int)
{
    typedef ::boost::mpl::vector<common<>, assignable<_self, int> > test_concept;
    any<test_concept> x(1);
    int* ip = any_cast<int*>(&x);
    x = 2;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    // make sure that we're actually using assignment
    // of the underlying object, not copy and swap.
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), ip);
}

BOOST_AUTO_TEST_CASE(test_basic_relaxed_int)
{
    typedef ::boost::mpl::vector<common<>, assignable<_self, int>, relaxed > test_concept;
    any<test_concept> x(1);
    int* ip = any_cast<int*>(&x);
    x = 2;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    // make sure that we're actually using assignment
    // of the underlying object, not copy and swap.
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), ip);
}

BOOST_AUTO_TEST_CASE(test_relaxed_no_copy_int)
{
    typedef ::boost::mpl::vector<
        destructible<>,
        typeid_<>,
        assignable<_self, int>,
        relaxed
    > test_concept;
    any<test_concept> x(1);
    int* ip = any_cast<int*>(&x);
    x = 2;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    // make sure that we're actually using assignment
    // of the underlying object, not copy and swap.
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), ip);
}

BOOST_AUTO_TEST_CASE(test_relaxed_no_assign_int)
{
    typedef ::boost::mpl::vector<
        common<>,
        relaxed
    > test_concept;
    any<test_concept> x(1);
    x = 2;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
}

BOOST_AUTO_TEST_CASE(test_basic_ref)
{
    typedef ::boost::mpl::vector<common<>, assignable<> > test_concept;
    int i = 1;
    any<test_concept, _self&> x(i);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(i, 2);
}

BOOST_AUTO_TEST_CASE(test_basic_relaxed_ref)
{
    typedef ::boost::mpl::vector<common<>, assignable<>, relaxed > test_concept;
    int i = 1;
    any<test_concept, _self&> x(i);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(i, 2);
}

BOOST_AUTO_TEST_CASE(test_relaxed_no_assign_ref)
{
    typedef ::boost::mpl::vector<
        common<>,
        relaxed
    > test_concept;
    int i = 1;
    any<test_concept, _self&> x(i);
    any<test_concept> y(2);
    x = y;
    BOOST_CHECK_EQUAL(i, 1);
    BOOST_CHECK_EQUAL(any_cast<int*>(&x), any_cast<int*>(&y));
}

BOOST_AUTO_TEST_CASE(test_dynamic_fallback_ref)
{
    typedef ::boost::mpl::vector<common<>, assignable<>, relaxed> test_concept;
    int i = 1;
    any<test_concept, _self&> x(i);
    any<test_concept> y(2.0);
    x = y;
    BOOST_CHECK_EQUAL(any_cast<double>(x), 2.0);
}
