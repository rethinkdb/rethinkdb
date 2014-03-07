//[ Calc1
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This is a simple example of how to build an arithmetic expression
// evaluator with placeholders.

#include <iostream>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
namespace proto = boost::proto;
using proto::_;

template<int I> struct placeholder {};

// Define some placeholders
proto::terminal< placeholder< 1 > >::type const _1 = {{}};
proto::terminal< placeholder< 2 > >::type const _2 = {{}};

// Define a calculator context, for evaluating arithmetic expressions
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

template<typename Expr>
double evaluate( Expr const &expr, double d1 = 0., double d2 = 0. )
{
    // Create a calculator context with d1 and d2 substituted for _1 and _2
    calculator_context const ctx(d1, d2);

    // Evaluate the calculator expression with the calculator_context
    return proto::eval(expr, ctx);
}

int main()
{
    // Displays "5"
    std::cout << evaluate( _1 + 2.0, 3.0 ) << std::endl;

    // Displays "6"
    std::cout << evaluate( _1 * _2, 3.0, 2.0 ) << std::endl;

    // Displays "0.5"
    std::cout << evaluate( (_1 - _2) / _2, 3.0, 2.0 ) << std::endl;

    return 0;
}
//]
