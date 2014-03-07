///////////////////////////////////////////////////////////////////////////////
// new_switch.cpp
//
//  Copyright 2011 Eric Niebler
//  Copyright Pierre Esterie & Joel Falcou.
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/proto/debug.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/long.hpp>
#include <boost/mpl/bool.hpp>

namespace proto = boost::proto;

struct MyCases
{
    template<typename Tag>
    struct case_
      : proto::not_<proto::_>
    {};
};

template<>
struct MyCases::case_<proto::tag::shift_right>
  : proto::_
{};

template<>
struct MyCases::case_<proto::tag::plus>
  : proto::_
{};

struct ArityOf;

struct ArityOfCases
{
    template<typename ArityOf>
    struct case_
      : proto::not_<proto::_>
    {};
};


template<>
struct ArityOfCases::case_<boost::mpl::long_<1> >
  : boost::proto::when<boost::proto::_, boost::mpl::false_()>
{};

template<>
struct ArityOfCases::case_<boost::mpl::long_<2> >
  : boost::proto::when<boost::proto::_, boost::mpl::true_()>
{};

struct ArityOf
  : boost::proto::switch_<
        ArityOfCases
      , proto::arity_of<proto::_>()
    >
{};

void test_switch()
{
    // Tests for backward compatibility
    proto::assert_matches<proto::switch_<MyCases> >(proto::lit(1) >> 'a');
    proto::assert_matches<proto::switch_<MyCases> >(proto::lit(1) + 'a');
    proto::assert_matches_not<proto::switch_<MyCases> >(proto::lit(1) << 'a');

    //Test new matching on the Transform result type
    ArityOf ar;

    proto::assert_matches_not<ArityOf>(proto::lit(1));
    proto::assert_matches<ArityOf>(proto::lit(1) + 2);
    proto::assert_matches<ArityOf>(!proto::lit(1));
    BOOST_CHECK_EQUAL(ar(!proto::lit(1)), false);
    BOOST_CHECK_EQUAL(ar(proto::lit(1) + 2), true);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite(int argc, char* argv[])
{
    test_suite *test = BOOST_TEST_SUITE("test proto::switch_<>");

    test->add(BOOST_TEST_CASE(&test_switch));

    return test;
}

