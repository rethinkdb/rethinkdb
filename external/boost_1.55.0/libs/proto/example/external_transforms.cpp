//[ CheckedCalc
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
#include <boost/assert.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/proto/proto.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;

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
            proto::terminal<placeholder<proto::_> >
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

int main()
{
    non_checked_division non_checked;
    int result2 = calc_grammar()(_1 / _2, fusion::make_vector(6, 2), non_checked);
    BOOST_ASSERT(result2 == 3);

    try
    {
        checked_division checked;
        // This should throw
        int result3 = calc_grammar()(_1 / _2, fusion::make_vector(6, 0), checked);
        BOOST_ASSERT(false); // shouldn't get here!
    }
    catch(division_by_zero)
    {
        std::cout << "caught division by zero!\n";
    }
}
//]
