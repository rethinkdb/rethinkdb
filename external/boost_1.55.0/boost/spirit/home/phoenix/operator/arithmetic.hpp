/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_ARITHMETIC_HPP
#define PHOENIX_OPERATOR_ARITHMETIC_HPP

#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/detail/type_deduction.hpp>
#include <boost/spirit/home/phoenix/operator/detail/unary_eval.hpp>
#include <boost/spirit/home/phoenix/operator/detail/unary_compose.hpp>
#include <boost/spirit/home/phoenix/operator/detail/binary_eval.hpp>
#include <boost/spirit/home/phoenix/operator/detail/binary_compose.hpp>

namespace boost { namespace phoenix
{
    struct negate_eval;
    struct posit_eval;
    struct pre_increment_eval;
    struct pre_decrement_eval;
    struct post_increment_eval;
    struct post_decrement_eval;

    struct plus_assign_eval;
    struct minus_assign_eval;
    struct multiplies_assign_eval;
    struct divides_assign_eval;
    struct modulus_assign_eval;

    struct plus_eval;
    struct minus_eval;
    struct multiplies_eval;
    struct divides_eval;
    struct modulus_eval;

    BOOST_UNARY_RESULT_OF(-x, result_of_negate)
    BOOST_UNARY_RESULT_OF(+x, result_of_posit)
    BOOST_UNARY_RESULT_OF(++x, result_of_pre_increment)
    BOOST_UNARY_RESULT_OF(--x, result_of_pre_decrement)
    BOOST_UNARY_RESULT_OF(x++, result_of_post_increment)
    BOOST_UNARY_RESULT_OF(x--, result_of_post_decrement)

    BOOST_BINARY_RESULT_OF(x += y, result_of_plus_assign)
    BOOST_BINARY_RESULT_OF(x -= y, result_of_minus_assign)
    BOOST_BINARY_RESULT_OF(x *= y, result_of_multiplies_assign)
    BOOST_BINARY_RESULT_OF(x /= y, result_of_divides_assign)
    BOOST_BINARY_RESULT_OF(x %= y, result_of_modulus_assign)

    BOOST_BINARY_RESULT_OF(x + y, result_of_plus)
    BOOST_BINARY_RESULT_OF(x - y, result_of_minus)
    BOOST_BINARY_RESULT_OF(x * y, result_of_multiplies)
    BOOST_BINARY_RESULT_OF(x / y, result_of_divides)
    BOOST_BINARY_RESULT_OF(x % y, result_of_modulus)

#define x a0.eval(env)
#define y a1.eval(env)

    PHOENIX_UNARY_EVAL(negate_eval, result_of_negate, -x)
    PHOENIX_UNARY_EVAL(posit_eval, result_of_posit, +x)
    PHOENIX_UNARY_EVAL(pre_increment_eval, result_of_pre_increment, ++x)
    PHOENIX_UNARY_EVAL(pre_decrement_eval, result_of_pre_decrement, --x)
    PHOENIX_UNARY_EVAL(post_increment_eval, result_of_post_increment, x++)
    PHOENIX_UNARY_EVAL(post_decrement_eval, result_of_post_decrement, x--)

    PHOENIX_BINARY_EVAL(plus_assign_eval, result_of_plus_assign, x += y)
    PHOENIX_BINARY_EVAL(minus_assign_eval, result_of_minus_assign, x -= y)
    PHOENIX_BINARY_EVAL(multiplies_assign_eval, result_of_multiplies_assign, x *= y)
    PHOENIX_BINARY_EVAL(divides_assign_eval, result_of_divides_assign, x /= y)
    PHOENIX_BINARY_EVAL(modulus_assign_eval, result_of_modulus_assign, x %= y)

    PHOENIX_BINARY_EVAL(plus_eval, result_of_plus, x + y)
    PHOENIX_BINARY_EVAL(minus_eval, result_of_minus, x - y)
    PHOENIX_BINARY_EVAL(multiplies_eval, result_of_multiplies, x * y)
    PHOENIX_BINARY_EVAL(divides_eval, result_of_divides, x / y)
    PHOENIX_BINARY_EVAL(modulus_eval, result_of_modulus, x % y)

    PHOENIX_UNARY_COMPOSE(negate_eval, -)
    PHOENIX_UNARY_COMPOSE(posit_eval, +)
    PHOENIX_UNARY_COMPOSE(pre_increment_eval, ++)
    PHOENIX_UNARY_COMPOSE(pre_decrement_eval, --)

    template <typename T0>
    inline actor<typename as_composite<post_increment_eval, actor<T0> >::type>
    operator++(actor<T0> const& a0, int) // special case
    {
        return compose<post_increment_eval>(a0);
    }

    template <typename T0>
    inline actor<typename as_composite<post_decrement_eval, actor<T0> >::type>
    operator--(actor<T0> const& a0, int) // special case
    {
        return compose<post_decrement_eval>(a0);
    }

    PHOENIX_BINARY_COMPOSE(plus_assign_eval, +=)
    PHOENIX_BINARY_COMPOSE(minus_assign_eval, -=)
    PHOENIX_BINARY_COMPOSE(multiplies_assign_eval, *=)
    PHOENIX_BINARY_COMPOSE(divides_assign_eval, /=)
    PHOENIX_BINARY_COMPOSE(modulus_assign_eval, %=)

    PHOENIX_BINARY_COMPOSE(plus_eval, +)
    PHOENIX_BINARY_COMPOSE(minus_eval, -)
    PHOENIX_BINARY_COMPOSE(multiplies_eval, *)
    PHOENIX_BINARY_COMPOSE(divides_eval, /)
    PHOENIX_BINARY_COMPOSE(modulus_eval, %)

#undef x
#undef y
}}

#endif
