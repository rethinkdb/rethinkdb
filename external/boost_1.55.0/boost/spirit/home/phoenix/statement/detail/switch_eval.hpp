/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_STATEMENT_DETAIL_SWITCH_EVAL_HPP
#define PHOENIX_STATEMENT_DETAIL_SWITCH_EVAL_HPP

namespace boost { namespace phoenix { namespace detail
{
    template <int N>
    struct switch_eval;

    template <>
    struct switch_eval<0>
    {
        template <
            typename Env, typename Cond, typename Default
        >
        struct result
        {
            typedef void type;
        };

        template <
            typename RT, typename Env, typename Cond, typename Default
        >
        static void
        eval(
            Env const& env, Cond& cond, Default& default_
        )
        {
            default_.eval(env);
        }
    };

    template <>
    struct switch_eval<1>
    {
        template <
            typename Env, typename Cond, typename Default
          , typename Case0
        >
        struct result
        {
            typedef void type;
        };

        template <
            typename RT, typename Env, typename Cond, typename Default
          , typename Case0
        >
        static void
        eval(
            Env const& env, Cond& cond, Default& default_
          , Case0& _0
        )
        {
            switch (cond.eval(env))
            {
                case Case0::value:
                    _0.eval(env);
                    break;
                default:
                    default_.eval(env);
            }
        }
    };

    template <>
    struct switch_eval<2>
    {
        template <
            typename Env, typename Cond, typename Default
          , typename Case0, typename Case1
        >
        struct result
        {
            typedef void type;
        };

        template <
            typename RT, typename Env, typename Cond, typename Default
          , typename Case0, typename Case1
        >
        static void
        eval(
            Env const& env, Cond& cond, Default& default_
          , Case0& _0, Case1& _1
        )
        {
            switch (cond.eval(env))
            {
                case Case0::value:
                    _0.eval(env);
                    break;
                case Case1::value:
                    _1.eval(env);
                    break;
                default:
                    default_.eval(env);
            }
        }
    };

    //  Bring in the rest of the switch_evals
    #include <boost/spirit/home/phoenix/statement/detail/switch_eval.ipp>
}}}

#endif
