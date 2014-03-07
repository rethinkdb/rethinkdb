// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_increment.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

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

BOOST_AUTO_TEST_CASE(test_value)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > test_concept;
    any<test_concept> x(1);
    ++x;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    any<test_concept> y(x++);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 3);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 2);
}

BOOST_AUTO_TEST_CASE(test_reference)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > test_concept;
    int i = 1;
    any<test_concept, _self&> x(i);
    ++x;
    BOOST_CHECK_EQUAL(i, 2);
    any<test_concept> y(x++);
    BOOST_CHECK_EQUAL(i, 3);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 2);
}
