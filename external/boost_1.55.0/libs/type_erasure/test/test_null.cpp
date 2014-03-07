// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_null.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

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
    relaxed
> {};

BOOST_AUTO_TEST_CASE(test_typeid) {
    any<common<> > val;
    BOOST_CHECK(typeid_of(val) == typeid(void));
}

BOOST_AUTO_TEST_CASE(test_any_cast) {
    any<common<> > val;
    BOOST_CHECK_EQUAL(any_cast<void*>(&val), (void*)0);
    BOOST_CHECK_EQUAL(any_cast<int*>(&val), (int*)0);
}

BOOST_AUTO_TEST_CASE(test_copy) {
    any<common<> > val;
    any<common<> > val2(val);
    BOOST_CHECK(typeid_of(val2) == typeid(void));
}
