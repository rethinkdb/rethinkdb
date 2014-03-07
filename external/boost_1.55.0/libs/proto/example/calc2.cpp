//[ Calc2
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This example enhances the simple arithmetic expression evaluator
// in calc1.cpp by using proto::extends to make arithmetic
// expressions immediately evaluable with operator (), a-la a
// function object

#include <iostream>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
namespace proto = boost::proto;
using proto::_;

template<typename Expr>
struct calculator_expression;

// Tell proto how to generate expressions in the calculator_domain
struct calculator_domain
  : proto::domain<proto::generator<calculator_expression> >
{};

// Will be used to define the placeholders _1 and _2
template<int I> struct placeholder {};

// Define a calculator context, for evaluating arithmetic expressions
// (This is as before, in calc1.cpp)
struct calculator_context
  : proto::callable_context< calculator_context const >
{
    // The values bound to the placeholders
    double d[2];

    // The result of evaluating arithmetic expressions
    typedef double result_type;

    explicit calculator_context(double d1 = 0., double d2 = 0.)
    {
        d[0] = d1;
        d[1] = d2;
    }

    // Handle the evaluation of the placeholder terminals
    template<int I>
    double operator ()(proto::tag::terminal, placeholder<I>) const
    {
        return d[ I - 1 ];
    }
};

// Wrap all calculator expressions in this type, which defines
// operator () to evaluate the expression.
template<typename Expr>
struct calculator_expression
  : proto::extends<Expr, calculator_expression<Expr>, calculator_domain>
{
    explicit calculator_expression(Expr const &expr = Expr())
      : calculator_expression::proto_extends(expr)
    {}

    BOOST_PROTO_EXTENDS_USING_ASSIGN(calculator_expression<Expr>)

    // Override operator () to evaluate the expression
    double operator ()() const
    {
        calculator_context const ctx;
        return proto::eval(*this, ctx);
    }

    double operator ()(double d1) const
    {
        calculator_context const ctx(d1);
        return proto::eval(*this, ctx);
    }

    double operator ()(double d1, double d2) const
    {
        calculator_context const ctx(d1, d2);
        return proto::eval(*this, ctx);
    }
};

// Define some placeholders (notice they're wrapped in calculator_expression<>)
calculator_expression<proto::terminal< placeholder< 1 > >::type> const _1;
calculator_expression<proto::terminal< placeholder< 2 > >::type> const _2;

// Now, our arithmetic expressions are immediately executable function objects:
int main()
{
    // Displays "5"
    std::cout << (_1 + 2.0)( 3.0 ) << std::endl;

    // Displays "6"
    std::cout << ( _1 * _2 )( 3.0, 2.0 ) << std::endl;

    // Displays "0.5"
    std::cout << ( (_1 - _2) / _2 )( 3.0, 2.0 ) << std::endl;

    return 0;
}
//]
