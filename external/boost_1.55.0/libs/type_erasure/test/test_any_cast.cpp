// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_any_cast.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

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
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_value_to_value)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    any<test_concept> x(2);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    BOOST_CHECK_THROW(any_cast<double>(x), bad_any_cast);
    const any<test_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 2);
    BOOST_CHECK_THROW(any_cast<double>(y), bad_any_cast);
}

BOOST_AUTO_TEST_CASE(test_value_to_ref)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    any<test_concept> x(2);
    BOOST_CHECK_EQUAL(any_cast<int&>(x), 2);
    BOOST_CHECK_EQUAL(any_cast<const int&>(x), 2);
    BOOST_CHECK_THROW(any_cast<double&>(x), bad_any_cast);
    BOOST_CHECK_THROW(any_cast<const double&>(x), bad_any_cast);
    const any<test_concept> y(x);
    // BOOST_CHECK_EQUAL(any_cast<int&>(y), 2);
    BOOST_CHECK_EQUAL(any_cast<const int&>(y), 2);
    // BOOST_CHECK_THROW(any_cast<double&>(y), bad_any_cast);
    BOOST_CHECK_THROW(any_cast<const double&>(y), bad_any_cast);
}

BOOST_AUTO_TEST_CASE(test_value_to_pointer)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    any<test_concept> x(2);
    BOOST_CHECK_EQUAL(*any_cast<int*>(&x), 2);
    BOOST_CHECK_EQUAL(*any_cast<const int*>(&x), 2);
    BOOST_CHECK_EQUAL(any_cast<void*>(&x), any_cast<int*>(&x));
    BOOST_CHECK_EQUAL(any_cast<const void*>(&x), any_cast<const int*>(&x));
    BOOST_CHECK_EQUAL(any_cast<double*>(&x), (double*)0);
    BOOST_CHECK_EQUAL(any_cast<const double*>(&x), (double*)0);
    const any<test_concept> y(x);
    // BOOST_CHECK_EQUAL(*any_cast<int*>(&y), 2);
    BOOST_CHECK_EQUAL(*any_cast<const int*>(&y), 2);
    // BOOST_CHECK_EQUAL(any_cast<void*>(&y), any_cast<int*>(&y));
    BOOST_CHECK_EQUAL(any_cast<const void*>(&y), any_cast<const int*>(&y));
    // BOOST_CHECK_EQUAL(any_cast<double*>(&y), (double*)0);
    BOOST_CHECK_EQUAL(any_cast<const double*>(&y), (double*)0);
}

BOOST_AUTO_TEST_CASE(test_ref_to_value)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 2;
    any<test_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    BOOST_CHECK_THROW(any_cast<double>(x), bad_any_cast);
    const any<test_concept, _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 2);
    BOOST_CHECK_THROW(any_cast<double>(y), bad_any_cast);
}

BOOST_AUTO_TEST_CASE(test_ref_to_ref)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 2;
    any<test_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int&>(x), 2);
    BOOST_CHECK_EQUAL(any_cast<const int&>(x), 2);
    BOOST_CHECK_THROW(any_cast<double&>(x), bad_any_cast);
    BOOST_CHECK_THROW(any_cast<const double&>(x), bad_any_cast);
    const any<test_concept, _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<int&>(y), 2);
    BOOST_CHECK_EQUAL(any_cast<const int&>(y), 2);
    BOOST_CHECK_THROW(any_cast<double&>(y), bad_any_cast);
    BOOST_CHECK_THROW(any_cast<const double&>(y), bad_any_cast);
}

BOOST_AUTO_TEST_CASE(test_ref_to_pointer)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 2;
    any<test_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(*any_cast<int*>(&x), 2);
    BOOST_CHECK_EQUAL(*any_cast<const int*>(&x), 2);
    BOOST_CHECK_EQUAL(any_cast<void*>(&x), any_cast<int*>(&x));
    BOOST_CHECK_EQUAL(any_cast<const void*>(&x), any_cast<const int*>(&x));
    BOOST_CHECK_EQUAL(any_cast<double*>(&x), (double*)0);
    BOOST_CHECK_EQUAL(any_cast<const double*>(&x), (double*)0);
    const any<test_concept, _self&> y(x);
    BOOST_CHECK_EQUAL(*any_cast<int*>(&y), 2);
    BOOST_CHECK_EQUAL(*any_cast<const int*>(&y), 2);
    BOOST_CHECK_EQUAL(any_cast<void*>(&y), any_cast<int*>(&y));
    BOOST_CHECK_EQUAL(any_cast<const void*>(&y), any_cast<const int*>(&y));
    BOOST_CHECK_EQUAL(any_cast<double*>(&y), (double*)0);
    BOOST_CHECK_EQUAL(any_cast<const double*>(&y), (double*)0);
}

BOOST_AUTO_TEST_CASE(test_cref_to_value)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 2;
    any<test_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 2);
    BOOST_CHECK_THROW(any_cast<double>(x), bad_any_cast);
    const any<test_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 2);
    BOOST_CHECK_THROW(any_cast<double>(y), bad_any_cast);
}

BOOST_AUTO_TEST_CASE(test_cref_to_ref)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 2;
    any<test_concept, const _self&> x(i);
    // BOOST_CHECK_EQUAL(any_cast<int&>(x), 2);
    BOOST_CHECK_EQUAL(any_cast<const int&>(x), 2);
    // BOOST_CHECK_THROW(any_cast<double&>(x), bad_any_cast);
    BOOST_CHECK_THROW(any_cast<const double&>(x), bad_any_cast);
    const any<test_concept, const _self&> y(x);
    // BOOST_CHECK_EQUAL(any_cast<int&>(y), 2);
    BOOST_CHECK_EQUAL(any_cast<const int&>(y), 2);
    // BOOST_CHECK_THROW(any_cast<double&>(y), bad_any_cast);
    BOOST_CHECK_THROW(any_cast<const double&>(y), bad_any_cast);
}

BOOST_AUTO_TEST_CASE(test_cref_to_pointer)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 2;
    any<test_concept, const _self&> x(i);
    // BOOST_CHECK_EQUAL(*any_cast<int*>(&x), 2);
    BOOST_CHECK_EQUAL(*any_cast<const int*>(&x), 2);
    // BOOST_CHECK_EQUAL(any_cast<void*>(&x), any_cast<int*>(&x));
    BOOST_CHECK_EQUAL(any_cast<const void*>(&x), any_cast<const int*>(&x));
    // BOOST_CHECK_EQUAL(any_cast<double*>(&x), (double*)0);
    BOOST_CHECK_EQUAL(any_cast<const double*>(&x), (double*)0);
    const any<test_concept, const _self&> y(x);
    // BOOST_CHECK_EQUAL(*any_cast<int*>(&y), 2);
    BOOST_CHECK_EQUAL(*any_cast<const int*>(&y), 2);
    // BOOST_CHECK_EQUAL(any_cast<void*>(&y), any_cast<int*>(&y));
    BOOST_CHECK_EQUAL(any_cast<const void*>(&y), any_cast<const int*>(&y));
    // BOOST_CHECK_EQUAL(any_cast<double*>(&y), (double*)0);
    BOOST_CHECK_EQUAL(any_cast<const double*>(&y), (double*)0);
}
