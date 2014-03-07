///////////////////////////////////////////////////////////////////////////////
// make.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/core.hpp>
#include <boost/proto/transform/arg.hpp>
#include <boost/proto/transform/make.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/test/unit_test.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;

template<typename T>
struct type2type {};

template<typename T>
struct wrapper
{
    T t_;
    explicit wrapper(T const & t = T()) : t_(t) {}
};

template<typename T>
struct careful
{
    typedef typename T::not_there not_there;
};

// Test that when no substitution is done, we don't instantiate templates
struct MakeTest1
  : proto::make< type2type< careful<int> > >
{};

void make_test1()
{
    proto::terminal<int>::type i = {42};
    type2type< careful<int> > res = MakeTest1()(i);
}

// Test that when substitution is done, and there is no nested ::type
// typedef, the result is the wrapper
struct MakeTest2
  : proto::make< wrapper< proto::_value > >
{};

void make_test2()
{
    proto::terminal<int>::type i = {42};
    wrapper<int> res = MakeTest2()(i);
    BOOST_CHECK_EQUAL(res.t_, 0);
}

// Test that when substitution is done, and there is no nested ::type
// typedef, the result is the wrapper
struct MakeTest3
  : proto::make< wrapper< proto::_value >(proto::_value) >
{};

void make_test3()
{
    proto::terminal<int>::type i = {42};
    wrapper<int> res = MakeTest3()(i);
    BOOST_CHECK_EQUAL(res.t_, 42);
}

// Test that when substitution is done, and there is no nested ::type
// typedef, the result is the wrapper
struct MakeTest4
  : proto::make< mpl::identity< proto::_value >(proto::_value) >
{};

void make_test4()
{
    proto::terminal<int>::type i = {42};
    int res = MakeTest4()(i);
    BOOST_CHECK_EQUAL(res, 42);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test the make transform");

    test->add(BOOST_TEST_CASE(&make_test1));
    test->add(BOOST_TEST_CASE(&make_test2));
    test->add(BOOST_TEST_CASE(&make_test3));
    test->add(BOOST_TEST_CASE(&make_test4));

    return test;
}
