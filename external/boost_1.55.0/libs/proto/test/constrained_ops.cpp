///////////////////////////////////////////////////////////////////////////////
// constrained_ops.cpp
//
//  Copyright 2010 Thomas Heller
//  Copyright 2011 Eric Niebler
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/proto.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost;

typedef proto::terminal<int>::type term;

struct equation;

struct addition:
    proto::or_
    <
       proto::terminal<proto::_>,
       proto::plus<addition, addition>
    >
{};

struct equation:
    proto::or_
    <
        proto::equal_to<addition, addition>
    >
{};

template<class Expr>
struct extension;

struct my_domain:
    proto::domain
    <
         proto::pod_generator<extension>,
         equation,
         proto::default_domain
    >
{};

template<class Expr>
struct lhs_extension;

struct my_lhs_domain:
    proto::domain
    <
        proto::pod_generator<lhs_extension>,
        addition,
        my_domain
    >
{};

template<class Expr>
struct rhs_extension;

struct my_rhs_domain:
    proto::domain
    <
        proto::pod_generator<rhs_extension>,
        addition,
        my_domain
    >
{};

template<class Expr>
struct extension
{
     BOOST_PROTO_BASIC_EXTENDS(
         Expr
       , extension<Expr>
       , my_domain
     )

    void test() const
    {}
};

template<class Expr>
struct lhs_extension
{
     BOOST_PROTO_BASIC_EXTENDS(
         Expr
       , lhs_extension<Expr>
       , my_lhs_domain
     )
};

template<class Expr>
struct rhs_extension
{
     BOOST_PROTO_BASIC_EXTENDS(
         Expr
       , rhs_extension<Expr>
       , my_rhs_domain
     )
};

void test_constrained_ops()
{
     lhs_extension<term> const i = {};
     rhs_extension<term> const j = {};

     proto::assert_matches_not<equation>(i);              // false
     proto::assert_matches_not<equation>(j);              // false
     proto::assert_matches_not<equation>(i + i);          // false
     proto::assert_matches_not<equation>(j + j);          // false
#if 0
     proto::assert_matches_not<equation>(i + j);          // compile error (by design)
     proto::assert_matches_not<equation>(j + i);          // compile error (by design)
#endif
     proto::assert_matches<equation>(i == j);             // true
     proto::assert_matches<equation>(i == j + j);         // true
     proto::assert_matches<equation>(i + i == j);         // true
     proto::assert_matches<equation>(i + i == j + j);     // true
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test constrained EDSLs");
    test->add(BOOST_TEST_CASE(&test_constrained_ops));
    return test;
}
