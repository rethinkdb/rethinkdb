/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/phoenix.hpp>
#include <boost/proto/proto.hpp>
#include <boost/proto/debug.hpp>

namespace phoenix = boost::phoenix;
namespace proto = boost::proto;

struct invert_actions
{
    template <typename Rule>
    struct when
        : proto::_
    {};
};

using phoenix::evaluator;

#ifdef _MSC_VER
// redifining evaluator, this is because MSVC chokes on function types like:
// F(G(...))
#define evaluator(A0, A1) proto::call<phoenix::evaluator(A0, A1)>
#endif

template <>
struct invert_actions::when<phoenix::rule::plus>
    : proto::call<
        phoenix::functional::make_minus(
            evaluator(proto::_left, phoenix::_context)
          , evaluator(proto::_right, phoenix::_context)
        )
    >
{};

template <>
struct invert_actions::when<phoenix::rule::minus>
    : proto::call<
        phoenix::functional::make_plus(
            evaluator(proto::_left, phoenix::_context)
          , evaluator(proto::_right, phoenix::_context)
        )
    >
{};

template <>
struct invert_actions::when<phoenix::rule::multiplies>
    : proto::call<
        phoenix::functional::make_divides(
            evaluator(proto::_left, phoenix::_context)
          , evaluator(proto::_right, phoenix::_context)
        )
    >
{};

template <>
struct invert_actions::when<phoenix::rule::divides>
    : proto::call<
        phoenix::functional::make_multiplies(
            evaluator(proto::_left, phoenix::_context)
          , evaluator(proto::_right, phoenix::_context)
        )
    >
{};

#ifdef _MSC_VER
#undef evaluator
#endif

template <typename Expr>
void print_expr(Expr const & expr)
{
    std::cout << "before inversion:\n";
    proto::display_expr(expr);
    std::cout << "after inversion:\n";
    proto::display_expr(
        phoenix::eval(
            expr
          , phoenix::context(
                phoenix::nothing
              , invert_actions()
            )
        )
    );
    std::cout << "\n";
}

template <typename Expr>
typename
    boost::phoenix::result_of::eval<
        Expr const&
      , phoenix::result_of::make_context<
            phoenix::result_of::make_env<>::type
          , invert_actions
        >::type
    >::type
invert(Expr const & expr)
{
    return 
        phoenix::eval(
            expr
          , phoenix::make_context(
                phoenix::make_env()
              , invert_actions()
            )
        );
}

int main()
{
    using phoenix::placeholders::_1;
    using phoenix::placeholders::_2;
    using phoenix::placeholders::_3;
    using phoenix::placeholders::_4;

    print_expr(_1);
    print_expr(_1 + _2);
    print_expr(_1 + _2 - _3);
    print_expr(_1 * _2);
    print_expr(_1 * _2 / _3);
    print_expr(_1 * _2 + _3);
    print_expr(_1 * _2 - _3);
    print_expr(if_(_1 * _4)[_2 - _3]);

    print_expr(_1 * invert(_2 - _3));
}

