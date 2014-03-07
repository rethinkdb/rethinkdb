/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_CORE_DETAIL_ACTOR_HPP
#define PHOENIX_CORE_DETAIL_ACTOR_HPP

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (3, PHOENIX_ACTOR_LIMIT,                                                \
    "boost/spirit/home/phoenix/core/detail/actor.hpp"))
#include BOOST_PP_ITERATE()

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    template <BOOST_PP_ENUM_PARAMS(N, typename T)>
    actor(BOOST_PP_ENUM_BINARY_PARAMS(N, T, const& _))
        : Eval(BOOST_PP_ENUM_PARAMS(N, _)) {}

    template <typename F, BOOST_PP_ENUM_PARAMS(N, typename A)>
    struct result<F(BOOST_PP_ENUM_PARAMS(N, A))>
      : eval_result<
            eval_type
          , basic_environment<
                BOOST_PP_ENUM_BINARY_PARAMS(
                    N
                  , typename remove_reference<A
                  , >::type BOOST_PP_INTERCEPT
                )
            >
        >
    {};

    template <BOOST_PP_ENUM_PARAMS(N, typename T)>
    typename result<
        actor(BOOST_PP_ENUM_BINARY_PARAMS(N, T, & BOOST_PP_INTERCEPT))
    >::type
    operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, T, & _)) const
    {
        return eval_type::eval(
            basic_environment<BOOST_PP_ENUM_PARAMS(N, T)>(
                BOOST_PP_ENUM_PARAMS(N, _))
        );
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


