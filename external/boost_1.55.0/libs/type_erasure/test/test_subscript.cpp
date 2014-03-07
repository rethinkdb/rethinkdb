// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_subscript.cpp 79858 2012-08-03 18:18:42Z steven_watanabe $

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

BOOST_AUTO_TEST_CASE(test_basic)
{
    typedef ::boost::mpl::vector<common<>, subscriptable<int&> > test_concept;
    int i[5];
    any<test_concept> x(&i[0]);
    BOOST_CHECK_EQUAL(&x[0], &i[0]);
}

BOOST_AUTO_TEST_CASE(test_basic_const)
{
    typedef ::boost::mpl::vector<common<>, subscriptable<int&, const _self> > test_concept;
    int i[5];
    const any<test_concept> x(&i[0]);
    BOOST_CHECK_EQUAL(&x[0], &i[0]);
}

BOOST_AUTO_TEST_CASE(test_any_result)
{
    typedef ::boost::mpl::vector<common<>, common<_a>, subscriptable<_a&, const _self> > test_concept;
    typedef ::boost::mpl::map<
        ::boost::mpl::pair<_self, int*>,
        ::boost::mpl::pair<_a, int>
    > types;
    int i[5];
    any<test_concept> x(&i[0], make_binding<types>());
    any<test_concept, _a&> y(x[0]);
    BOOST_CHECK_EQUAL(any_cast<int*>(&y), &i[0]);
}

BOOST_AUTO_TEST_CASE(test_any_result_const)
{
    typedef ::boost::mpl::vector<common<>, common<_a>, subscriptable<const _a&, const _self> > test_concept;
    typedef ::boost::mpl::map<
        ::boost::mpl::pair<_self, const int*>,
        ::boost::mpl::pair<_a, int>
    > types;
    const int i[5] = { 0, 0, 0, 0, 0 };
    any<test_concept> x(&i[0], make_binding<types>());
    any<test_concept, const _a&> y(x[0]);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i[0]);
}
