/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_BITWISE_HPP
#define PHOENIX_OPERATOR_BITWISE_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/detail/type_deduction.hpp>
#include <boost/spirit/home/phoenix/operator/detail/unary_eval.hpp>
#include <boost/spirit/home/phoenix/operator/detail/unary_compose.hpp>
#include <boost/spirit/home/phoenix/operator/detail/binary_eval.hpp>
#include <boost/spirit/home/phoenix/operator/detail/binary_compose.hpp>

#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable : 4800)
#endif

namespace boost { namespace phoenix
{
    struct invert_eval;

    struct and_assign_eval;
    struct or_assign_eval;
    struct xor_assign_eval;
    struct shift_left_assign_eval;
    struct shift_right_assign_eval;

    struct and_eval;
    struct or_eval;
    struct xor_eval;
    struct shift_left_eval;
    struct shift_right_eval;

    BOOST_UNARY_RESULT_OF(~x, result_of_invert)

    BOOST_BINARY_RESULT_OF(x &= y, result_of_and_assign)
    BOOST_BINARY_RESULT_OF(x |= y, result_of_or_assign)
    BOOST_BINARY_RESULT_OF(x ^= y, result_of_xor_assign)
    BOOST_BINARY_RESULT_OF(x <<= y, result_of_shift_left_assign)
    BOOST_BINARY_RESULT_OF(x >>= y, result_of_shift_right_assign)

    BOOST_BINARY_RESULT_OF(x & y, result_of_and)
    BOOST_BINARY_RESULT_OF(x | y, result_of_or)
    BOOST_BINARY_RESULT_OF(x ^ y, result_of_xor)
    BOOST_BINARY_RESULT_OF(x << y, result_of_shift_left)
    BOOST_BINARY_RESULT_OF(x >> y, result_of_shift_right)

#define x a0.eval(env)
#define y a1.eval(env)

    PHOENIX_UNARY_EVAL(invert_eval, result_of_invert, ~x)
    PHOENIX_UNARY_COMPOSE(invert_eval, ~)

    PHOENIX_BINARY_EVAL(and_assign_eval, result_of_and_assign, x &= y)
    PHOENIX_BINARY_EVAL(or_assign_eval, result_of_or_assign, x |= y)
    PHOENIX_BINARY_EVAL(xor_assign_eval, result_of_xor_assign, x ^= y)
    PHOENIX_BINARY_EVAL(shift_left_assign_eval, result_of_shift_left_assign, x <<= y)
    PHOENIX_BINARY_EVAL(shift_right_assign_eval, result_of_shift_right_assign, x >>= y)

    PHOENIX_BINARY_EVAL(and_eval, result_of_and, x & y)
    PHOENIX_BINARY_EVAL(or_eval, result_of_or, x | y)
    PHOENIX_BINARY_EVAL(xor_eval, result_of_xor, x ^ y)
    PHOENIX_BINARY_EVAL(shift_left_eval, result_of_shift_left, x << y)
    PHOENIX_BINARY_EVAL(shift_right_eval, result_of_shift_right, x >> y)

    PHOENIX_BINARY_COMPOSE(and_assign_eval, &=)
    PHOENIX_BINARY_COMPOSE(or_assign_eval, |=)
    PHOENIX_BINARY_COMPOSE(xor_assign_eval, ^=)
    PHOENIX_BINARY_COMPOSE(shift_left_assign_eval, <<=)
    PHOENIX_BINARY_COMPOSE(shift_right_assign_eval, >>=)

    PHOENIX_BINARY_COMPOSE(and_eval, &)
    PHOENIX_BINARY_COMPOSE(or_eval, |)
    PHOENIX_BINARY_COMPOSE(xor_eval, ^)
    PHOENIX_BINARY_COMPOSE(shift_left_eval, <<)
    PHOENIX_BINARY_COMPOSE(shift_right_eval, >>)

#undef x
#undef y
}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
