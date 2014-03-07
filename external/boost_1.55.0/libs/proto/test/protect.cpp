///////////////////////////////////////////////////////////////////////////////
// protect.hpp
//
//  Copyright 2012 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/core.hpp>
#include <boost/proto/transform/make.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/test/unit_test.hpp>
namespace proto=boost::proto;
using proto::_;

template<typename T>
struct identity
{
    typedef T type;
};

struct TestWithMake
  : proto::make< proto::protect<_> >
{};

struct TestWithMake1
  : proto::make< identity<proto::protect<_> > >
{};

struct TestWithMake2
  : proto::make< identity<proto::protect<int> > >
{};

struct TestWithMake3
  : proto::make< identity<proto::protect<identity<_> > > >
{};

struct TestWithMake4
  : proto::make< identity<proto::protect<identity<int> > > >
{};

struct TestWithMake5
  : proto::make< identity<proto::protect<identity<identity<int> > > > >
{};

void test_protect_with_make()
{
    proto::terminal<int>::type i = {42};

    _ t = TestWithMake()(i);
    _ t1 = TestWithMake1()(i);
    int t2 = TestWithMake2()(i);
    identity<_> t3 = TestWithMake3()(i);
    identity<int> t4 = TestWithMake4()(i);
    identity<identity<int> > t5 = TestWithMake5()(i);
}

//struct TestWithWhen
//  : proto::when<_, proto::protect<_>() >
//{};

struct TestWithWhen1
  : proto::when<_, identity<proto::protect<_> >() >
{};

struct TestWithWhen2
  : proto::when<_, identity<proto::protect<int> >() >
{};

struct TestWithWhen3
  : proto::when<_, identity<proto::protect<identity<_> > >() >
{};

struct TestWithWhen4
  : proto::when<_, identity<proto::protect<identity<int> > >() >
{};

struct TestWithWhen5
  : proto::when<_, identity<proto::protect<identity<identity<int> > > >() >
{};

void test_protect_with_when()
{
    proto::terminal<int>::type i = {42};

    //_ t = TestWithWhen()(i);
    _ t1 = TestWithWhen1()(i);
    int t2 = TestWithWhen2()(i);
    identity<_> t3 = TestWithWhen3()(i);
    identity<int> t4 = TestWithWhen4()(i);
    identity<identity<int> > t5 = TestWithWhen5()(i);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto::protect");

    test->add(BOOST_TEST_CASE(&test_protect_with_make));
    test->add(BOOST_TEST_CASE(&test_protect_with_when));

    return test;
}
