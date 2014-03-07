/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_DETAIL_BINARY_EVAL_HPP
#define PHOENIX_OPERATOR_DETAIL_BINARY_EVAL_HPP

#include <boost/mpl/or.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>

#define PHOENIX_BINARY_EVAL(eval_name, op_result, expr)                         \
    struct eval_name                                                            \
    {                                                                           \
        template <typename Env, typename A0, typename A1>                       \
        struct result                                                            \
        {                                                                       \
            typedef typename A0::template result<Env>::type x_type;              \
            typedef typename A1::template result<Env>::type y_type;              \
                                                                                \
            typedef typename                                                    \
                mpl::eval_if<                                                   \
                    mpl::or_<is_actor<x_type>, is_actor<y_type> >               \
                  , re_curry<eval_name, x_type, y_type>                         \
                  , op_result<x_type, y_type>                                   \
                >::type                                                         \
            type;                                                               \
        };                                                                      \
                                                                                \
        template <typename RT, typename Env, typename A0, typename A1>          \
        static RT                                                               \
        eval(Env const& env, A0& a0, A1& a1)                                    \
        {                                                                       \
            return expr;                                                        \
        }                                                                       \
    };

#undef x
#undef y
#endif
