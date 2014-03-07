// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_reference.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

class no_destroy
{
protected:
    ~no_destroy() {}
};

class with_destroy : public no_destroy
{
public:
    ~with_destroy() {}
};

template<class T = _self>
struct common : ::boost::mpl::vector<
    destructible<T>,
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_basic)
{
    typedef ::boost::mpl::vector<typeid_<> > test_concept;
    with_destroy val;
    any<test_concept, _self&> x(static_cast<no_destroy&>(val));
    no_destroy& ref = any_cast<no_destroy&>(x);
    BOOST_CHECK_EQUAL(&ref, &val);
}

BOOST_AUTO_TEST_CASE(test_increment)
{
    typedef ::boost::mpl::vector<incrementable<> > test_concept;
    int i = 0;
    any<test_concept, _self&> x(i);
    ++x;
    BOOST_CHECK_EQUAL(i, 1);
}

BOOST_AUTO_TEST_CASE(test_add)
{
    typedef ::boost::mpl::vector<common<>, addable<> > test_concept;
    int i = 1;
    int j = 2;
    any<test_concept, _self&> x(i);
    any<test_concept, _self&> y(j);
    any<test_concept, _self> z(x + y);
    int k = any_cast<int>(z);
    BOOST_CHECK_EQUAL(k, 3);
}

BOOST_AUTO_TEST_CASE(test_mixed_add)
{
    typedef ::boost::mpl::vector<common<>, addable<> > test_concept;
    int i = 1;
    int j = 2;
    any<test_concept, _self&> x(i);
    any<test_concept, _self> y(j);
    any<test_concept, _self> z(x + y);
    int k = any_cast<int>(z);
    BOOST_CHECK_EQUAL(k, 3);
}
