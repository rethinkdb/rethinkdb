// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_nested.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

typedef any<boost::mpl::vector<common<> > > any1_type;

struct test_class
{
    int i;
};

test_class operator+(const test_class& lhs, const any1_type& rhs)
{
    test_class result = { lhs.i + any_cast<int>(rhs) };
    return result;
}

BOOST_AUTO_TEST_CASE(test_basic)
{
    typedef boost::mpl::vector<common<>, addable<_self, any1_type> > test_concept;
    any1_type a1(1);
    test_class v = { 2 };
    any<test_concept> x(v);
    any<test_concept> y(x + a1);
    BOOST_CHECK_EQUAL(any_cast<test_class>(y).i, 3);
}

BOOST_AUTO_TEST_CASE(test_relaxed)
{
    typedef boost::mpl::vector<common<_a>, addable<_a, any1_type>, relaxed> test_concept;
    typedef boost::mpl::vector<common<_b>, addable<_b, any1_type>, relaxed> dest_concept;
    any1_type a1(1);
    test_class v = { 2 };
    any<test_concept, _a> x(v);
    any<test_concept, _a> y(x + a1);
    BOOST_CHECK_EQUAL(any_cast<test_class>(y).i, 3);

    any<dest_concept, _b> z(x);
}
