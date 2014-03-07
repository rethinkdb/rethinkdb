// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_add_assign.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

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
    typedef ::boost::mpl::vector<common<>, add_assignable<> > test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2);
    any<test_concept>& z(x += y);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 3);
    BOOST_CHECK_EQUAL(&x, &z);
}

BOOST_AUTO_TEST_CASE(test_int1)
{
    typedef ::boost::mpl::vector<common<>, add_assignable<_self, int> > test_concept;
    any<test_concept> x(1);
    any<test_concept>& z(x += 2);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 3);
    BOOST_CHECK_EQUAL(&x, &z);
}

BOOST_AUTO_TEST_CASE(test_int2)
{
    typedef ::boost::mpl::vector<common<>, add_assignable<int, _self> > test_concept;
    int x = 1;
    any<test_concept> y(2);
    int& z(x += y);
    BOOST_CHECK_EQUAL(x, 3);
    BOOST_CHECK_EQUAL(&x, &z);
}

BOOST_AUTO_TEST_CASE(test_mixed)
{
    typedef ::boost::mpl::vector<common<_a>, common<_b>, add_assignable<_a, _b> > test_concept;
    tuple<test_concept, _a, _b> t(1.0, 2);
    any<test_concept, _a> x(get<0>(t));
    any<test_concept, _b> y(get<1>(t));
    any<test_concept, _a>& z(x += y);
    BOOST_CHECK_EQUAL(any_cast<double>(x), 3.0);
    BOOST_CHECK_EQUAL(&x, &z);
}

BOOST_AUTO_TEST_CASE(test_overload)
{
    typedef ::boost::mpl::vector<
        common<_a>,
        common<_b>,
        add_assignable<_a>,
        add_assignable<_a, int>,
        add_assignable<double, _a>,
        add_assignable<_b>,
        add_assignable<_b, int>,
        add_assignable<double, _b>,
        add_assignable<_a, _b>
    > test_concept;
    tuple<test_concept, _a, _b> t(1.0, 2);

    {
        any<test_concept, _a> x(get<0>(t));
        any<test_concept, _a> y(get<0>(t));
        any<test_concept, _a>& z(x += y);
        BOOST_CHECK_EQUAL(any_cast<double>(x), 2.0);
        BOOST_CHECK_EQUAL(&x, &z);
    }

    {
        any<test_concept, _a> x(get<0>(t));
        int y = 5;
        any<test_concept, _a>& z(x += y);
        BOOST_CHECK_EQUAL(any_cast<double>(x), 6.0);
        BOOST_CHECK_EQUAL(&x, &z);
    }

    {
        double x = 11;
        any<test_concept, _a> y(get<0>(t));
        double& z(x += y);
        BOOST_CHECK_EQUAL(x, 12);
        BOOST_CHECK_EQUAL(&x, &z);
    }

    {
        any<test_concept, _b> x(get<1>(t));
        any<test_concept, _b> y(get<1>(t));
        any<test_concept, _b>& z(x += y);
        BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
        BOOST_CHECK_EQUAL(&x, &z);
    }

    {
        any<test_concept, _b> x(get<1>(t));
        int y = 5;
        any<test_concept, _b>& z(x += y);
        BOOST_CHECK_EQUAL(any_cast<int>(x), 7);
        BOOST_CHECK_EQUAL(&x, &z);
    }

    {
        double x = 11;
        any<test_concept, _b> y(get<1>(t));
        double& z(x += y);
        BOOST_CHECK_EQUAL(x, 13);
        BOOST_CHECK_EQUAL(&x, &z);
    }

    {
        any<test_concept, _a> x(get<0>(t));
        any<test_concept, _b> y(get<1>(t));
        any<test_concept, _a>& z(x += y);
        BOOST_CHECK_EQUAL(any_cast<double>(x), 3.0);
        BOOST_CHECK_EQUAL(&x, &z);
    }
}
