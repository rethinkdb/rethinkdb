/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_COMPARISON_HPP
#define PHOENIX_OPERATOR_COMPARISON_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/detail/type_deduction.hpp>
#include <boost/spirit/home/phoenix/operator/detail/unary_eval.hpp>
#include <boost/spirit/home/phoenix/operator/detail/unary_compose.hpp>
#include <boost/spirit/home/phoenix/operator/detail/binary_eval.hpp>
#include <boost/spirit/home/phoenix/operator/detail/binary_compose.hpp>

namespace boost { namespace phoenix
{
    struct equal_to_eval;
    struct not_equal_to_eval;
    struct less_eval;
    struct less_equal_eval;
    struct greater_eval;
    struct greater_equal_eval;

    BOOST_BINARY_RESULT_OF(x == y, result_of_equal_to)
    BOOST_BINARY_RESULT_OF(x != y, result_of_not_equal_to)
    BOOST_BINARY_RESULT_OF(x < y, result_of_less)
    BOOST_BINARY_RESULT_OF(x <= y, result_of_less_equal)
    BOOST_BINARY_RESULT_OF(x > y, result_of_greater)
    BOOST_BINARY_RESULT_OF(x >= y, result_of_greater_equal)

#define x a0.eval(env)
#define y a1.eval(env)

    PHOENIX_BINARY_EVAL(equal_to_eval, result_of_equal_to, x == y)
    PHOENIX_BINARY_EVAL(not_equal_to_eval, result_of_not_equal_to, x != y)
    PHOENIX_BINARY_EVAL(less_eval, result_of_less, x < y)
    PHOENIX_BINARY_EVAL(less_equal_eval, result_of_less_equal, x <= y)
    PHOENIX_BINARY_EVAL(greater_eval, result_of_greater, x > y)
    PHOENIX_BINARY_EVAL(greater_equal_eval, result_of_greater_equal, x >= y)

    PHOENIX_BINARY_COMPOSE(equal_to_eval, ==)
    PHOENIX_BINARY_COMPOSE(not_equal_to_eval, !=)
    PHOENIX_BINARY_COMPOSE(less_eval, <)
    PHOENIX_BINARY_COMPOSE(less_equal_eval, <=)
    PHOENIX_BINARY_COMPOSE(greater_eval, >)
    PHOENIX_BINARY_COMPOSE(greater_equal_eval, >=)

#undef x
#undef y
}}

#endif
