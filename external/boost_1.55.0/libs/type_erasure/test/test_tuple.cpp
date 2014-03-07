// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_tuple.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/front.hpp>
#include <boost/fusion/include/back.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/empty.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/end.hpp>
#include <boost/fusion/include/distance.hpp>
#include <boost/fusion/include/next.hpp>
#include <boost/fusion/include/prior.hpp>
#include <boost/fusion/include/equal_to.hpp>
#include <boost/fusion/include/advance.hpp>
#include <boost/fusion/include/deref.hpp>
#include <boost/fusion/include/value_of.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_same)
{
    tuple<common<_a>, _a, _a> t(1, 2);
    BOOST_CHECK_EQUAL(any_cast<int&>(get<0>(t)), 1);
    BOOST_CHECK_EQUAL(any_cast<int&>(get<1>(t)), 2);
}

BOOST_AUTO_TEST_CASE(test_degenerate)
{
    tuple<boost::mpl::vector<> > t;
}

template<class T>
typename T::value_type get_static(T) { return T::value; }

BOOST_AUTO_TEST_CASE(test_fusion)
{
    typedef boost::mpl::vector<common<_a>, common<_b>, addable<_a, _b> > test_concept;
    tuple<test_concept, _a, _b> t(2.0, 1);
    BOOST_CHECK_EQUAL(any_cast<double&>(boost::fusion::at_c<0>(t)), 2.0);
    BOOST_CHECK_EQUAL(any_cast<int&>(boost::fusion::at_c<1>(t)), 1);
    BOOST_CHECK_EQUAL(any_cast<double&>(boost::fusion::front(t)), 2.0);
    BOOST_CHECK_EQUAL(any_cast<int&>(boost::fusion::back(t)), 1);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::empty(t)), false);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::size(t)), 2);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::begin(t), boost::fusion::end(t))), 2);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::next(boost::fusion::begin(t)), boost::fusion::end(t))), 1);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::begin(t), boost::fusion::prior(boost::fusion::end(t)))), 1);

    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::advance_c<2>(boost::fusion::begin(t)), boost::fusion::end(t))), 0);
    BOOST_CHECK_EQUAL(any_cast<double&>(boost::fusion::deref(boost::fusion::begin(t))), 2.0);
}

BOOST_AUTO_TEST_CASE(test_fusion_const)
{
    typedef boost::mpl::vector<common<_a>, common<_b>, addable<_a, _b> > test_concept;
    const tuple<test_concept, _a, _b> t(2.0, 1);
    BOOST_CHECK_EQUAL(any_cast<const double&>(boost::fusion::at_c<0>(t)), 2.0);
    BOOST_CHECK_EQUAL(any_cast<const int&>(boost::fusion::at_c<1>(t)), 1);
    BOOST_CHECK_EQUAL(any_cast<const double&>(boost::fusion::front(t)), 2.0);
    BOOST_CHECK_EQUAL(any_cast<const int&>(boost::fusion::back(t)), 1);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::empty(t)), false);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::size(t)), 2);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::begin(t), boost::fusion::end(t))), 2);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::next(boost::fusion::begin(t)), boost::fusion::end(t))), 1);
    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::begin(t), boost::fusion::prior(boost::fusion::end(t)))), 1);

    BOOST_CHECK_EQUAL(get_static(boost::fusion::distance(boost::fusion::advance_c<2>(boost::fusion::begin(t)), boost::fusion::end(t))), 0);
    BOOST_CHECK_EQUAL(any_cast<const double&>(boost::fusion::deref(boost::fusion::begin(t))), 2.0);
}
