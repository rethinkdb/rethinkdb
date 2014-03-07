//  Copyright 2011 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This is an example of how to specify a transform externally so
// that a single grammar can be used to drive multiple differnt
// calculations. In particular, it defines a calculator grammar
// that computes the result of an expression with either checked
// or non-checked division.

#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/proto/proto.hpp>
#include <boost/test/unit_test.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;
using proto::_;

// The argument placeholder type
template<typename I> struct placeholder : I {};

// Give each rule in the grammar a "name". This is so that we
// can easily dispatch on it later.
struct calc_grammar;
struct divides_rule : proto::divides<calc_grammar, calc_grammar> {};

// Use external transforms in calc_gramar
struct calc_grammar
  : proto::or_<
        proto::when<
            proto::terminal<placeholder<_> >
            , proto::functional::at(proto::_state, proto::_value)
        >
      , proto::when<
            proto::terminal<proto::convertible_to<double> >
          , proto::_value
        >
      , proto::when<
            proto::plus<calc_grammar, calc_grammar>
          , proto::_default<calc_grammar>
        >
      , proto::when<
            proto::minus<calc_grammar, calc_grammar>
          , proto::_default<calc_grammar>
        >
      , proto::when<
            proto::multiplies<calc_grammar, calc_grammar>
          , proto::_default<calc_grammar>
        >
        // Note that we don't specify how division nodes are
        // handled here. Proto::external_transform is a placeholder
        // for an actual transform.
      , proto::when<
            divides_rule
          , proto::external_transform
        >
    >
{};

template<typename E> struct calc_expr;
struct calc_domain : proto::domain<proto::generator<calc_expr> > {};

template<typename E>
struct calc_expr
  : proto::extends<E, calc_expr<E>, calc_domain>
{
    calc_expr(E const &e = E()) : calc_expr::proto_extends(e) {}
};

calc_expr<proto::terminal<placeholder<mpl::int_<0> > >::type> _1;
calc_expr<proto::terminal<placeholder<mpl::int_<1> > >::type> _2;

// Use proto::external_transforms to map from named grammar rules to
// transforms.
struct non_checked_division
  : proto::external_transforms<
        proto::when< divides_rule, proto::_default<calc_grammar> >
    >
{};

struct division_by_zero : std::exception {};

struct do_checked_divide
  : proto::callable
{
    typedef int result_type;
    int operator()(int left, int right) const
    {
        if (right == 0) throw division_by_zero();
        return left / right;
    }
};

// Use proto::external_transforms again, this time to map the divides_rule
// to a transforms that performs checked division.
struct checked_division
  : proto::external_transforms<
        proto::when<
            divides_rule
          , do_checked_divide(calc_grammar(proto::_left), calc_grammar(proto::_right))
        >
    >
{};

BOOST_PROTO_DEFINE_ENV_VAR(mydata_tag, mydata);

void test_external_transforms()
{
    non_checked_division non_checked;
    int result1 = calc_grammar()(_1 / _2, fusion::make_vector(6, 2), non_checked);
    BOOST_CHECK_EQUAL(result1, 3);

    // check that additional data slots are ignored
    int result2 = calc_grammar()(_1 / _2, fusion::make_vector(8, 2), (non_checked, mydata = "foo"));
    BOOST_CHECK_EQUAL(result2, 4);

    // check that we can use the dedicated slot for this purpose
    int result3 = calc_grammar()(_1 / _2, fusion::make_vector(8, 2), (42, proto::transforms = non_checked, mydata = "foo"));
    BOOST_CHECK_EQUAL(result2, 4);

    checked_division checked;
    try
    {
        // This should throw
        int result3 = calc_grammar()(_1 / _2, fusion::make_vector(6, 0), checked);
        BOOST_CHECK(!"Didn't throw an exception"); // shouldn't get here!
    }
    catch(division_by_zero)
    {
        ; // OK
    }
    catch(...)
    {
        BOOST_CHECK(!"Unexpected exception"); // shouldn't get here!
    }

    try
    {
        // This should throw
        int result4 = calc_grammar()(_1 / _2, fusion::make_vector(6, 0), (checked, mydata = test_external_transforms));
        BOOST_CHECK(!"Didn't throw an exception"); // shouldn't get here!
    }
    catch(division_by_zero)
    {
        ; // OK
    }
    catch(...)
    {
        BOOST_CHECK(!"Unexpected exception"); // shouldn't get here!
    }

    try
    {
        // This should throw
        int result5 = calc_grammar()(_1 / _2, fusion::make_vector(6, 0), (42, proto::transforms = checked, mydata = test_external_transforms));
        BOOST_CHECK(!"Didn't throw an exception"); // shouldn't get here!
    }
    catch(division_by_zero)
    {
        ; // OK
    }
    catch(...)
    {
        BOOST_CHECK(!"Unexpected exception"); // shouldn't get here!
    }
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test for external transforms");

    test->add(BOOST_TEST_CASE(&test_external_transforms));

    return test;
}
