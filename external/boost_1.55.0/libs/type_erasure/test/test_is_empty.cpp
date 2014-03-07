// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_is_empty.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/is_empty.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

BOOST_AUTO_TEST_CASE(test_non_relaxed)
{
    typedef copy_constructible<> test_concept;
    any<test_concept> x(2);
    BOOST_CHECK(!is_empty(x));
}

BOOST_AUTO_TEST_CASE(test_relaxed)
{
    typedef boost::mpl::vector<copy_constructible<>, relaxed> test_concept;
    any<test_concept> x(2);
    BOOST_CHECK(!is_empty(x));
    any<test_concept> y;
    BOOST_CHECK(is_empty(y));
}
