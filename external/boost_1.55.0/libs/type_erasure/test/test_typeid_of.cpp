// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_typeid_of.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/typeid_of.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_val)
{
    typedef common<> test_concept;
    any<test_concept> x(2);
    BOOST_CHECK(typeid_of(x) == typeid(int));
    const any<test_concept> y(2);
    BOOST_CHECK(typeid_of(y) == typeid(int));
}

BOOST_AUTO_TEST_CASE(test_ref)
{
    typedef common<> test_concept;
    int i;
    any<test_concept, _self&> x(i);
    BOOST_CHECK(typeid_of(x) == typeid(int));
    const any<test_concept, _self&> y(i);
    BOOST_CHECK(typeid_of(y) == typeid(int));
}

BOOST_AUTO_TEST_CASE(test_cref)
{
    typedef common<> test_concept;
    int i;
    any<test_concept, const _self&> x(i);
    BOOST_CHECK(typeid_of(x) == typeid(int));
    const any<test_concept, const _self&> y(i);
    BOOST_CHECK(typeid_of(y) == typeid(int));
}

BOOST_AUTO_TEST_CASE(test_binding)
{
    typedef boost::mpl::vector<common<_a>, common<_b> > test_concept;
    binding<test_concept> b =
        make_binding<
            boost::mpl::map<
                boost::mpl::pair<_a, int>,
                boost::mpl::pair<_b, double>
            >
        >();
    BOOST_CHECK(typeid_of<_a>(b) == typeid(int));
    BOOST_CHECK(typeid_of<_b>(b) == typeid(double));
}
