/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_IF_ELSE_HPP
#define PHOENIX_OPERATOR_IF_ELSE_HPP

#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/detail/type_deduction.hpp>

namespace boost { namespace phoenix
{
    BOOST_BINARY_RESULT_OF(true ? x : y, result_of_if_else)

    struct if_else_op_eval
    {
        template <
            typename Env
          , typename Cond
          , typename Then
          , typename Else
        >
        struct result
        {
            typedef typename Then::template result<Env>::type then_type;
            typedef typename Else::template result<Env>::type else_type;

            typedef typename
                result_of_if_else<then_type, else_type>::type
            ite_result;

            // Note: c ? x : y can return an lvalue! Allow if_else_op_eval
            // to return an lvalue IFF then_type and else_type are both lvalues
            // with the same type.

            typedef typename
                mpl::if_<
                    mpl::and_<
                        is_same<then_type, else_type>
                      , is_reference<then_type>
                    >
                  , ite_result
                  , typename remove_reference<ite_result>::type
                >::type
            type;
        };

        template <
            typename RT
          , typename Env
          , typename Cond
          , typename Then
          , typename Else
        >
        static RT
        eval(Env const& env, Cond& cond, Then& then, Else& else_)
        {
            return cond.eval(env) ? then.eval(env) : else_.eval(env);
        }
    };

    template <typename Cond, typename Then, typename Else>
    inline actor<typename as_composite<if_else_op_eval, Cond, Then, Else>::type>
    if_else(Cond const& cond, Then const& then, Else const& else_)
    {
        return compose<if_else_op_eval>(cond, then, else_);
    }
}}

#endif
