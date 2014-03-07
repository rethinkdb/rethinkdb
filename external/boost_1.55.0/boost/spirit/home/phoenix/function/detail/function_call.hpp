/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_FUNCTION_DETAIL_FUNCTION_CALL_HPP
#define PHOENIX_FUNCTION_DETAIL_FUNCTION_CALL_HPP

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (3, BOOST_PP_DEC(PHOENIX_COMPOSITE_LIMIT),                              \
    "boost/spirit/home/phoenix/function/detail/function_call.hpp"))
#include BOOST_PP_ITERATE()

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    template <BOOST_PP_ENUM_PARAMS(N, typename A)>
    actor<typename as_composite<detail::function_eval<N>, F
      , BOOST_PP_ENUM_PARAMS(N, A)>::type>
    operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& _)) const
    {
        return compose<detail::function_eval<N> >(f, BOOST_PP_ENUM_PARAMS(N, _));
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


