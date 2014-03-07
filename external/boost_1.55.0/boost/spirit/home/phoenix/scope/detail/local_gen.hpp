/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
// Allow multiple inclusion. let.hpp and lambda.hpp will have the guards

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

#define PHOENIX_LOCAL_GEN_PARAM(z, n, data)                                     \
    actor<composite<assign_eval                                                 \
  , fusion::vector<local_variable<K##n>, V##n> > > const& a##n

#define PHOENIX_LOCAL_GEN_ACTOR(z, n, data)                                     \
    fusion::at_c<1>(a##n)

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (3, PHOENIX_LOCAL_LIMIT,                                                \
    "boost/spirit/home/phoenix/scope/detail/local_gen.hpp"))
#include BOOST_PP_ITERATE()

#undef PHOENIX_LOCAL_GEN_PARAM
#undef PHOENIX_LOCAL_GEN_ACTOR

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    template <
        BOOST_PP_ENUM_PARAMS(N, typename K)
      , BOOST_PP_ENUM_PARAMS(N, typename V)
    >
    PHOENIX_LOCAL_GEN_NAME<
        fusion::vector<BOOST_PP_ENUM_PARAMS(N, V)>
      , detail::map_local_index_to_tuple<BOOST_PP_ENUM_PARAMS(N, K)>
    >
    operator()(
        BOOST_PP_ENUM(N, PHOENIX_LOCAL_GEN_PARAM, _)
    ) const
    {
        return fusion::vector<BOOST_PP_ENUM_PARAMS(N, V)>(
            BOOST_PP_ENUM(N, PHOENIX_LOCAL_GEN_ACTOR, _));
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


