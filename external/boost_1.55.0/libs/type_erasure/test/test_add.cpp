// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_add.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    destructible<T>,
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_same)
{
    typedef ::boost::mpl::vector<common<>, addable<> > test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2);
    any<test_concept> z(x + y);
    int i = any_cast<int>(z);
    BOOST_CHECK_EQUAL(i, 3);
}

BOOST_AUTO_TEST_CASE(test_int1)
{
    typedef ::boost::mpl::vector<common<>, addable<_self, int> > test_concept;
    any<test_concept> x(1);
    any<test_concept> z(x + 2);
    int i = any_cast<int>(z);
    BOOST_CHECK_EQUAL(i, 3);
}

BOOST_AUTO_TEST_CASE(test_int2)
{
    typedef ::boost::mpl::vector<common<>, addable<int, _self, _self> > test_concept;
    any<test_concept> x(1);
    any<test_concept> z(2 + x);
    int i = any_cast<int>(z);
    BOOST_CHECK_EQUAL(i, 3);
}

BOOST_AUTO_TEST_CASE(test_mixed)
{
    typedef ::boost::mpl::vector<common<_a>, common<_b>, addable<_a, _b> > test_concept;
    tuple<test_concept, _a, _b> x(1.0, 2);
    any<test_concept, _a> z(get<0>(x) + get<1>(x));
    double d = any_cast<double>(z);
    BOOST_CHECK_EQUAL(d, 3);
}

BOOST_AUTO_TEST_CASE(test_overload)
{
    typedef ::boost::mpl::vector<
        common<_a>,
        common<_b>,
        addable<_a>,
        addable<_a, int>,
        addable<int, _a, _a>,
        addable<_b>,
        addable<_b, int>,
        addable<int, _b, _b>,
        addable<_a, _b>
    > test_concept;
    tuple<test_concept, _a, _b> t(1.0, 2);
    any<test_concept, _a> x(get<0>(t));
    any<test_concept, _b> y(get<1>(t));

    {
        any<test_concept, _a> z(x + x);
        BOOST_CHECK_EQUAL(any_cast<double>(z), 2.0);
    }

    {
        any<test_concept, _a> z(x + 3);
        BOOST_CHECK_EQUAL(any_cast<double>(z), 4.0);
    }

    {
        any<test_concept, _a> z(3 + x);
        BOOST_CHECK_EQUAL(any_cast<double>(z), 4.0);
    }
    
    {
        any<test_concept, _b> z(y + y);
        BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    }

    {
        any<test_concept, _b> z(y + 3);
        BOOST_CHECK_EQUAL(any_cast<int>(z), 5);
    }

    {
        any<test_concept, _b> z(3 + y);
        BOOST_CHECK_EQUAL(any_cast<int>(z), 5);
    }

    {
        any<test_concept, _a> z(x + y);
        BOOST_CHECK_EQUAL(any_cast<double>(z), 3);
    }
}
