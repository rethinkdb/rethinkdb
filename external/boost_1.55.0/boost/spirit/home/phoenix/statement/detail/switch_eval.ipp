/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_STATEMENT_DETAIL_SWITCH_EVAL_IPP
#define PHOENIX_STATEMENT_DETAIL_SWITCH_EVAL_IPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define PHOENIX_CASE_ITEM(z, n, prefix)                                         \
    case BOOST_PP_CAT(Case, n)::value:                                          \
        BOOST_PP_CAT(_, n).eval(env);                                           \
        break;

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (3, BOOST_PP_DEC(BOOST_PP_DEC(PHOENIX_COMPOSITE_LIMIT)),                \
    "boost/spirit/home/phoenix/statement/detail/switch_eval.ipp"))
#include BOOST_PP_ITERATE()

#undef PHOENIX_CASE_ITEM
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    template <>
    struct switch_eval<N>
    {
        template <
            typename Env, typename Cond, typename Default
          , BOOST_PP_ENUM_PARAMS(N, typename Case)
        >
        struct result
        {
            typedef void type;
        };

        template <
            typename RT, typename Env, typename Cond, typename Default
          , BOOST_PP_ENUM_PARAMS(N, typename Case)
        >
        static void
        eval(Env const& env, Cond& cond, Default& default_
          , BOOST_PP_ENUM_BINARY_PARAMS(N, Case, & _)
        )
        {
            switch (cond.eval(env))
            {
                BOOST_PP_REPEAT(N, PHOENIX_CASE_ITEM, _)
                default:
                    default_.eval(env);
            }
        }
    };

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


